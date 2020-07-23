#include "utilities.h"
#include <string>
using std::wstring;
using std::string;

bool is_gcloud = false;
bool is_json = false;
wstring token;
wstring json_path;
wstring biling_project;

bool gcloud_auth(const char* email = NULL) {
	string command = "gcloud auth print-access-token";
	if (email != NULL) {
		command.append(" ");
		command.append(email);
	}
	try {
		string token_char = execute_command(command.c_str());
		token = L"Bearer " + wstring(token_char.begin(), token_char.end());
	}
	catch (const std::exception& e) {
		return false;
	}
	is_gcloud = true;
	is_json = false;
	return true;
}

wstring& get_token() {
	return token;
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
