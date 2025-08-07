#ifndef FILEOPS_H
#define FILEOPS_H

#include "../cJSON/cJSON.h"

namespace filemanager 
{
    void newfileCreate(cJSON *results, const char *body);
    void updatefile(cJSON *results, const char *body);
    void editfile(cJSON *results, const char *body);
    void filestatus(cJSON *results, const char *body);
} // namespace filemanager

#endif // FILEOPS_H