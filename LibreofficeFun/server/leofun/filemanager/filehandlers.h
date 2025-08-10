#ifndef FILEHANDLERS_H
#define FILEHANDLERS_H

#include <string>
// cJSON
#include "../cJSON/cJSON.h"
#include "utils.h"

namespace filemanager
{
    int createfile(std::string filename);
    int fileupdate(cJSON *root);
    void findInSheet(cJSON *results, const char *body);
    void readSheetContents(cJSON *results, const char *body);
    int worksheetAdd(cJSON *taskData);
    int worksheetRemove(cJSON *taskData);
} // namespace filemanager

#endif // FILEHANDLERS_H