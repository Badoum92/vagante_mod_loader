#pragma once

#include <filesystem>

void unpack_file(const std::filesystem::path& src, const std::filesystem::path& dst);
void pack_dir(const std::filesystem::path& src, const std::filesystem::path& dst);
void load_mods(const std::filesystem::path& vra);
