#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <rtl/ustring.hxx>

namespace filemanager
{
    // 内部辅助函数声明
    std::string convertToAbsolutePath(const std::string &path);
    rtl::OUString convertStringToOUString(const char *str);
    void make_dirs(const std::string &path);
    
    // 自动检测文本内容的语言
    std::string detectLanguage(const rtl::OUString &text);
    
    // 解析单元格地址
    void parseCellAddress(const rtl::OUString &cellAddress, sal_Int32 &col, sal_Int32 &row);
} // namespace filemanager

#endif // UTILS_H