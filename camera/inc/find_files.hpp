/// \file find_files.hpp
/// Basic file finding utils
#ifndef LIGHTBOX_CAMERA_FIND_FILE_HPP_
#define LIGHTBOX_CAMERA_FIND_FILE_HPP_

#include <filesystem>
#include <string>
#include <vector>

namespace zebral
{
/// Struct to simplify really using file searches
struct FindFilesMatch
{
  std::filesystem::directory_entry dir_entry;
  std::vector<std::string> matches;
};

/// Returns a list of files from the base directory that match the regex string.
/// Regex is case-insensitive ECMAScript.
std::vector<FindFilesMatch> FindFiles(const std::filesystem::path& base,
                                      const std::string& regex_str);
}  // namespace zebral
#endif  // LIGHTBOX_CAMERA_FIND_FILE_HPP_