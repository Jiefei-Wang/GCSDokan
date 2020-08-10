#include <google/cloud/storage/client.h>
#include <string>
#include <vector>
#include <time.h>
#include "utilities.h"
#include "googleAuth.h"
#include "globalVariables.h"
using std::wstring;
using std::string;

enum class SOURCE { none, gcloud, service_account };


SOURCE authen_source = SOURCE::none;
time_t token_update_interval = 60 * 30;
time_t auth_time;
wstring gcloud_email;
wstring token;
wstring biling_project;
wstring json_path;


bool auto_auth() {
	bool success = false;
	WCHAR buffer[DIRECTORY_MAX_PATH];
	size_t read_len = GetEnvironmentVariable(L"GOOGLE_APPLICATION_CREDENTIALS", buffer, DIRECTORY_MAX_PATH);

	if (read_len != 0) {
		success = json_auth(buffer);
	}

	if (success) {
		debug_print(L"Authenticate using a service account\n");
		return true;
	}
	else {
		success = gcloud_auth(L"");
	}
	if (success) {
		debug_print(L"Authenticate using gcloud\n");
		return true;
	}
	else {
		debug_print(L"No authenticate source has been found");
		return false;
	}
}


bool gcloud_auth(const wstring email) {
	if (!is_command_exist("gcloud"))
		return false;
	string command = "gcloud auth print-access-token";
	if (email.length() != 0) {
		command.append(" ");
		command.append(wstringToString(email));
	}
	try {
		string token_char = execute_command(command.c_str());
		token = L"Bearer " + wstring(token_char.begin(), token_char.end());
	}
	catch (const std::exception) {
		return false;
	}
	gcloud_email = email;
	time(&auth_time);
	authen_source = SOURCE::gcloud;
	return true;
}

bool json_auth(std::wstring path) {
	namespace gcs = google::cloud::storage;
	std::set< std::string > scope;
	scope.insert("https://www.googleapis.com/auth/devstorage.read_only");

	auto credentials = gcs::oauth2::CreateServiceAccountCredentialsFromFilePath(
		wstringToString(path),
		google::cloud::optional<std::set< std::string >> {scope},
		google::cloud::optional <string>());
	if (credentials) {
		auto header = credentials.value()->AuthorizationHeader();
		if (header) {
			wstring value = stringToWstring(header.value());
			size_t offset = value.find(L"Bearer", 0);
			token = value.substr(offset);
			json_path = path;
			time(&auth_time);
			authen_source = SOURCE::service_account;
			return true;
		}
	}
	return false;
}
wstring& get_token() {
	if (is_token_outdated()) {
		refresh_token(); 
	}
	return token;
}
bool refresh_token() {
	if (authen_source == SOURCE::gcloud) {
		return gcloud_auth(gcloud_email);
	}
	if (authen_source == SOURCE::service_account) {
		return json_auth(json_path);
	}
	return false;
}

bool is_token_outdated() {
	time_t current_time;
	time(&current_time);
	return current_time - auth_time > token_update_interval;
}

void clear_token() {
	token.clear();
}


std::wstring& get_requester_pay() {
	return biling_project;
}

void set_requester_pay(std::wstring project) {
	biling_project = project;
}



