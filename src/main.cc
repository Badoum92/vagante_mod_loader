#include <libdeflate_win32/libdeflate.h>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <sstream>

template <typename T>
T read_file(const std::filesystem::path& path)
{
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (file.fail())
    {
        throw std::runtime_error("Could not read file " + path.string());
    }

    std::streampos end = file.tellg();
    file.seekg(0, std::ios::beg);
    std::streampos begin = file.tellg();

    T result;
    result.resize(static_cast<size_t>(end - begin) / sizeof(typename T::value_type));

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(result.data()), end - begin);
    file.close();

    return result;
}

template <typename T>
void write_file(const std::filesystem::path& path, const T& data)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data.data(), data.size());
}

void write_file(const std::filesystem::path& path, const char* data, size_t size)
{
    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path()))
    {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data, size);
}

void decompress_data(const std::string& comp, std::string& decomp)
{
    auto decompressor = libdeflate_alloc_decompressor();
    size_t decomp_size = 0;
    auto status = libdeflate_zlib_decompress(decompressor, comp.c_str(), comp.size(), decomp.data(), comp.size() * 2,
                                             &decomp_size);
    if (status != LIBDEFLATE_SUCCESS)
    {
        std::cerr << "Could not decompress data\n";
        exit(1);
    }
    libdeflate_free_decompressor(decompressor);
    decomp.resize(decomp_size);
}

void compress_data(const std::string& decomp, std::string& comp)
{
    auto compressor = libdeflate_alloc_compressor(1);
    size_t comp_size = libdeflate_zlib_compress(compressor, decomp.c_str(), decomp.size(), comp.data(), decomp.size());
    libdeflate_free_compressor(compressor);
    comp.resize(comp_size);
}

void unpack_data(const std::filesystem::path& path, const std::string& str)
{
    const char* data = str.c_str();
    data += 4;
    size_t nb_files = *((uint16_t*)data) + 3;
    data += 2;
    std::string repack_list;
    repack_list.reserve(nb_files * 64);
    repack_list += std::to_string(nb_files);
    repack_list += "\n";
    repack_list += std::to_string(str.size());
    repack_list += "\n";
    for (size_t i = 0; i < nb_files; ++i)
    {
        size_t file_name_size = *((uint32_t*)data);
        data += 4;
        std::string file_name(data, file_name_size);
        data += file_name_size;
        repack_list += file_name;
        repack_list += "\n";
        size_t file_size = *((uint32_t*)data);
        data += 4;
        write_file(path / file_name, data, file_size);
        data += file_size;
    }
    write_file(path / "repack_list.txt", repack_list);
}

void unpack_file(const std::filesystem::path& path)
{
    auto comp = read_file<std::string>(path);
    std::string decomp(comp.size() * 2, 0);
    decompress_data(comp, decomp);
    unpack_data(path.stem(), decomp);
}

void pack_file(const std::filesystem::path& file_path, std::stringstream& sstream)
{
    const std::string& file_path_str = file_path.string();
    uint32_t file_name_size = file_path_str.size();
    std::filesystem::path true_file_path = "data" / file_path;
    auto file_data = read_file<std::string>(true_file_path);
    uint32_t file_size = file_data.size();

    sstream.write((char*)&file_name_size, sizeof(file_name_size));
    sstream.write(file_path_str.c_str(), file_name_size);
    sstream.write((char*)&file_size, sizeof(file_size));
    sstream.write(file_data.c_str(), file_data.size());
}

void pack_dir(const std::filesystem::path& dir_path)
{
    std::string new_path_str = "new_" + dir_path.string() + ".vra";
    std::filesystem::path new_path(new_path_str);
    std::filesystem::path repack_list_path = dir_path / "repack_list.txt";
    std::ifstream repack_list(repack_list_path);

    std::string line;
    std::getline(repack_list, line);
    uint16_t nb_files = std::stoi(line);
    uint16_t true_nb_files = nb_files - 3;
    std::getline(repack_list, line);
    size_t total_size = std::stoi(line);

    std::stringstream sstream;

    std::string decomp_data;
    std::string comp_data(total_size, 0);

    unsigned char magic[4] = {0x78, 0xda, 0xec, 0xdd};
    sstream.write((char*)magic, sizeof(magic));
    sstream.write((char*)&true_nb_files, sizeof(true_nb_files));

    for (size_t i = 0; i < nb_files; ++i)
    {
        std::getline(repack_list, line);
        pack_file(line, sstream);
    }

    compress_data(sstream.str(), comp_data);
    write_file(new_path, comp_data);
}

int main(int, char** argv)
{
    unpack_file(argv[1]);
    pack_dir("data");
}
