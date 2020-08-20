#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <boost/filesystem.hpp>
#include "utilities.h"
#include "globalVariables.h"



using std::string;
using std::wstring;
using std::vector;


wstring to_linux_slash(wstring path) {
    for (size_t i = 0; i < path.size(); ++i) {
        if (path.at(i) == L'\\') {
            path.replace(i, 1, L"/");
        }
    }
    return path;
}
wstring to_win_slash(wstring path) {
    for (size_t i = 0; i < path.size(); ++i) {
        if (path.at(i) == L'/') {
            path.replace(i, 1, L"\\");
        }
    }
    return path;
}

//Work on both win and linux slash
wstring to_parent_folder(wstring path) {
    wstring parent_path = path.substr(0, path.find_last_of(L"\\/"));
    return parent_path;
}
string to_parent_folder(string path) {
    string parent_path = path.substr(0, path.find_last_of("\\/"));
    return parent_path;
}

std::wstring get_file_name_in_path(std::wstring path) {
    return path.substr(path.find_last_of(L"\\/") + 1);
}


string get_tailing_string(string source, size_t offset) {
    if (offset < source.length()) {
        return source.substr(offset);
    }
    else {
        return "";
    }
}
string remove_tailing_slash(string path) {
    return remove_tailing_symbol(path, '/');
}
wstring remove_tailing_slash(wstring path) {
    return remove_tailing_symbol(path, L'/');
}

decomposed_path get_path_info(std::wstring wide_linux_path) {
    string linux_path = wstringToString(wide_linux_path);
    vector<string> full_path_vec;
    for (auto& part : boost::filesystem::path(linux_path)) {
        if (part.string() != ".")
            full_path_vec.push_back(part.string());
    }
    string bucket = full_path_vec.at(0);
    string name = "";
    if (full_path_vec.size() > 1) {
        name = full_path_vec.at(full_path_vec.size() - 1);
    }
    string path = get_tailing_string(linux_path, bucket.length() + 1);
    string parent_path = "";
    for (int i = 1; i < full_path_vec.size() - 1; i++) {
        parent_path = build_path(parent_path, full_path_vec[i]);
    }
    return decomposed_path{ bucket, name, path, parent_path, full_path_vec };
}

std::string build_path(std::string path1, std::string path2, char delimiter) {
    wstring result = build_path(stringToWstring(path1), stringToWstring(path2), (WCHAR)delimiter);
    return wstringToString(result);
}

std::wstring build_path(std::wstring path1, std::wstring path2, WCHAR delimiter) {
    if (path1.length() == 0) {
        return path2;
    }
    if (path2.length() == 0) {
        return path1;
    }
    if (path1.at(path1.length() - 1) == delimiter &&
        path2.at(0) == delimiter) {
        path2 = path2.erase(0);
    }
    if (path1.length()==0||
        path2.length()==0||
        path1.at(path1.length() - 1) == delimiter||
        path2.at(0) == delimiter){
        return path1 + path2;
    }
    else {
        return path1 + delimiter + path2;
    }
}



static char buffer[1024 * 1024];
void debug_print(LPCWSTR format, ...) {
    if (verbose_level>0) {
        va_list args;
        int len;
        va_start(args, format);
        len = _vscwprintf(format, args) + 1;
        vswprintf_s((WCHAR*)buffer, len, format, args);
        fwprintf(stdout, (WCHAR*)buffer);
    }
}

void error_print(LPCWSTR format, ...) {
        va_list args;
        int len;
        va_start(args, format);
        len = _vscwprintf(format, args) + 1;
        vswprintf_s((WCHAR*)buffer, len, format, args);
        fwprintf(stderr, (WCHAR*)buffer);
}

void debug_print(LPCSTR format, ...) {
    if (verbose_level > 0) {
        va_list args;
        int len;
        va_start(args, format);
        len = _vscprintf(format, args) + 1;
        vsprintf_s(buffer, len, format, args);
        printf(buffer);
    }
}

void error_print(LPCSTR format, ...) {
        va_list args;
        int len;
        va_start(args, format);
        len = _vscprintf(format, args) + 1;
        vsprintf_s(buffer, len, format, args);
        printf(buffer);
}



bool endsWith(std::wstring str, std::wstring suffix)
{
    return str.find(suffix, str.length() - suffix.length()) != wstring::npos;
}


#include <codecvt>
#include <locale> 
std::wstring stringToWstring(std::string utf8Bytes)
{
    return stringToWstring(utf8Bytes.c_str());
}
std::string wstringToString(std::wstring utf16Bytes)
{
    return wstringToString(utf16Bytes.c_str());
}

std::wstring stringToWstring(const char* utf8Bytes)
{
    //setup converter
    using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
    std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    return converter.from_bytes(utf8Bytes);
}
std::string wstringToString(const WCHAR* utf16Bytes)
{
    //setup converter
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    return convert.to_bytes(utf16Bytes);
}

FILETIME time_point_to_file_time(std::chrono::system_clock::time_point time) {
    FILETIME fileTime = { 0 };
    long long timePointTmp = std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count() * 10 + 116444736000000000;
    fileTime.dwLowDateTime = (unsigned long)timePointTmp;
    fileTime.dwHighDateTime = timePointTmp >> 32;
    return fileTime;
}


#include "cpprest/streams.h"
std::string to_base64(void* data, size_t size) {
    string result;
    for (int i = 0; i < size/8; i++) {
        result = result + wstringToString(utility::conversions::to_base64(((uint64_t*)data)[i]));
    }
    return result;
}