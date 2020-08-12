#pragma once
#include <string>
#include "dataTypes.h"

void set_mount_point(std::wstring path);
void set_remote_link(std::wstring link);
extern std::wstring remote_link;
extern std::wstring mount_point;

extern std::wstring cache_root;
extern CACHE_TYPE cache_type;
extern size_t memory_cache_block_number;

//The refresh time of the cache file
extern size_t update_interval;

extern short verbose_level;


#ifdef WIN10_ENABLE_LONG_PATH
//dirty but should be enough
#define DIRECTORY_MAX_PATH 32768
#else
#define DIRECTORY_MAX_PATH MAX_PATH
#endif // DEBUG