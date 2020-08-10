#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include "utilities.h"
#include "globalVariables.h"

using std::string;
using std::wstring;
using std::vector;


#define EXECUTE_COMMAND_FAIL_TO_OPEN_PIPE "Excuting command line program failed!"
#define EXECUTE_COMMAND_NONZERO_EXIST "Command has non-zero exist status"

std::string execute_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = _popen(cmd, "r");
    //std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        _pclose(pipe);
        throw std::runtime_error(EXECUTE_COMMAND_FAIL_TO_OPEN_PIPE);
    }
    while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int status = _pclose(pipe);
    if (status != 0) {
        throw std::runtime_error(EXECUTE_COMMAND_NONZERO_EXIST);
    }
    return result;
}



bool is_command_exist(const char* command) {
    string checkCommand = string("where ")+ command + " 2>&1";
    try {
        execute_command(checkCommand.c_str());
    }
    catch (const std::exception& e) {
        if (strcmp(e.what(), EXECUTE_COMMAND_NONZERO_EXIST) == 0)
            return false;
        else
            throw;
    }
    return true;
}


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

std::wstring get_file_name_in_path(std::wstring path) {
    return path.substr(path.find_last_of(L"\\/") + 1);
}

std::wstring build_path(std::wstring path1, std::wstring path2, WCHAR delimiter) {
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



void debug_print(LPCWSTR format, ...) {
    if (verbose_level>0) {
        va_list args;
        int len;
        wchar_t* buffer;
        va_start(args, format);
        len = _vscwprintf(format, args) + 1;
        buffer = (wchar_t*)_malloca(len * sizeof(WCHAR));
        vswprintf_s(buffer, len, format, args);
        wprintf(buffer);
        _freea(buffer);
    }
}

void error_print(LPCWSTR format, ...) {
    if (verbose_level > 0) {
        va_list args;
        int len;
        wchar_t* buffer;
        va_start(args, format);
        len = _vscwprintf(format, args) + 1;
        buffer = (wchar_t*)_malloca(len * sizeof(WCHAR));
        vswprintf_s(buffer, len, format, args);
        wprintf(buffer);
        _freea(buffer);
    }
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


#include <regex>
FILETIME json_time_to_file_time(wstring time)
{
    using namespace std;
    string time_stamp = wstringToString(time.c_str());
    regex re("(\\d+)-(\\d+)-(\\d+)T(\\d+):(\\d+):(\\d+).(\\d+)");
    smatch match;
    if (regex_search(time_stamp, match, re) == true && match.size() == 8) {
        FILETIME ft;
        SYSTEMTIME st;
        st.wYear = stoi(match.str(1));
        st.wMonth = stoi(match.str(2));
        st.wDay = stoi(match.str(3));
        st.wHour = stoi(match.str(4));
        st.wMinute = stoi(match.str(5));
        st.wSecond = stoi(match.str(6));
        st.wMilliseconds = stoi(match.str(7));
        bool res = SystemTimeToFileTime(&st, &ft);
        if (res) {
            return ft;
        }
    }
    return FILETIME{0,0};
}


#include "cpprest/streams.h"
std::string to_base64(void* data, size_t size) {
    string result;
    for (int i = 0; i < size/8; i++) {
        result = result + wstringToString(utility::conversions::to_base64(((uint64_t*)data)[i]));
    }
    return result;
}