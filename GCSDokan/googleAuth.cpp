#include <google/cloud/storage/client.h>
#include <string>
#include <memory>
#include "googleAuth.h"
#include "utilities.h"
using std::string;
using std::shared_ptr;
namespace gcs = google::cloud::storage::v1;

shared_ptr<gcs::oauth2::Credentials> credentials;
string credentials_path;
string biling_project;
gcs::Client* client = nullptr;


void auto_auth() {
	bool success = false;
	if (credentials_path.length() != 0) {
		success = auth_by_file(credentials_path);
	}
	if (!success) {
		success = auth_by_google_default();
	}
	if (!success) {
		auth_anonymous();
	}
}

bool auth_by_google_default() {
	auto status_or_credentials = gcs::oauth2::CreateServiceAccountCredentialsFromDefaultPaths();
	if (status_or_credentials) {
		credentials = status_or_credentials.value();
		return true;
	}
	else {
		return false;
	}
}

bool auth_anonymous()
{
	credentials = gcs::oauth2::CreateAnonymousCredentials();
	return true;
}


bool auth_by_file(std::string path) {
	std::set< std::string > scope;
	scope.insert("https://www.googleapis.com/auth/devstorage.read_only");

	auto status_or_credentials = gcs::oauth2::CreateServiceAccountCredentialsFromFilePath(
		path,
		google::cloud::optional<std::set< std::string >> {scope},
		google::cloud::optional <string>());
	if (status_or_credentials) {
		credentials = status_or_credentials.value();
		return true;
	}
	return false;
}

void set_billing_project(std::string project) {
	biling_project = project;
}
std::string get_billing_project() {
	return biling_project;
}


void set_credentials_path(std::string path) {
	credentials_path = path;
}

std::string get_credentials_path() {
	return credentials_path;
}


gcs::Client& get_client() {
	if (client == nullptr) {
		return refresh_client();
	}
	else {
		return *client;
	}
}

gcs::Client& refresh_client() {
	if (client != nullptr) {
		delete client;
	}
	client = DBG_NEW gcs::Client(credentials);
	return *client;
}


