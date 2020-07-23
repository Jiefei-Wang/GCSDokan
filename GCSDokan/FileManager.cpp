#include <chrono>
#include <map>
#include "fileManager.h"
#include "restAPIs.h"


//std::wstring root_path = L"genomics-public-data";
std::wstring root_path = L"bioconductor_test";


#define EXIST_FILE(meta) (meta.win_name.length() != 0)
#define EXIST_FOLDER(meta) (meta.file_meta_vector.size()!=0 || meta.folder_names.size()!=0)

size_t update_interval = 60*10;
std::map<std::wstring,REST_result_holder> result_holder;


folder_meta_info list_files_internal(std::wstring win_path)
{
	return gcs_get_folder_meta(win_path);
}

file_meta_info get_file_meta_internal(std::wstring win_path)
{
	return gcs_get_file_meta(win_path);
}

void get_file_data_internal(std::wstring win_path, void* buffer, size_t& buffer_len, size_t offset) {
	gcs_read_file(win_path, buffer, buffer_len, offset);
}

// ********************Derived function********************
void update_map(std::wstring key, REST_result_holder value) {
	if (result_holder.find(key) != result_holder.end())
		result_holder.insert(std::pair<std::wstring, REST_result_holder>(key, value));
	else
		result_holder[key] = value;
}
bool is_outdate(std::chrono::system_clock::time_point query_time) {
	std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - query_time;
	return time_diff.count() > update_interval;
}

bool need_update(std::wstring win_path) {
	bool has_key = result_holder.find(win_path) != result_holder.end();
	if (has_key) {
		REST_result_holder& result = result_holder.at(win_path);
		if (!is_outdate(result.query_time)) {
			return false;
		}
	}
	return true;
}

void add_file_to_map(file_meta_info info) {
	REST_result_holder holder;
	holder.exist = EXIST_FILE(info);
	holder.is_file = true;
	holder.query_time = std::chrono::system_clock::now();
	holder.file_meta = info;
	update_map(info.win_full_path, holder);
}

void add_folder_to_map(std::wstring win_path, folder_meta_info info) {
	REST_result_holder holder;
	holder.exist = EXIST_FOLDER(info);
	holder.is_file = false;
	for (auto i : info.file_meta_vector) {
		add_file_to_map(i);
	}
	holder.folder_meta = info;
	holder.query_time = std::chrono::system_clock::now();
	update_map(win_path, holder);
}

bool update_file_info(std::wstring win_path) {
	if (!need_update(win_path)) {
		return result_holder[win_path].exist;
	}
	//If we have cached the file info, we need to check if the file name 
	//has a confiction with the folder name.
	if (result_holder.find(win_path) != result_holder.end()) {
		auto info_holder = result_holder.find(win_path)->second;
		if (info_holder.exist&& 
			info_holder.is_file&& 
			info_holder.file_meta.win_name!= info_holder.file_meta.cloud_name) {
			file_meta_info file_info = get_file_meta_internal(info_holder.file_meta.cloud_full_path);
			if (EXIST_FILE(file_info)) {
				add_file_to_map(file_info);
			}
			else {
				file_info = get_file_meta_internal(win_path);
				add_file_to_map(file_info);
			}
		}
	}
	else {
		file_meta_info info = get_file_meta_internal(win_path);
		add_file_to_map(info);
	}
	
	if (result_holder.find(win_path) != result_holder.end()&&
		result_holder[win_path].exist)
		return true;
	else 
		return false;
}
bool update_folder_info(std::wstring win_path) {
	if (!need_update(win_path)) {
		return result_holder[win_path].exist;
	}
	folder_meta_info info = list_files_internal(win_path);
	add_folder_to_map(win_path, info);
	if (EXIST_FOLDER(info)|| 
		win_path == L"\\") {
		return true;
	}
	else {
		return false;
	}
}

void update_info(std::wstring win_path) {
	bool exist = false;
	if (win_path == L"\\") {
		exist = update_folder_info(win_path);
	}
	else {
		exist = update_file_info(win_path);
		if (!exist) {
			exist = update_folder_info(win_path);
		}
	}
	if (!exist) {
		REST_result_holder holder;
		holder.query_time = std::chrono::system_clock::now();
		holder.exist = false;
		update_map(win_path, holder);
	}
}



folder_meta_info get_folder_meta(std::wstring win_path)
{
	update_info(win_path);
	return result_holder[win_path].folder_meta;
}

file_meta_info get_file_meta(std::wstring win_path)
{
	update_info(win_path);
	return result_holder[win_path].file_meta;
}


bool exist_path(std::wstring win_path)
{
	update_info(win_path);
	return result_holder[win_path].exist;
}

bool exist_file(std::wstring win_path)
{
	return EXIST_FILE(get_file_meta(win_path));
}

bool exist_folder(std::wstring win_path)
{
	return EXIST_FOLDER(get_folder_meta(win_path))|| win_path == L"\\";
}


NTSTATUS get_file_data(std::wstring win_path, void* buffer, size_t& buffer_len, size_t offset)
{
	if (result_holder.find(win_path) == result_holder.end()) {
		buffer_len = 0;
	}
	else {
		win_path = result_holder.at(win_path).file_meta.cloud_full_path;
	}
	size_t true_read_len = buffer_len;
	get_file_data_internal(win_path, buffer, true_read_len, offset);
	if (true_read_len != buffer_len) {
		buffer_len = true_read_len;
		return STATUS_ACCESS_DENIED;
	}
	return STATUS_SUCCESS;
}

