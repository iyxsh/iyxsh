#include "ErrorHandler.h"
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
namespace filemanager {

void ErrorHandler::handleUnoException(const com::sun::star::uno::Exception& e, 
                                    const std::string& context) {
    std::string errorMsg = "UNO Exception";
    if (!context.empty()) {
        errorMsg += " in " + context;
    }
    errorMsg += ": " + std::string(rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
    logger_log_error("%s", errorMsg.c_str());
}

void ErrorHandler::handleStdException(const std::exception& e, 
                                     const std::string& context) {
    std::string errorMsg = "STD Exception";
    if (!context.empty()) {
        errorMsg += " in " + context;
    }
    errorMsg += ": " + std::string(e.what());
    logger_log_error("%s", errorMsg.c_str());
}

void ErrorHandler::handleUnknownException(const std::string& context) {
    std::string errorMsg = "Unknown Exception";
    if (!context.empty()) {
        errorMsg += " in " + context;
    }
    logger_log_error("%s", errorMsg.c_str());
}

template<typename T>
bool ErrorHandler::checkDocumentValidity(const com::sun::star::uno::Reference<T>& doc, 
                                        const std::string& docName) {
    if (!doc.is()) {
        logger_log_error("Invalid %s reference", docName.c_str());
        return false;
    }
    return true;
}

// 显式实例化常用模板
template bool ErrorHandler::checkDocumentValidity<com::sun::star::uno::XInterface>(
    const com::sun::star::uno::Reference<com::sun::star::uno::XInterface>&, 
    const std::string&);

template bool ErrorHandler::checkDocumentValidity<com::sun::star::sheet::XSpreadsheetDocument>(
    const com::sun::star::uno::Reference<com::sun::star::sheet::XSpreadsheetDocument>&, 
    const std::string&);

} // namespace filemanager