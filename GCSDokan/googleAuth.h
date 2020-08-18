#pragma once
#include <string>
#include <google/cloud/storage/client.h>

void auto_auth();
bool auth_by_file(std::string path);
bool auth_by_google_default();
bool auth_anonymous();

void set_billing_project(std::string);
std::string get_billing_project();

void set_credentials_path(std::string);
std::string get_credentials_path();


google::cloud::storage::v1::Client& get_client();
google::cloud::storage::v1::Client& refresh_client();

google::cloud::storage::v1::oauth2::Credentials& get_credentials();


