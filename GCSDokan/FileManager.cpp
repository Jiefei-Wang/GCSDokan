#include <chrono>
#include <map>
#include "fileManager.h"
#include "restAPIs.h"
#include "utilities.h"

using std::wstring;
//wstring root_path = L"genomics-public-data";
wstring root_path = L"bioconductor_test";


#define EXIST_FILE(meta) (meta.local_name.length() != 0)
#define EXIST_FOLDER(meta) (meta.file_meta_vector.size()!=0 || meta.folder_names.size()!=0 || meta.local_name==L"/")

size_t update_interval = 60*10;
std::map<wstring,REST_result_holder> result_holder;


folder_meta_info list_files_internal(wstring linux_path)
{
	return gcs_get_folder_meta(build_path(root_path, linux_path));
}

void get_file_data_internal(wstring linux_full_path, void* buffer, size_t& buffer_len, size_t offset) {
	gcs_read_file(linux_full_path, buffer, buffer_len, offset);
}


// ********************Derived function********************
void update_map(wstring key, REST_result_holder value) {
	if (result_holder.find(key) != result_holder.end())
		result_holder.insert(std::pair<wstring, REST_result_holder>(key, value));
	else
		result_holder[key] = value;
}
#define key_in_map(my_map, key) (my_map.find(key)!= my_map.end())

bool key_cached(wstring linux_path) {
	return key_in_map(result_holder, linux_path);
}

bool exist_folder_in_map(wstring linux_path) {
	return key_cached(linux_path);
}

bool exist_folder_in_cloud(wstring linux_path) {
	return exist_folder_in_map(linux_path) && result_holder[linux_path].exist;
}
bool exist_folder_name_in_parent(wstring linux_path) {
	wstring folder_name = get_file_name_in_path(linux_path);
	wstring parent_path = to_parent_folder(linux_path);
	std::vector<wstring>& folder_names = result_holder[linux_path].folder_meta.folder_names;
	return exist_folder_in_map(parent_path) && 
		std::find(folder_names.begin(), folder_names.end(), folder_name)!= folder_names.end();
}

bool exist_file_in_cloud(wstring linux_path) {
	wstring file_name = get_file_name_in_path(linux_path);
	wstring parent_path = to_parent_folder(linux_path);
	return exist_folder_in_map(parent_path) &&
		key_in_map(result_holder[parent_path].folder_meta.file_meta_vector, file_name);
		
}

bool is_time_outdated(std::chrono::system_clock::time_point query_time) {
	std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - query_time;
	return time_diff.count() > update_interval;
}
bool is_folder_outdated(wstring linux_path) {
	return !exist_folder_in_map(linux_path) ||
		is_time_outdated(result_holder[linux_path].query_time);
}

bool is_file_outdated(wstring linux_path) {
	wstring parent_path = to_parent_folder(linux_path);
	return is_folder_outdated(parent_path);
}

folder_meta_info get_folder_meta_internal(wstring linux_path) {
	if (exist_folder_in_map(linux_path)) {
		return result_holder[linux_path].folder_meta;
	}
	else {
		return folder_meta_info{};
	}
}

file_meta_info get_file_meta_internal(wstring linux_path) {
	wstring file_name = get_file_name_in_path(linux_path);
	wstring parent_path = to_parent_folder(linux_path);
	if (exist_file_in_cloud(linux_path)) {
		return get_folder_meta_internal(parent_path).file_meta_vector.at(file_name);
	}
	else {
		return file_meta_info{};
	}
}


void add_folder_to_map(wstring linux_path, folder_meta_info info) {
	REST_result_holder holder;
	holder.exist = EXIST_FOLDER(info);
	holder.folder_meta = info;
	holder.query_time = std::chrono::system_clock::now();
	update_map(linux_path, holder);
}

void update_folder_info(wstring linux_path) {
	if (is_folder_outdated(linux_path)) {
		folder_meta_info info = list_files_internal(linux_path);
		add_folder_to_map(linux_path, info);
	}
}

void update_info(wstring linux_path) {
	if (linux_path == L"/") {
		update_folder_info(linux_path);
	}
	else {
		// We first update its parent folder
		// If we identify the path as a folder then update the folder
		// Otherwise do nothing for we have all the information in its parent folder
		wstring parent_path = to_parent_folder(linux_path);
		update_folder_info(parent_path);
		if (exist_folder_name_in_parent(linux_path)) {
			update_folder_info(linux_path);
		}
	}
}



folder_meta_info get_folder_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	update_info(linux_path);
	return get_folder_meta_internal(linux_path);
}

file_meta_info get_file_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	update_info(linux_path);
	return get_file_meta_internal(linux_path);
}


bool exist_path(wstring win_path)
{
	return exist_file(win_path)|| exist_folder(win_path);
}

bool exist_file(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	update_info(linux_path);
	return exist_file_in_cloud(linux_path);
}

bool exist_folder(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	update_info(linux_path);
	return exist_folder_in_cloud(linux_path);
}


NTSTATUS get_file_data(wstring win_path, void* buffer, size_t& buffer_len, size_t offset)
{
	wstring linux_path = to_linux_slash(win_path);
	if (result_holder.find(linux_path) == result_holder.end()) {
		buffer_len = 0;
	}
	else {
		linux_path = get_file_meta_internal(linux_path).remote_full_path;
	}
	size_t true_read_len = buffer_len;
	get_file_data_internal(linux_path, buffer, true_read_len, offset);
	if (true_read_len != buffer_len) {
		buffer_len = true_read_len;
		return STATUS_ACCESS_DENIED;
	}
	return STATUS_SUCCESS;
}

