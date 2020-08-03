#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <vector>
#include <algorithm>
#include "googleAuth.h"
#include "restUtilities.h"
#include "restAPIs.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using std::vector;
using std::wstring;


#define delimiter L"/"
#define json_uri_version "v1"
#define json_base_uri (L"https://storage.googleapis.com/storage/" json_uri_version "/")
#define xml_base_uri (L"https://storage.googleapis.com/")


/*
The number of times that a function will be called
when a network error occurs
*/
int retry_time = 3;

http_client_config* client_config = nullptr;
http_client* json_base_client = nullptr;
http_client* xml_base_client = nullptr;

void initial_client() {
	if (client_config == nullptr) {
		client_config = new http_client_config();
		client_config->set_timeout(std::chrono::seconds(10));
		json_base_client = new http_client(json_base_uri, *client_config);
		xml_base_client = new http_client(xml_base_uri, *client_config);
	}
}

uri_builder get_builder(std::initializer_list<const wchar_t*> relative_uri) {
	initial_client();
	string_t uri;
	for (auto i : relative_uri) {
		auto res = uri::encode_data_string(i);
		uri = uri + uri::encode_data_string(i) + L"/";
	}
	// Remove the trailing slash
	if (uri.length() > 0) {
		uri = uri.substr(0, uri.length() - 1);
	}
	uri_builder builder(uri);
	return builder;
}


http_request get_request(method mtd, uri_builder builder, bool add_token = true, bool add_billing_project = true) {
	http_request request = http_request(mtd);
	request.set_request_uri(builder.to_string());
	http_headers& headers = request.headers();
	if (add_token && get_token().length() != 0)
		headers.add(L"Authorization", get_token());
	if (add_billing_project && get_requester_pay().length() != 0) {
		headers.add(L"userProject", get_requester_pay());
	}
	return request;
}


file_meta_info compose_file_meta(
	wstring linux_file_path,
	wstring name,
	wstring size,
	wstring crc32c,
	wstring created,
	wstring updated,
	wstring accessed
) {
	file_meta_info info;
	info.local_full_path = linux_file_path;
	info.remote_full_path = linux_file_path;
	info.local_name = name;
	info.remote_name = name;
	info.size = std::stoll(size);
	info.crc32c = crc32c;
	info.time_created = json_time_to_file_time(created);
	info.time_updated = json_time_to_file_time(updated);
	info.time_accessed = json_time_to_file_time(accessed);
	return info;
}

void solve_file_name_conflict(wstring linux_parent_path, file_meta_info& file_info, vector<wstring> folder_names) {
	bool has_conflict = false;
	while (std::find(folder_names.begin(), folder_names.end(), file_info.local_name) != folder_names.end()) {
		file_info.local_name = file_info.local_name + L"_";
		has_conflict = true;
	}
	if (has_conflict) {
		wstring file_path = build_path(linux_parent_path, file_info.local_name);
		file_info.local_full_path = file_path;
	}
}


folder_meta_info gcs_get_folder_meta_internal(std::wstring linux_path, wstring next_page_token = L"") {
	decomposed_path path_info = get_path_info(linux_path);
	folder_meta_info dir_info;
	dir_info.local_full_path = linux_path;
	dir_info.local_name = path_info.name;
	uri_builder builder = get_builder({ L"b", path_info.bucket.c_str() ,L"o" });
	builder.append_query(L"delimiter", delimiter);
	builder.append_query(L"pageToken", next_page_token);
	if (path_info.path.length() > 0) {
		builder.append_query(L"prefix", path_info.path + L"/");
	}

	string_t final_uri = builder.to_string();
	http_request request = get_request(methods::GET, builder);

	auto task = json_base_client->request(request).then([=, &dir_info,&next_page_token](http_response response)
	{
		auto json_table = response.extract_json().get();
		if (response.status_code() != 200) {
			error_print(L"%s",json_table.at(L"error").at(L"message").as_string().c_str());
			return;
		}
		//print_json(json_table);
		vector<wstring> folder_names = get_json_array(json_table, { L"prefixes" });
		vector<wstring> file_names = get_json_array(json_table, { L"items" }, L"name");
		vector<wstring> file_sizes = get_json_array(json_table, { L"items" }, L"size");
		vector<wstring> file_crc32c = get_json_array(json_table, { L"items" }, L"crc32c");
		vector<wstring> file_created = get_json_array(json_table, { L"items" }, L"timeCreated");
		vector<wstring> file_updated = get_json_array(json_table, { L"items" }, L"updated");

		// Check if the result is not complete
		if (json_table.has_field(L"nextPageToken")) {
			next_page_token = json_table.at(L"nextPageToken").as_string();
		}
		else {
			next_page_token = L"";
		}

		//Remove the empty path
		auto idx = std::remove_if(folder_names.begin(), folder_names.end(), [=](wstring const& x) {
			return x == path_info.path + L"/";
		});
		folder_names.erase(idx, folder_names.end());

		// Get purely folders and files name, not an relative path
		size_t start_offset;
		if (path_info.path.length() != 0) {
			start_offset = path_info.path.length() + 1;
		}
		else {
			start_offset = 0;
		}
		for (auto& i : folder_names) {
			i = i.substr(start_offset, i.length() - 1 - start_offset);
		}
		for (auto& i : file_names) {
			i = i.substr(start_offset);
		}

		// fill the result to the folder_meta_info object
		for (int i = 0; i < file_names.size(); i++) {
			wstring file_path = build_path(linux_path, file_names[i]);
			file_meta_info file_meta = compose_file_meta(
				file_path, file_names[i], file_sizes[i], file_crc32c[i], 
				file_created[i], file_updated[i], file_updated[i]
			);
			//Add postfix if the file name has a conflict with the folder names
			solve_file_name_conflict(linux_path, file_meta, folder_names);
			dir_info.file_meta_vector.insert(std::pair<wstring,file_meta_info>(file_meta.local_name, file_meta));
		}
		dir_info.folder_names = folder_names;

		// If the result is not complete, send another request and get the result back
		if (next_page_token.length() != 0) {
			auto next_dir_info = gcs_get_folder_meta_internal(linux_path, next_page_token);
			dir_info.file_meta_vector.insert(
				next_dir_info.file_meta_vector.begin(), next_dir_info.file_meta_vector.end());

			dir_info.folder_names.insert(dir_info.folder_names.end(),
				next_dir_info.folder_names.begin(), next_dir_info.folder_names.end());
		}
	}
	);

	try
	{
		task.wait();
		return dir_info;
	}
	catch (const std::exception& e)
	{
		handle_rest_error(e);
	}
	return folder_meta_info{};
}




bool gcs_read_file_internal(std::wstring win_path, void* buffer,
	size_t offset,
	size_t length) {
	decomposed_path info = get_path_info(win_path);
	uri_builder builder = get_builder({ info.bucket.c_str() ,info.path.c_str() });
	string_t final_uri = builder.to_string();
	http_request request = get_request(methods::GET, builder, true, false);
	request.headers().add(L"range",
		L"bytes=" + std::to_wstring(offset) + L"-" + std::to_wstring(offset + length - 1));
	if (get_requester_pay().length() != 0)
		request.headers().add(L"x-goog-user-project", get_requester_pay());

	pplx::task<bool> task = xml_base_client->request(request).then([=](http_response response)
	{
		try {
			if (response.status_code() != 200 && response.status_code() != 206) {
				auto json_table = response.extract_json().get();
				error_print(L"%s", json_table.at(L"error").at(L"message").as_string().c_str());
				return false;
			}
			std::vector<unsigned char> response_vec = response.extract_vector().get();
			for (int i = 0; i < response_vec.size(); ++i) {
				((unsigned char*)buffer)[i] = response_vec.at(i);
			}
		}
		catch (...) {
			return false;
		}
		return true;
	}
	);

	try
	{
		return task.get();
	}
	catch (const std::exception& e)
	{
		handle_rest_error(e);
		return false;
	}
}


/*
 ================================================================================
 Simple wrappers to handle the connection error
 ================================================================================
*/
folder_meta_info gcs_get_folder_meta(std::wstring win_path) {
	retry_when_error(gcs_get_folder_meta_internal(win_path));
}

bool gcs_read_file(std::wstring win_path, void* buffer,
	size_t offset,
	size_t length) {
	retry_when_error(gcs_read_file_internal(win_path, buffer, offset, length));
}