#ifndef UNICODETABLE_H
#define UNICODETABLE_H
#include <iostream>
#include <vector>
#include <string>
namespace filemanager
{
std::string getLanguageType(char32_t ch);
std::vector<char32_t> splitToUnicode(const std::string &text);
}
#endif // UNICODETABLE_H