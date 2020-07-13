#pragma once
#include "stdlib.h"


bool gcloud_auth(const char* email = NULL);


std::string& get_token();
void clear_token();