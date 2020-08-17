#include <chrono>
#include <map>
#include "fileManager.h"
#include "cloudFunctions.h"
#include "utilities.h"
#include "fileCache.h"
#include "memoryCache.h"

using std::wstring;

#define EXIST_FOLDER(meta) (meta.file_meta_vector.size()!=0 || meta.folder_names.size()!=0)

//Key: full remote path(bucket + path)
struct cloud_info_holder {
	std::chrono::system_clock::time_point query_time;
	bool exist = false;
	folder_meta_info folder_meta;
};


static CACHE_TYPE cache_type = CACHE_TYPE::memory;
static wstring cache_root;
static wstring remote_link;
static size_t refresh_interval = 60;

std::map<wstring, cloud_info_holder> cloud_info_cache;


folder_meta_info list_files_internal(wstring linux_full_path)
{
	return gcs_get_folder_meta(linux_full_path);
}

bool get_file_data_internal(wstring linux_full_path, void* buffer, size_t offset, size_t length) {
	return gcs_read_file(linux_full_path, buffer, offset, length);
}

// ********************accessor functions********************
void set_remote_link(std::wstring link) {
	wstring header = L"gs://";
	if (link.rfind(header.c_str(), 0) == 0) {
		link = link.substr(header.length());
	}
	remote_link = remove_tailing_slash(link);
}
void set_cache_root(std::wstring path)
{
	cache_root = remove_tailing_slash(path);
}
void set_cache_type(CACHE_TYPE type) {
	cache_type = type;
}
void set_refresh_interval(size_t time) {
	refresh_interval = time;
}

void set_mount_point(std::string path) {
	set_mount_point(stringToWstring(path));
}
void set_remote_link(std::string link) {
	set_remote_link(stringToWstring(link));
}
void set_cache_root(std::string path) {
	set_cache_root(stringToWstring(path));
}
std::wstring get_remote_link() {
	return remote_link;
}
std::wstring get_cache_root() {
	return cache_root;
}
CACHE_TYPE get_cache_type() {
	return cache_type;
}

size_t get_refresh_interval() {
	return refresh_interval;
}


// ********************Derived function********************
static void update_map(wstring key, cloud_info_holder value) {
	if (cloud_info_cache.find(key) != cloud_info_cache.end())
		cloud_info_cache.insert(std::pair<wstring, cloud_info_holder>(key, value));
	else
		cloud_info_cache[key] = value;
}
#define key_in_map(my_map, key) (my_map.find(key)!= my_map.end())

static bool key_cached(wstring linux_full_path) {
	return key_in_map(cloud_info_cache, linux_full_path);
}

static bool exist_folder_in_map(wstring linux_full_path) {
	return key_cached(linux_full_path);
}

static bool exist_folder_in_cloud(wstring linux_full_path) {
	if (!exist_folder_in_map(linux_full_path)) {
		throw("Folder <%s> is not cached!", wstringToString(linux_full_path.c_str()).c_str());
	}
	return cloud_info_cache[linux_full_path].exist;
}
static bool exist_folder_name_in_parent(wstring linux_full_path) {
	wstring folder_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (!exist_folder_in_map(parent_path)) {
		throw("Folder <%s> is not cached!", wstringToString(parent_path.c_str()).c_str());
	}
	std::vector<wstring>& folder_names = cloud_info_cache[parent_path].folder_meta.folder_names;
	return std::find(folder_names.begin(), folder_names.end(), folder_name) != folder_names.end();
}

static bool exist_file_in_cloud(wstring linux_full_path) {
	wstring file_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (!exist_folder_in_map(parent_path)) {
		throw("Folder <%s> is not cached!", wstringToString(parent_path.c_str()).c_str());
	}
	return key_in_map(cloud_info_cache[parent_path].folder_meta.file_meta_vector, file_name);

}

static bool is_time_outdated(std::chrono::system_clock::time_point query_time) {
	std::chrono::duration<double> time_diff = std::chrono::system_clock::now() - query_time;
	return time_diff.count() > get_refresh_interval();
}
static bool is_folder_outdated(wstring linux_full_path) {
	return !exist_folder_in_map(linux_full_path) ||
		is_time_outdated(cloud_info_cache[linux_full_path].query_time);
}

static bool is_file_outdated(wstring linux_full_path) {
	wstring parent_path = to_parent_folder(linux_full_path);
	return is_folder_outdated(parent_path);
}

static folder_meta_info get_folder_meta_internal(wstring linux_full_path) {
	if (exist_folder_in_map(linux_full_path)) {
		return cloud_info_cache[linux_full_path].folder_meta;
	}
	else {
		return folder_meta_info{};
	}
}

static file_meta_info get_file_meta_internal(wstring linux_full_path) {
	wstring file_name = get_file_name_in_path(linux_full_path);
	wstring parent_path = to_parent_folder(linux_full_path);
	if (exist_file_in_cloud(linux_full_path)) {
		return get_folder_meta_internal(parent_path).file_meta_vector.at(file_name);
	}
	else {
		return file_meta_info{};
	}
}


static void add_folder_to_map(wstring linux_full_path, folder_meta_info info) {
	cloud_info_holder holder;
	holder.exist = EXIST_FOLDER(info)|| linux_full_path == get_remote_link();
	holder.folder_meta = info;
	holder.query_time = std::chrono::system_clock::now();
	update_map(linux_full_path, holder);
}

static void update_folder_info(wstring linux_full_path) {
	if (!exist_folder_in_map(linux_full_path) ||
		is_folder_outdated(linux_full_path)) {
		folder_meta_info info = list_files_internal(linux_full_path);
		add_folder_to_map(linux_full_path, info);
	}
}

static void update_info(wstring linux_full_path) {
	if (linux_full_path == get_remote_link()) {
		update_folder_info(linux_full_path);
		return;
	}
	// We first update its parent folder
	// If we identify the path as a folder then update the folder
	// Otherwise do nothing for we have all the information in its parent folder
	wstring parent_path = to_parent_folder(linux_full_path);
	update_folder_info(parent_path);

	if (exist_folder_name_in_parent(linux_full_path)) {
		update_folder_info(linux_full_path);
	}
	else {
		//We only add a placeholder here to indicate we have queired it
		folder_meta_info info;
		add_folder_to_map(linux_full_path, info);
	}
}

/*
=================================================================
Exported functions
=================================================================
*/
folder_meta_info get_folder_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(get_remote_link(), linux_path);
	update_info(linux_full_path);
	return get_folder_meta_internal(linux_full_path);
}

file_meta_info get_file_meta(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") linux_path = L"";
	wstring linux_full_path = build_path(get_remote_link(), linux_path);
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
	wstring linux_full_path = build_path(get_remote_link(), linux_path);
	update_info(linux_full_path);
	return exist_file_in_cloud(linux_full_path);
}

bool exist_folder(wstring win_path)
{
	wstring linux_path = to_linux_slash(win_path);
	if (linux_path == L"/") return true;
	wstring linux_full_path = build_path(get_remote_link(), linux_path);
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
	wstring* path = (wstring*)linux_full_path;
	get_file_data_internal(*path, buffer, offset, length);
}



file_manager_handle get_file_manager_handle(wstring win_path) {
	file_manager_handle manager_handle = DBG_NEW file_manager_handle_struct{};
	manager_handle->cache_type = get_cache_type();
	manager_handle->win_path = win_path;
	file_meta_info file_meta = get_file_meta(win_path);
	manager_handle->linux_full_path = file_meta.remote_full_path;
	if (manager_handle->cache_type == CACHE_TYPE::disk) {
		manager_handle->cache_info = DBG_NEW file_cache(
			wstringToString(build_path(get_cache_root(), win_path, L'\\')),
			wstringToString(file_meta.crc32c),
			file_meta.size,
			&read_file_from_cloud,
			&manager_handle->linux_full_path);
	}
	if (manager_handle->cache_type == CACHE_TYPE::memory) {
		manager_handle->cache_info = DBG_NEW memory_cache(
			wstringToString(file_meta.crc32c),
			file_meta.size,
			&read_file_from_cloud,
			&manager_handle->linux_full_path);
	}
	return manager_handle;
}

void release_file_manager_handle(file_manager_handle manager_handle) {
	if (manager_handle->cache_type == CACHE_TYPE::disk) {
		delete (file_cache*)manager_handle->cache_info;
	}
	if (manager_handle->cache_type == CACHE_TYPE::memory) {
		delete (memory_cache*)manager_handle->cache_info;
	}
	delete manager_handle;
}



NTSTATUS get_file_data(file_manager_handle file_manager, void* buffer, size_t offset, size_t length)
{
	bool success = false;
	if (file_manager->cache_type == CACHE_TYPE::disk) {
		file_cache* cache_info = (file_cache*)file_manager->cache_info;
		try {
			cache_info->read_data((char*)buffer, offset, length);
			success = true;
		}
		catch (std::exception ex) {
			error_print("%s\n", ex.what());
		}
	}

	if (file_manager->cache_type == CACHE_TYPE::memory) {
		memory_cache* cache_info = (memory_cache*)file_manager->cache_info;
		try {
			cache_info->read_data((char*)buffer, offset, length);
			success = true;
		}
		catch (std::exception ex) {
			error_print("%s\n", ex.what());
		}
	}
	if (!success) {
		return get_file_data_from_cloud(file_manager->win_path, buffer, offset, length);
	}
	else {
		return STATUS_SUCCESS;
	}
}

