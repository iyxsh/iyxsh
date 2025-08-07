#ifndef FILEHANDLERS_H
#define FILEHANDLERS_H

#include <string>
// cJSON
#include "../cJSON/cJSON.h"
#include "utils.h"

namespace filemanager
{
    int createfile(std::string& filename);
    int updatefile(cJSON *root);
    void findInSheet(cJSON *results, const char *body);
    void readSheetContents(cJSON *results, const char *body);
} // namespace filemanager

#endif // FILEHANDLERS_H