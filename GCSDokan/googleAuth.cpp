#include "utilities.h"
#include <string>
using std::string;

bool is_gcloud = false;
bool is_json = false;
string token;
string json_path;


bool gcloud_auth(const char* email = NULL) {
	string command = "gcloud auth print-access-token";
	if (email != NULL) {
		command.append(" ");
		command.append(email);
	}
	try {
		token = execute_command(command.c_str());
	}
	catch (const std::exception& e) {
		return false;
	}
	is_gcloud = true;
	is_json = false;
	return true;
}

string& get_token() {
	return token;
}


void clear_token() {
	token.clear();
}