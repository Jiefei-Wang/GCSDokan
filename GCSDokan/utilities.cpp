#include "utilities.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

using std::string;
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
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
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