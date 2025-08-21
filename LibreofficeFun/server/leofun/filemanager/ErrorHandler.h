#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <string>
#include "../logger/logger.h"

namespace filemanager {

class ErrorHandler {
public:
    // 处理UNO异常
    static void handleUnoException(const com::sun::star::uno::Exception& e, 
                                 const std::string& context = "");
    
    // 处理标准异常
    static void handleStdException(const std::exception& e, 
                                 const std::string& context = "");
    
    // 处理未知异常
    static void handleUnknownException(const std::string& context = "");
    
    // 检查文档有效性
    template<typename T>
    static bool checkDocumentValidity(const com::sun::star::uno::Reference<T>& doc, 
                                     const std::string& docName = "document");
};

} // namespace filemanager

#endif // ERRORHANDLER_H