#pragma once
#include <cpprest/http_client.h>
#include <string>
#include <vector>
#include "dataTypes.h"
#include "utilities.h"

extern std::wstring remote_link;


std::string get_last_error();
void set_last_error(std::string error);

#define retry_when_error(func)\
int count = 0;\
for(int count = 0;count < retry_time - 1;++count){\
auto result = func;\
if (get_last_error().length() == 0) {\
	return result;\
}\
}\
return func;

#define handle_rest_error(e)\
auto error = e.what();\
error_print(L"Error exception:%s\n", stringToWstring(error).c_str());\
set_last_error(error);



void print_json(web::json::value json_value, int deep = 0);
std::wstring tailing_string(std::wstring source, size_t offset);
decomposed_path get_path_info(std::wstring win_path);


std::vector<std::wstring> get_json_array(web::json::value json_value,
    std::initializer_list <const utility::char_t*> indices,
    const utility::char_t * indices2 = nullptr);

