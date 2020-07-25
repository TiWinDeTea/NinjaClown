#ifndef NINJACLOWN_SYSTEM_HPP
#define NINJACLOWN_SYSTEM_HPP

#include <filesystem>

namespace utils {
std::filesystem::path binary_directory();

std::filesystem::path resources_directory();

std::filesystem::path config_directory();

std::string sys_last_error();
}

#endif //NINJACLOWN_SYSTEM_HPP
