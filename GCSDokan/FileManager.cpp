#include <chrono>
#include <map>
#include "fileManager.h"
#include "restAPIs.h"
#include "utilities.h"

using std::wstring;
//wstring root_path = L"genomics-public-data";
wstring root_path = L"bioconductor_test";
wstring cache_root = L"D:\\test";
bool use_cache = true;
bool use_disk_cache = true;
size_t update_interval = 60 * 10;
std::map<wstring, REST_result_holder> result_holder;

#define EXIST_FOLDER(meta) (meta.file_meta_vector.size()!=0 || meta.folder_names.size()!=0 || meta.local_name==L"/")

//minumum number of random access which will turn off the caching.
#define randome_access_tolerance 5


folder_meta_info list_files_internal(wstring linux_full_path)
{
	return gcs_get_folder_meta(linux_full_path);
}

bool get_file_data_internal(wstring linux_full_path, void* buffer, size_t offset, size_t length) {
	return gcs_read_file(linux_full_path, buffer, offset, length);
}


// ********************Derived function********************
void update_map(wstring key, REST_result_holder value) {
	if (result_holder.find(key) != result_holder.end())
		result_holder.insert(std::pair<wstring, REST_result_holder>(key, value));
	else
		result_holder[key] = value;
}
#define key_in_map(my_map, key) (my_map.find(key)!= my_map.end())

bool key_cached(wstring linux_full_path) {
	return key_in_map(result_holder, linux_full_path);
}

bool exist_folder_in_map(wstring linux_full_path) {
	return key_cached(linux_full_path);
}

bool exist_folder_in_cloud(wstring linux_full_path) {
	if (!exist_folder_in_map(linux_full_path)) {
		throw("Folder <%s> is not cached!", wstringToString(linux_full_path.c_str()).c_str());
	}
	return result_holder[linux_full_path].exist;
}
bool exist_folder_name_in_parent(wstring linux_full_path) {
	wstring folder_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (!exist_folder_in_map(parent_path)) {
		throw("Folder <%s> is not cached!", wstringToString(parent_path.c_str()).c_str());
	}
	std::vector<wstring>& folder_names = result_holder[parent_path].folder_meta.folder_names;
	return std::find(folder_names.begin(), folder_names.end(), folder_name) != folder_names.end();
}

bool exist_file_in_cloud(wstring linux_full_path) {
	wstring file_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (!exist_folder_in_map(parent_path)) {
		throw("Folder <%s> is not cached!", wstringToString(parent_path.c_str()).c_str());
	}
	return key_in_map(result_holder[parent_path].folder_meta.file_meta_vector, file_name);

}

bool is_time_outdated(std::chrono::system_clock::time_point query_time) {
	std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - query_time;
	return time_diff.count() > update_interval;
}
bool is_folder_outdated(wstring linux_full_path) {
	return !exist_folder_in_map(linux_full_path) ||
		is_time_outdated(result_holder[linux_full_path].query_time);
}

bool is_file_outdated(wstring linux_full_path) {
	wstring parent_path = to_parent_folder(linux_full_path);
	return is_folder_outdated(parent_path);
}

folder_meta_info get_folder_meta_internal(wstring linux_full_path) {
	if (exist_folder_in_map(linux_full_path)) {
		return result_holder[linux_full_path].folder_meta;
	}
	else {
		return folder_meta_info{};
	}
}

file_meta_info get_file_meta_internal(wstring linux_full_path) {
	wstring file_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (exist_folder_in_map(parent_path) &&
		exist_file_in_cloud(linux_full_path)) {
		return get_folder_meta_internal(parent_path).file_meta_vector.at(file_name);
	}
	else {
		return file_meta_info{};
	}
}


void add_folder_to_map(wstring linux_full_path, folder_meta_info info) {
	REST_result_holder holder;
	holder.exist = EXIST_FOLDER(info);
	holder.folder_meta = info;
	holder.query_time = std::chrono::system_clock::now();
	update_map(linux_full_path, holder);
}

void update_folder_info(wstring linux_full_path) {
	if (!exist_folder_in_map(linux_full_path) ||
		is_folder_outdated(linux_full_path)) {
		folder_meta_info info = list_files_internal(linux_full_path);
		add_folder_to_map(linux_full_path, info);
	}
}

void update_info(wstring linux_full_path) {

	// We first update its parent folder
	// If we identify the path as a folder then update the folder
	// Otherwise do nothing for we have all the information in its parent folder
	wstring parent_path = to_parent_folder(linux_full_path);
	if (parent_path == root_path) {
		update_folder_info(parent_path);
	}
	else {
		update_info(parent_path);
	}
	if (exist_folder_name_in_parent(linux_full_path)) {
		update_folder_info(linux_full_path);
	}
}



folder_meta_info get_folder_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(root_path, linux_path);
	update_info(linux_full_path);
	return get_folder_meta_internal(linux_full_path);
}

file_meta_info get_file_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(root_path, linux_path);
	update_info(linux_full_path);
	return get_file_meta_internal(linux_full_path);
}


bool exist_path(wstring win_path)
{
	return exist_file(win_path) || exist_folder(win_path);
}

bool exist_file(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(root_path, linux_path);
	update_info(linux_full_path);
	return exist_file_in_cloud(linux_full_path);
}

bool exist_folder(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(root_path, linux_path);
	update_info(linux_full_path);
	return exist_folder_in_cloud(linux_full_path);
}

NTSTATUS get_file_data_from_cloud(wstring win_path, void* buffer, size_t offset, size_t length) {
	file_meta_info file_meta = get_file_meta(win_path);
	wstring linux_full_path = file_meta.remote_full_path;
	bool success = get_file_data_internal(linux_full_path, buffer, offset, length);
	if (success) {
		return STATUS_ACCESS_DENIED;
	}
	return STATUS_SUCCESS;
}

void read_file_from_cloud(void* linux_full_path, void* buffer, size_t offset, size_t length) {
	get_file_data_internal(*(wstring*)linux_full_path, buffer, offset, length);
}

#include "fileCache.h"
NTSTATUS get_file_data(wstring win_path, void* buffer, size_t offset, size_t length, file_private_info* private_info)
{
	void*& cache_info = private_info->cache_info;
	file_meta_info file_meta = get_file_meta(win_path);
	bool success = false;
	if (use_cache&& private_info->random_read_time < randome_access_tolerance) {
		if (use_disk_cache) {
			if (cache_info == nullptr)
				cache_info = new file_cache(
					wstringToString(build_path(cache_root, win_path, L'\\')),
					wstringToString(file_meta.crc32c),
					file_meta.size,
					&read_file_from_cloud,
					(void*)&file_meta.remote_full_path);
			try {
				((file_cache*)cache_info)->read_data((char*)buffer, offset, length);
				success = true;
			}
			catch (std::exception) {
			}
		}
		else {
			//TODO: memory cache
		}
	}
	if (!success) {
		return get_file_data_from_cloud(win_path, buffer, offset, length);
	}
	else {
		return STATUS_SUCCESS;
	}
}

