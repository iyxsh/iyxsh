#include "utils.h"
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include "../config/json_config.h"
#include "../logger/logger.h"

namespace filemanager
{
    // 移除文件名后缀
    std::string removeFileExtension(const std::string &filename)
    {
        // 查找最后一个点的位置
        size_t lastDot = filename.find_last_of('.');

        // 如果没有找到点，或者点在第一个位置，或者点在最后一个位置，则返回原文件名
        if (lastDot == std::string::npos || lastDot == 0 || lastDot == filename.length() - 1)
        {
            return filename;
        }

        // 返回不包含后缀的文件名
        return filename.substr(0, lastDot);
    }

    // 确保文件名有.ods后缀 - 修正版：移除重复判断
    std::string ensureOdsExtension(const std::string &filename)
    {
        // 如果文件名已经以.ods或.ODS结尾，则直接返回
        if (filename.length() >= 4 &&
            (filename.substr(filename.length() - 4) == ".ods" ||
             filename.substr(filename.length() - 4) == ".ODS"))
        {
            return filename;
        }

        // 检查文件名是否已经有其他后缀
        size_t lastDotPos = filename.find_last_of('.');
        if (lastDotPos != std::string::npos && lastDotPos > 0)
        {
            // 有其他后缀，替换为.ods
            return filename.substr(0, lastDotPos) + ".ods";
        }

        // 没有后缀，直接添加.ods
        return filename + ".ods";
    }

    // 实现工具函数 - 修正版：正确处理所有类型的相对路径
    std::string convertToAbsolutePath(const std::string &path)
    {
        std::string absolutePath = path;
        
        // 检查路径是否已经是绝对路径
        bool isAbsolute = false;
        #ifdef _WIN32
        // Windows: 绝对路径判断 (如 C:/, C:\ 或 \ 开头)
        if ((path.length() > 2 && isalpha(path[0]) && path[1] == ':' && 
            (path[2] == '/' || path[2] == '\\')) || path[0] == '\\') {
            isAbsolute = true;
        }
        #else
        // Unix/Linux/Mac: 绝对路径判断 (以 / 开头)
        if (path[0] == '/') {
            isAbsolute = true;
        }
        #endif
        
        // 如果已经是绝对路径，直接返回
        if (isAbsolute) {
            return absolutePath;
        }
        
        // 获取当前工作目录
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            std::cerr << "convertToAbsolutePath: failed to get current working directory" << std::endl;
            return absolutePath; // 获取失败时返回原路径
        }
        
        std::string cwdStr(cwd);
        std::string result = cwdStr;
        
        // 处理路径分隔符（统一使用/）
        std::string processedPath = path;
        #ifdef _WIN32
        std::replace(processedPath.begin(), processedPath.end(), '\\', '/');
        #endif
        
        // 分割路径并处理每个部分
        size_t start = 0;
        size_t end = 0;
        while ((end = processedPath.find('/', start)) != std::string::npos) {
            std::string segment = processedPath.substr(start, end - start);
            if (!segment.empty()) {
                if (segment == "..") {
                    // 回退到父目录
                    size_t lastSlash = result.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        result = result.substr(0, lastSlash);
                    }
                } else if (segment != ".") {
                    // 添加非当前目录的部分
                    result += "/" + segment;
                }
                // segment == "." 时，跳过当前目录
            }
            start = end + 1;
        }
        
        // 处理最后一个路径段
        if (start < processedPath.length()) {
            std::string segment = processedPath.substr(start);
            if (!segment.empty()) {
                if (segment == "..") {
                    // 回退到父目录
                    size_t lastSlash = result.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        result = result.substr(0, lastSlash);
                    }
                } else if (segment != ".") {
                    // 添加非当前目录的部分
                    result += "/" + segment;
                }
            }
        }
        
        // 确保路径格式正确
        #ifdef _WIN32
        std::replace(result.begin(), result.end(), '/', '\\');
        #endif
        
        return result;
    }

    // 将C字符串转换为OUString，自动处理UTF-8编码
    rtl::OUString convertStringToOUString(const char *str)
    {
        if (!str)
            return rtl::OUString();

        rtl::OString oStr(str);
        return rtl::OStringToOUString(oStr, RTL_TEXTENCODING_UTF8);
    }

    // 将OUString转换为std::string，使用UTF-8编码
    std::string convertOUStringToString(const rtl::OUString &oustr)
    {
        rtl::OString oStr = rtl::OUStringToOString(oustr, RTL_TEXTENCODING_UTF8);
        return std::string(oStr.getStr());
    }

    void make_dirs(const std::string &path)
    {
#if __cplusplus >= 201703L
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec)
        {
            std::cerr << "[make_dirs] Failed to create directory: " << path << " : " << ec.message() << std::endl;
        }
#else
        size_t pos = 0;
        do
        {
            pos = path.find_first_of("/\\", pos + 1);
            std::string sub = path.substr(0, pos);
            if (!sub.empty() && sub != "." && sub != "..")
            {
                mkdir(sub.c_str(), 0755);
            }
        } while (pos != std::string::npos);
#endif
    }

    // 自动检测文本内容的语言
    std::string detectLanguage(const rtl::OUString &text)
    {
        // 获取文本长度
        sal_Int32 length = text.getLength();
        if (length == 0)
        {
            return "unknown";
        }

        // 统计各类字符
        int chineseChars = 0;
        int englishChars = 0;
        int digits = 0;
        int spaces = 0;
        int others = 0;

        for (sal_Int32 i = 0; i < length; ++i)
        {
            sal_Unicode c = text[i];

            // 中文字符范围判断（基本汉字）
            // 注意：sal_Unicode 是 unsigned short 类型，范围是 0x0000-0xFFFF
            // 所以只检查基本平面的中文字符范围
            if ((c >= 0x4E00 && c <= 0x9FFF) || // CJK统一汉字
                (c >= 0x3400 && c <= 0x4DBF))
            { // CJK扩展A
                chineseChars++;
            }
            // 英文字母
            else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            {
                englishChars++;
            }
            // 数字
            else if (c >= '0' && c <= '9')
            {
                digits++;
            }
            // 空格和制表符
            else if (c == ' ' || c == '\t')
            {
                spaces++;
            }
            // 其他字符
            else
            {
                others++;
            }
        }

        // 判断主要语言
        if (chineseChars > 0 && (chineseChars + others == length))
        {
            return "zh"; // 纯中文（包括标点符号等其他字符）
        }
        else if (englishChars > 0 && (englishChars + others == length))
        {
            return "en"; // 纯英文（包括标点符号等其他字符）
        }
        else if (digits > 0 && (digits + others == length))
        {
            return "num"; // 纯数字（包括标点符号等其他字符）
        }
        else if (spaces > 0 && (spaces == length))
        {
            return "space"; // 纯空格
        }
        else if (chineseChars > 0 && englishChars > 0)
        {
            return "zh-en"; // 中英文混合
        }
        else if (chineseChars > 0 && digits > 0)
        {
            return "zh-num"; // 中文数字混合
        }
        else if (englishChars > 0 && digits > 0)
        {
            return "en-num"; // 英文数字混合
        }
        else
        {
            return "other"; // 其他混合或无法识别的语言
        }
    }

    void parseCellAddress(const rtl::OUString &cellAddress, sal_Int32 &col, sal_Int32 &row)
    {
        col = 0;
        row = 0;
        int i = 0;
        // 解析列
        while (i < cellAddress.getLength() && iswalpha(cellAddress[i]))
        {
            col = col * 26 + (cellAddress[i] - 'A' + 1);
            ++i;
        }
        col--; // 转换为0基索引

        // 解析行
        rtl::OUString rowStr = cellAddress.copy(i);
        row = rowStr.toInt32() - 1; // 转换为0基索引
    }

    void getAbsolutePath(const rtl::OUString &fileName, rtl::OUString &absoluteFilePath)
    {
        // 先判断 fileName 是否为绝对路径
        std::string fileNameStr = rtl::OUStringToOString(fileName, RTL_TEXTENCODING_UTF8).getStr();
        bool isAbsolute = false;
        
        // 路径判断：使用正确的字符函数
        #ifdef _WIN32
        // Windows 绝对路径判断（如 C:/ 或 C:\ 或 \ 开头）
        if ((fileNameStr.length() > 2 && isalpha(fileNameStr[0]) && fileNameStr[1] == ':' && 
            (fileNameStr[2] == '/' || fileNameStr[2] == '\\')) || fileNameStr[0] == '\\')
        {
            isAbsolute = true;
        }
        #else
        // Unix/Linux/Mac 绝对路径判断（以 / 开头）
        if (fileNameStr[0] == '/')
        {
            isAbsolute = true;
        }
        #endif
        
        if (isAbsolute)
        {
            // 已经是绝对路径，直接转换为OUString
            absoluteFilePath = fileName;
            logger_log_info("absoluteFilePath (already absolute): %s", fileNameStr.c_str());
            return;
        }
        
        // 非绝对路径，拼接数据路径
        const char *datapath = json_config_get_string("datapath");
        const char *workpathname = json_config_get_string("workpathname");
        
        // 确保有默认路径值
        if (!datapath)
            datapath = "../data";
        if (!workpathname)
            workpathname = "";
        
        // 构建完整路径
        std::string filePathStr;
        if (!workpathname[0]) {
            // 如果workpathname为空，直接使用datapath
            filePathStr = std::string(datapath) + "/" + ensureOdsExtension(fileNameStr);
        } else {
            // 否则拼接完整路径
            filePathStr = std::string(datapath) + "/" + std::string(workpathname) + "/" + ensureOdsExtension(fileNameStr);
        }
        
        // 转换为绝对路径
        std::string absoluteFilePathStr = convertToAbsolutePath(filePathStr);
        
        // 转换为OUString
        rtl::OString absoluteFilePathOStr = rtl::OString(absoluteFilePathStr.c_str());
        absoluteFilePath = rtl::OStringToOUString(absoluteFilePathOStr, RTL_TEXTENCODING_UTF8);
        
        logger_log_info("absoluteFilePath (generated): %s", absoluteFilePathStr.c_str());
        // logger_log_info("absoluteFilePath: %s", rtl::OUStringToOString(absoluteFilePath, RTL_TEXTENCODING_UTF8).getStr());
    }
    void getDefaultData(rtl::OUString &defaultFilePath, rtl::OUString &defaultFilename, rtl::OUString &wordsSheetName)
    {
        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        const char *defaultpathname = json_config_get_string("defaultpathname");
        const char *defaultname = json_config_get_string("defaultname");
        const char *wordsSheetNameConfig = json_config_get_string("WordsSheet");

        if (!datapath)
            datapath = "../data"; // 默认数据路径
        if (!defaultname)
            defaultname = "default.ods"; // 默认文件名

        // 默认工作表名
        const char *defaultSheetName = wordsSheetNameConfig ? wordsSheetNameConfig : "WordsSheet";

        // 构建默认文件的完整路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultpathname) + "/";

        // 处理相对路径，转换为绝对路径
        std::string absoluteDefaultFilePathStr = convertToAbsolutePath(defaultFilePathStr);

        // 确保路径末尾有斜杠
        if (!absoluteDefaultFilePathStr.empty() && absoluteDefaultFilePathStr.back() != '/' && absoluteDefaultFilePathStr.back() != '\\') {
#ifdef _WIN32
            absoluteDefaultFilePathStr += '\\';
#else
            absoluteDefaultFilePathStr += '/';
#endif
        }

        rtl::OString absoluteDefaultFilePathOStr = rtl::OString(absoluteDefaultFilePathStr.c_str());
        defaultFilePath = rtl::OStringToOUString(absoluteDefaultFilePathOStr, RTL_TEXTENCODING_UTF8);
        defaultFilename = convertStringToOUString(ensureOdsExtension(std::string(defaultname)).c_str());
        wordsSheetName = convertStringToOUString(defaultSheetName);
        // logger_log_info("defaultFilePath: %s", rtl::OUStringToOString(defaultFilePath, RTL_TEXTENCODING_UTF8).getStr());
        // logger_log_info("wordsSheetName: %s", rtl::OUStringToOString(wordsSheetName, RTL_TEXTENCODING_UTF8).getStr());
    }
    std::string getAbsoluteString(const std::string &fileName)
    {
        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        if (!datapath)
            datapath = "../data"; // 默认数据路径
                                  // 构建默认文件的完整路径
        std::string FilePathStr = std::string(datapath) + "/" + ensureOdsExtension(fileName);
        // logger_log_info("getAbsoluteString: input fileName: %s", FilePathStr.c_str());
        return convertToAbsolutePath(FilePathStr);
    }
    // Function to convert a number to Excel column letter notation
    std::string extractFilenameWithoutODSExtension(const std::string &filepath) {
        // 查找最后一个路径分隔符
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string filename = (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
        
        // 转换为小写比较
        std::string lowerFilename = filename;
        std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        
        // 去除.ods或.ODS后缀
        if (lowerFilename.size() >= 4 && 
            lowerFilename.substr(lowerFilename.size() - 4) == ".ods") {
            filename = filename.substr(0, filename.size() - 4);
        }
        
        return filename;
    }

    std::string numberToExcelColumn(int num)
    {
        std::string columnName;
        while (num > 0)
        {
            int remainder = (num - 1) % 26;
            columnName = char('A' + remainder) + columnName;
            num = (num - 1) / 26;
        }
        return columnName;
    }

    void ExcelColumnToNumber(const std::string &posStr,int *col, int *row)
    {
        if (posStr.length() >= 2)
        {
            // 解析列名（如 "A"）和行号（如 "1"）
            size_t i = 0;
            while (i < posStr.length() && isalpha(posStr[i]))
                ++i;
            std::string colStr = posStr.substr(0, i);
            std::string rowStr = posStr.substr(i);
            // 列名转索引
            *col = 0;
            for (char c : colStr)
            {
                *col = *col * 26 + (toupper(c) - 'A' + 1);
            }
            *col -= 1;                    // 0-based
            *row = std::stoi(rowStr) - 1; // 0-based
        }
        else
        {
            *col = -1;
            *row = -1;
            return;
        }
    }
}