/// \file find_files.cpp
/// Utility for finding files that match a regex
#include "find_files.hpp"
#include <fstream>
#include <iostream>
#include <regex>

namespace zebral
{
std::vector<FindFilesMatch> FindFiles(const std::filesystem::path& base,
                                      const std::string& regex_str)
{
  std::vector<FindFilesMatch> files;
  std::regex fn_regex(regex_str, std::regex_constants::ECMAScript | std::regex_constants::icase);

  for (auto& entry : std::filesystem::directory_iterator{base})
  {
    std::string filename = entry.path().filename().string();
    std::smatch fn_match;

    if (std::regex_match(filename, fn_match, fn_regex))
    {
      std::vector<std::string> matches;
      for (size_t i = 0; i < fn_match.size(); ++i)
      {
        matches.emplace_back(fn_match[i].str());
      }
      files.emplace_back(FindFilesMatch{entry, matches});
    }
  }
  return files;
}
}  // namespace zebral