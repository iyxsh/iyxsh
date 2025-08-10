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
    std::string removeFileExtension(const std::string& filename) {
        // 查找最后一个点的位置
        size_t lastDot = filename.find_last_of('.');
        
        // 如果没有找到点，或者点在第一个位置，或者点在最后一个位置，则返回原文件名
        if (lastDot == std::string::npos || lastDot == 0 || lastDot == filename.length() - 1) {
            return filename;
        }
        
        // 返回不包含后缀的文件名
        return filename.substr(0, lastDot);
    }
    
    // 确保文件名有.xlsx后缀
    std::string ensureXlsxExtension(const std::string& filename) {
        // 如果文件名已经以.xlsx结尾，则直接返回
        if (filename.length() >= 5 && 
            (filename.substr(filename.length() - 5) == ".xlsx" || 
             filename.substr(filename.length() - 5) == ".XLSX")) {
            return filename;
        }
        
        // 如果文件名以.xls结尾，则替换为.xlsx
        if (filename.length() >= 4 && 
            (filename.substr(filename.length() - 4) == ".xls" || 
             filename.substr(filename.length() - 4) == ".XLS")) {
            return filename.substr(0, filename.length() - 4) + ".xlsx";
        }
        
        // 其他情况添加.xlsx后缀
        return filename + ".xlsx";
    }
    
    // 实现工具函数
    std::string convertToAbsolutePath(const std::string &path)
    {
        std::string absolutePath = path;
        std::cerr << "convertToAbsolutePath: input path: " << path << std::endl;

        // 检查是否为相对路径（以../开头）
        if (path.length() >= 2 && path.substr(0, 2) == "..")
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                std::string cwdStr(cwd);
                std::cerr << "convertToAbsolutePath: current working directory: " << cwdStr << std::endl;

                // 查找最后一个斜杠以获取父目录
                size_t lastSlash = cwdStr.find_last_of('/');
                if (lastSlash != std::string::npos)
                {
                    std::string parentDir = cwdStr.substr(0, lastSlash);
                    absolutePath = parentDir + path.substr(2); // 去掉前面的".."
                    std::cerr << "convertToAbsolutePath: converted to absolute path: " << absolutePath << std::endl;
                }
                else
                {
                    std::cerr << "convertToAbsolutePath: could not find parent directory" << std::endl;
                }
            }
            else
            {
                std::cerr << "convertToAbsolutePath: failed to get current working directory" << std::endl;
            }
        }
        else
        {
            std::cerr << "convertToAbsolutePath: path is not relative to parent, no conversion needed" << std::endl;
        }

        return absolutePath;
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
        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        if (!datapath)
            datapath = "../data"; // 默认数据路径
                                  // 构建默认文件的完整路径
        std::string FilePathStr = std::string(datapath) + "/" + rtl::OUStringToOString(fileName, RTL_TEXTENCODING_UTF8).getStr();

        // 处理相对路径，转换为绝对路径
        std::string absoluteFilePathStr = convertToAbsolutePath(FilePathStr);

        rtl::OString absoluteFilePathOStr = rtl::OString(absoluteFilePathStr.c_str());
        absoluteFilePath = rtl::OStringToOUString(absoluteFilePathOStr, RTL_TEXTENCODING_UTF8);
        logger_log_info("absoluteFilePath: %s", rtl::OUStringToOString(absoluteFilePath, RTL_TEXTENCODING_UTF8).getStr());
    }
    void getDefaultData(rtl::OUString &defaultFilePath, rtl::OUString &wordsSheetName)
    {
        // 从配置文件中获取数据路径和默认文件名
        const char *datapath = json_config_get_string("datapath");
        const char *defaultname = json_config_get_string("defaultname");
        const char *wordsSheetNameConfig = json_config_get_string("WordsSheet");

        if (!datapath)
            datapath = "../data"; // 默认数据路径
        if (!defaultname)
            defaultname = "default.xlsx"; // 默认文件名

        // 默认工作表名
        const char *defaultSheetName = wordsSheetNameConfig ? wordsSheetNameConfig : "WordsSheet";

        // 构建默认文件的完整路径
        std::string defaultFilePathStr = std::string(datapath) + "/" + std::string(defaultname);

        // 处理相对路径，转换为绝对路径
        std::string absoluteDefaultFilePathStr = convertToAbsolutePath(defaultFilePathStr);

        rtl::OString absoluteDefaultFilePathOStr = rtl::OString(absoluteDefaultFilePathStr.c_str());
        defaultFilePath = rtl::OStringToOUString(absoluteDefaultFilePathOStr, RTL_TEXTENCODING_UTF8);
        wordsSheetName = rtl::OUString::createFromAscii(defaultSheetName);
        logger_log_info("defaultFilePath: %s", rtl::OUStringToOString(defaultFilePath, RTL_TEXTENCODING_UTF8).getStr());
        logger_log_info("wordsSheetName: %s", rtl::OUStringToOString(wordsSheetName, RTL_TEXTENCODING_UTF8).getStr());
    }
}