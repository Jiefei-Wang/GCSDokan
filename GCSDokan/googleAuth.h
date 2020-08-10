#pragma once
#include <string>


bool auto_auth();
bool gcloud_auth(std::wstring email);
bool json_auth(std::wstring path);

bool refresh_token();
bool is_token_outdated();

std::wstring& get_token();
void clear_token();

std::wstring& get_requester_pay();
void set_requester_pay(std::wstring);






