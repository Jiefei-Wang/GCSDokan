#pragma once
#include <string>


bool gcloud_auth(const char* email = NULL);


std::wstring& get_token();
void clear_token();

std::wstring& get_requester_pay();
void set_requester_pay(std::wstring);






