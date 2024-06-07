#ifndef UTIL_FILE_H
#define UTIL_FIL_H

#include <SPIFFS.h>

std::string readFile(fs::FS& fs, const char* path);

#endif
