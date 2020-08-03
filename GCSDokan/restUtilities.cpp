#include "restUtilities.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using std::wstring;
using std::vector;


std::string last_error;

std::string get_last_error() {
    auto return_value = last_error;
    last_error = "";
    return return_value;
}

void set_last_error(std::string error) {
    last_error = error;
}




void print_json(json::value json_value, int deep) {
    wchar_t* space = (wchar_t*)malloc((deep + 1) * sizeof(wchar_t));
    for (int i = 0; i < deep; i++) {
        space[i] = L'\t';
    }
    space[deep] = L'\0';
    if (json_value.is_object()) {
        auto json_object = json_value.as_object();
        for (auto i : json_object) {
            wprintf(L"%s%s%s\n", space, L"object :", i.first.c_str());
            print_json(i.second, deep + 1);
        }
    }
    else if (json_value.is_array()) {
        auto json_array = json_value.as_array();
        for (auto i : json_array) {
            wprintf(L"%s%s\n", space, L"array :");
            print_json(i, deep + 1);
        }
    }
    else if (json_value.is_boolean()) {
        wprintf(L"%s%s\n", space, json_value.as_bool() ? L"TRUE" : L"FALSE");
    }
    else if (json_value.is_integer()) {
        wprintf(L"%s%d\n", space, json_value.as_integer());
    }
    else if (json_value.is_double()) {
        wprintf(L"%s%f\n", space, json_value.as_double());
    }
    else if (json_value.is_string()) {
        wprintf(L"%s%s\n", space, json_value.as_string().c_str());
    }
    else {
        wprintf(L"%sUnknown\n", space);
    }
    free(space);
}

wstring tailing_string(wstring source, size_t offset) {
    if (offset < source.length()) {
        return source.substr(offset);
    }
    else {
        return L"";
    }
}

decomposed_path get_path_info(std::wstring linux_path) {
    vector<string_t> full_path_vec = web::uri::split_path(linux_path);
    string_t bucket = full_path_vec.at(0);
    string_t name = L"";
    if (full_path_vec.size() > 1) {
        name = full_path_vec.at(full_path_vec.size() - 1);
    }
    string_t path = tailing_string(linux_path, bucket.length() + 1);
    wstring parent_path = to_parent_folder(linux_path);
    if (parent_path == L"") {
        parent_path = L"/";
    }
    return decomposed_path{bucket, name, path, parent_path, full_path_vec};
}




std::vector<wstring> get_json_array(web::json::value json_value,
    std::initializer_list <const utility::char_t*> indices,
    const utility::char_t* indices2) {
    try {
        for (auto i : indices) {
            json_value = json_value.at(i);
        }
        web::json::array json_array = json_value.as_array();
        std::vector<std::wstring> json_vector;
        json_vector.reserve(json_array.size());
        for (auto i : json_array) {
            if (indices2 != nullptr) {
                json_vector.push_back(i.at(indices2).as_string());
            }
            else {
                json_vector.push_back(i.as_string());
            }
        }
        return json_vector;
    }
    catch (...) {
    }
    return std::vector<wstring>();
}


