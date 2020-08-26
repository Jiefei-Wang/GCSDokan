#include <vector>
#include <algorithm>
#include <google/cloud/storage/client.h>
#include "cloudFunctions.h"
#include "googleAuth.h"
#include "utilities.h"

using std::vector;
using std::string;
using std::wstring;
namespace gcs = google::cloud::storage::v1;


#define delimiter "/"


file_meta_info compose_file_meta(
	string linux_file_full_path,
	string name,
	size_t size,
	string crc32c,
	std::chrono::system_clock::time_point created,
	std::chrono::system_clock::time_point updated
) {
	file_meta_info info;
	info.remote_full_path = stringToWstring(linux_file_full_path);
	info.local_name = stringToWstring(name);
	info.remote_name = stringToWstring(name);
	info.size = size;
	info.crc32c = stringToWstring(crc32c);
	info.time_created = created;
	info.time_updated = updated;
	info.time_accessed = updated;
	return info;
}
gcs::ListObjectsAndPrefixesReader get_reader(decomposed_path path_info) {
	gcs::Client& client = get_client();
	string billing_project = get_billing_project();
	if (billing_project.length() != 0) {
		return client.ListObjectsAndPrefixes(path_info.bucket,
			gcs::UserProject(get_billing_project()),
			gcs::Prefix(path_info.path),
			gcs::Delimiter(delimiter));
	}
	else {
		return client.ListObjectsAndPrefixes(path_info.bucket,
			gcs::Prefix(path_info.path),
			gcs::Delimiter(delimiter));
	}
}


folder_meta_info gcs_get_folder_meta(std::wstring linux_path) {
	if (linux_path.at(linux_path.length() - 1) != L'/') {
		linux_path = linux_path + L"/";
	}
	decomposed_path path_info = get_path_info(linux_path);
	size_t path_size = path_info.path.length();
	gcs::ListObjectsAndPrefixesReader reader = get_reader(path_info);

	folder_meta_info dir_meta_info;
	dir_meta_info.local_name = stringToWstring(path_info.name);
	std::vector<wstring>& folder_names = dir_meta_info.folder_names;
	std::vector<file_meta_info> file_meta_vec;
	for (auto i : reader) {
		if (!i.ok()) {
			error_print(i.status().message().c_str());
			return {};
		}else{
			gcs::ObjectOrPrefix& info = i.value();
			if (absl::holds_alternative<gcs::ObjectMetadata>(info)) {
				gcs::ObjectMetadata meta = absl::get<gcs::ObjectMetadata>(info);
				string file_path = meta.name();
				string file_name = file_path.substr(path_size);
				string file_full_path = build_path(path_info.bucket, file_path);
				file_meta_info file_meta = compose_file_meta(
					file_full_path, file_name, meta.size(), meta.crc32c(),
					meta.time_created(), meta.updated()
				);
				file_meta_vec.push_back(file_meta);
			}
			if (absl::holds_alternative<string>(info)) {
				string folder_name = absl::get<string>(info);
				folder_name = folder_name.substr(path_size, folder_name.length() - 1 - path_size);
				folder_names.push_back(stringToWstring(folder_name));
			}
		}
	}
	// Solve name conflicting
	// 1. Place holder name conflicting
	// 2. file and folder name conflicting
	for (auto i : file_meta_vec) {
		//place holder
		if (i.local_name == L"") {
			i.hidden = true;
			i.local_name = L".placeholder";
			while (std::find_if(
				file_meta_vec.begin(),
				file_meta_vec.end(),
				[&](file_meta_info const& item) {
				return item.local_name == i.local_name; 
			}) != file_meta_vec.end()
				) {
				i.local_name = i.local_name + L"_";
			}
		}
		// Folder names
		while (std::find(
			folder_names.begin(),
			folder_names.end(),
			i.local_name) != folder_names.end()
			) {
			i.local_name = i.local_name + L"_";
		}
		
		//Insert the final result to the meta info
		dir_meta_info.file_meta_vector.insert(std::pair<std::wstring, file_meta_info>(i.local_name,i));
	}
	return dir_meta_info;
}


bool gcs_read_file_internal(std::wstring linux_path, void* buffer,
	size_t offset,
	size_t length) {
	decomposed_path info = get_path_info(linux_path);
	gcs::Client& client = get_client();
	gcs::ObjectReadStream read_stream;
	string billing_project = get_billing_project();
	if (billing_project.length() != 0) {
		read_stream = client.ReadObject(info.bucket, info.path, 
			gcs::ReadRange(offset, offset + length), 
			gcs::UserProject(get_billing_project()));
	}
	else {
		read_stream = client.ReadObject(info.bucket, info.path, 
			gcs::ReadRange(offset, offset + length));
	}



	read_stream.read((char*)buffer, length);
	if (read_stream.bad()) {
		error_print(read_stream.status().message().c_str());
		return false;
	}
	else {
		return true;
	}
}
/*
 ================================================================================
 Simple wrappers to handle the connection error
 ================================================================================
*/
/*
folder_meta_info gcs_get_folder_meta(std::wstring linux_path) {
	gcs_get_folder_meta_internal(linux_path);
}
*/

bool gcs_read_file(std::wstring linux_path, void* buffer,
	size_t offset,
	size_t length) {
	return gcs_read_file_internal(linux_path, buffer, offset, length);
}