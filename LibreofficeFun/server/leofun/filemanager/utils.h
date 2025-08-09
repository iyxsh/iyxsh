#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <rtl/ustring.hxx>

namespace filemanager
{
    // 内部辅助函数声明
    std::string convertToAbsolutePath(const std::string &path);
    rtl::OUString convertStringToOUString(const char *str);
    std::string convertOUStringToString(const rtl::OUString &oustr);
    void make_dirs(const std::string &path);
    
    // 自动检测文本内容的语言
    std::string detectLanguage(const rtl::OUString &text);
    
    // 解析单元格地址
    void parseCellAddress(const rtl::OUString &cellAddress, sal_Int32 &col, sal_Int32 &row);
    // 获取文件的绝对路径
    void getAbsolutePath(const rtl::OUString &fileName, rtl::OUString &absoluteFilePath);
    // 获取默认数据文件的路径
    void getDefaultData(rtl::OUString &defaultFilePath, rtl::OUString &wordsSheetName);
} // namespace filemanager

#endif // UTILS_H