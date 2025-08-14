#include "error_codes.h"

const std::unordered_map<int, std::string> ErrorCodeManager::errorMessages = {
    {200, "OK"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {415, "Unsupported Media Type"},
    {1000, "Success"},
    {1001, "Invalid input parameters"},
    {1002, "File not found"},
    {1003, "Failed to process document"},
    {1004, "LibreOffice connection failed"},
    {1005, "Unknown error"},
    {1006, "File already exists"},
    {1007, "File already opened"},
    {1008, "File already closed"},
    {1009, "File already saved"},
    {1010, "File is already opened for edit"},
    {1011, "File can't be opened"},
    {1012, "Failed to convert file"},
    {1013, "Failed to start the process"},
    {1014, "File is not a spreadsheet"},
    {1015, "File is not a document"},
    {1016, "File is not a presentation"},
    {1017, "File is not a text file"},
    {1018, "File is not a valid format"},
    {1019, "File name is empty"},
    {1020, "File name contains invalid characters"},
    {1021, "File name is too long"},
    {1022, "File name is too short"},
    {1023, "File size exceeds limit"},
    {1024, "Invalid file format"},
    {1025, "Invalid file content"},
    {1026, "Invalid file path"},
    {1027, "Failed to read file content"},
    {1028, "Failed to write file content"},
    {1029, "File operation failed"},
    {1030, "File is too large"},
    {1031, "File is too small"},
    {1032, "File transfer failed"},
    {1033, "File encoding failed"},
    {1034, "File decompression failed"},
    {1035, "File compression failed"},
    {1036, "File parsing failed"},
    {1037, "File validation failed"},
    {1038, "File upload failed"},
    {1039, "File download failed"},
    {1040, "File deletion failed"},
    {1041, "File renaming failed"},
    {1042, "File copying failed"},
    {1043, "File moving failed"},
    {1044, "File sharing failed"},
    {1045, "File path not found"}
};

std::string ErrorCodeManager::getErrorMessage(int errorCode) {
    auto it = errorMessages.find(errorCode);
    if (it != errorMessages.end()) {
        return it->second;
    }
    return "Error code not found";
}

void ErrorCodeManager::setErrorMessage(cJSON *setToJson, int errorCode) {
    std::string errorMessage = getErrorMessage(errorCode);
    cJSON_AddNumberToObject(setToJson, "errorCode", errorCode);
    cJSON_AddStringToObject(setToJson, "errorMessage", errorMessage.c_str());
}