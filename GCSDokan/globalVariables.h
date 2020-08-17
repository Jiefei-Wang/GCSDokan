#pragma once
#include <string>

enum class CACHE_TYPE { none, disk, memory };

void set_mount_point(std::wstring path);
std::wstring& get_mount_point();

//The refresh time of the cache file
extern short verbose_level;


#ifdef WIN10_ENABLE_LONG_PATH
//dirty but should be enough
#define DIRECTORY_MAX_PATH 32768
#else
#define DIRECTORY_MAX_PATH MAX_PATH
#endif // DEBUG