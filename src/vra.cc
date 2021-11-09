#include "vra.hh"

#include <libdeflate/libdeflate.h>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "types.hh"
#include "graphics.hh"

template <typename T>
static T read_file(const std::filesystem::path& path)
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
static void write_file(const std::filesystem::path& path, const T& data)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data.data(), data.size());
}

static void write_file(const std::filesystem::path& path, const char* data, size_t size)
{
    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path()))
    {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data, size);
}

static void decompress_data(const std::string& comp, std::string& decomp)
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

static void compress_data(const std::string& decomp, std::string& comp)
{
    auto compressor = libdeflate_alloc_compressor(1);
    size_t comp_size = libdeflate_zlib_compress(compressor, decomp.c_str(), decomp.size(), comp.data(), decomp.size());
    libdeflate_free_compressor(compressor);
    comp.resize(comp_size);
}

static void unpack_data(const std::filesystem::path& dst, const std::string& data_str)
{
    const char* data = data_str.c_str();
    data += 4;
    size_t nb_files = *((uint16_t*)data) + 3;
    data += 2;
    std::string repack_list;
    repack_list.reserve(nb_files * 64);
    repack_list += std::to_string(nb_files);
    repack_list += "\n";
    repack_list += std::to_string(data_str.size());
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
        write_file(dst / file_name, data, file_size);
        data += file_size;

        if (i % 100 == 0)
        {
            g_vra_data.progress = static_cast<float>(i + 1) / nb_files;
            render(g_render_data, g_vra_data, false);
        }
    }
    g_vra_data.progress = 1.0f;

    write_file(dst / "repack_list.txt", repack_list);
}

void unpack_file(const std::filesystem::path& src, const std::filesystem::path& dst)
{
    if (std::filesystem::exists(dst))
    {
        throw std::runtime_error(dst.string() + " already exists");
    }

    auto comp = read_file<std::string>(src);
    std::string decomp(comp.size() * 2, 0);
    decompress_data(comp, decomp);
    unpack_data(dst, decomp);
}

static void pack_file(const std::filesystem::path& data_path, const std::filesystem::path& file_path, std::stringstream& sstream)
{
    const std::string& file_path_str = file_path.string();
    uint32_t file_name_size = file_path_str.size();
    auto file_data = read_file<std::string>(data_path / file_path);
    uint32_t file_size = file_data.size();

    sstream.write((char*)&file_name_size, sizeof(file_name_size));
    sstream.write(file_path_str.c_str(), file_name_size);
    sstream.write((char*)&file_size, sizeof(file_size));
    sstream.write(file_data.c_str(), file_data.size());
}

void pack_dir(const std::filesystem::path& src, const std::filesystem::path& dst)
{
    if (std::filesystem::exists(dst))
    {
        throw std::runtime_error(dst.string() + " already exists");
    }

    std::filesystem::path repack_list_path = src / "repack_list.txt";

    if (!std::filesystem::exists(repack_list_path))
    {
        throw std::runtime_error("Cannot be repack directory (missing repack_list.txt)");
    }

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
        pack_file(src, line, sstream);

        if (i % 100 == 0)
        {
            g_vra_data.progress = static_cast<float>(i + 1) / (nb_files * 2);
            render(g_render_data, g_vra_data, false);
        }
    }
    g_vra_data.progress = 0.5f;
    g_vra_data.log_buffer.appendf("Compressing...\n");
    render(g_render_data, g_vra_data, false);

    compress_data(sstream.str(), comp_data);
    write_file(dst, comp_data);

    g_vra_data.progress = 1.0f;
}

void load_mods(const std::filesystem::path& vra)
{
    std::filesystem::path unpacked = vra;
    unpacked.replace_extension("");
    unpacked.replace_filename("modded_" + unpacked.filename().string());

    g_vra_data.log_buffer.appendf("Unpacking...\n");

    unpack_file(vra, unpacked);

    for (size_t i = 0; i < g_vra_data.mods.size(); ++i)
    {
        const auto& mod = g_vra_data.mods[i];
        g_vra_data.log_buffer.appendf("Loading mod: %s\n", mod.name.c_str());

        std::filesystem::path mod_dir = g_vra_data.mod_dir;
        mod_dir /= mod.name;

        if (!std::filesystem::is_directory(mod_dir))
        {
            g_vra_data.log_buffer.appendf("%ls is not a valid mod\n", mod_dir.c_str());
            continue;
        }

        for (const auto& dirent : std::filesystem::directory_iterator(mod_dir))
        {
            std::filesystem::copy(dirent, unpacked / dirent.path().filename(),
                                  std::filesystem::copy_options::recursive
                                      | std::filesystem::copy_options::overwrite_existing);
        }

        g_vra_data.progress = static_cast<float>(i + 1) / g_vra_data.mods.size();
    }
    g_vra_data.progress = 1.0f;
    g_vra_data.log_buffer.appendf("Repacking...\n");

    std::filesystem::path modded_vra = unpacked;
    modded_vra.replace_filename(modded_vra.filename().string() + ".vra");
    pack_dir(unpacked, modded_vra);
    std::filesystem::remove_all(unpacked);
}
