#pragma once
#include <string>
#include "dokan.h"

extern short verbose_level;

//CMD related function
std::string execute_command(const char* cmd);
bool is_command_exist(const char* command);


std::wstring to_linux_slash(std::wstring path);
std::wstring  to_win_slash(std::wstring path);
std::wstring to_parent_folder(std::wstring path);
std::wstring get_file_name_in_path(std::wstring path);
std::wstring build_path(std::wstring path1, std::wstring path2, WCHAR delimiter = L'/');

void debug_print(LPCWSTR format, ...);
void error_print(LPCWSTR format, ...);
std::wstring stringToWstring(const char* utf8Bytes);
std::string wstringToString(const WCHAR* utf16Bytes);
FILETIME json_time_to_file_time(std::wstring time);


bool endsWith(std::wstring str, std::wstring suffix);

//Splint a long long type variable to its low and high order parts
static inline void LlongToDwLowHigh(const LONGLONG& value, DWORD& low,
    DWORD& hight) {
    hight = value >> 32;
    low = static_cast<DWORD>(value);
}

