#include "utilities.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

using std::string;
using std::wstring;

short verbose_level = 1;

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