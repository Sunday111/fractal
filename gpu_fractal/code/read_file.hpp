#pragma once

#include <filesystem>
#include <vector>

void ReadFile(const std::filesystem::path& path, std::vector<char>& buffer);
