#include<process.h>
#include<functional>
#include "daemon.h"
#include "utilities.h"
#include "globalVariables.h"
#include "fileManager.h"
#define BOOST_USE_WINDOWS_H
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>


using std::wstring;
using std::string;
namespace bi = boost::interprocess;

#define common_shared_memory_name "Local\\GCSDokan_mount_point_list"

//GCSDokan running instance in the shared memory
// map: mount point -> remote link
typedef bi::allocator<WCHAR, bi::managed_windows_shared_memory::segment_manager> CharAllocator;
typedef bi::basic_string<WCHAR, std::char_traits<WCHAR>, CharAllocator> ShmString;
typedef std::pair<const ShmString, ShmString> ValueType;
typedef bi::allocator<ValueType, bi::managed_windows_shared_memory::segment_manager> ShmemAllocator;
typedef bi::map<ShmString, ShmString, std::less<ShmString>, ShmemAllocator> shared_map;

//This shared memory object stores all the running gcs instance.
static bi::managed_windows_shared_memory* segment = nullptr;
static shared_map* gcs_instance_list = nullptr;
static bi::managed_windows_shared_memory* existance_indicator = nullptr;
bi::named_mutex mutex(bi::open_or_create, "GCSDokan_mount_point_list");

static std::hash<wstring> hash_object;

/*
========================================================================================
Functions to get keys and utilities
========================================================================================
*/
wstring add_white_space(wstring str, size_t maxLength) {
	size_t i, length;
	length = str.length();
	for (i = length; i < maxLength; i++)
		str = str + L" ";
	return str;
};


static wstring get_key(wstring key) {
	return L"GCSDokan_" + std::to_wstring(hash_object(key));
}

static wstring get_pip_name(wstring key) {
	return L"\\\\.\\pipe\\" + get_key(key);
}

static wstring get_shared_memory_name(wstring key) {
	return L"Local\\" + get_key(key);
}

/*
========================================================================================
Functions to store all running GCSDokan instance in shared memory
========================================================================================
*/
static bool has_shared_memory(string key)
{
	try
	{
		bi::managed_windows_shared_memory segment(bi::open_only, key.c_str());
		return true;
	}
	catch (const std::exception)
	{
		return false;
	}
}

static bool exist_instance(ShmString mount_point) {
	string key = wstringToString(get_shared_memory_name(mount_point.c_str()));
	return has_shared_memory(key.c_str());
}

static void remove_unexist_instance_from_list() {
	auto itr = gcs_instance_list->begin();
	while (itr != gcs_instance_list->end()) {
		if (!exist_instance(itr->first)) {
			itr = gcs_instance_list->erase(itr);
		}
		else {
			++itr;
		}
	}
}

static bool initial_gcs_instance_list()
{
	if (gcs_instance_list != nullptr)
		return true;
	try
	{
		if (segment != nullptr) {
			delete segment;
		}
		//Initialize the instance list
		if (!has_shared_memory(common_shared_memory_name)) {
			boost::interprocess::permissions perm;
			perm.set_unrestricted();
			segment = new bi::managed_windows_shared_memory(bi::create_only, common_shared_memory_name, 1024 * 1024, 0, perm);
			//allocate the instance list
			ShmemAllocator alloc_inst(segment->get_segment_manager());
			gcs_instance_list = segment->construct<shared_map>("instance_list")(std::less<ShmString>(), alloc_inst);
		}
		else {
			segment = new bi::managed_windows_shared_memory(bi::open_only, common_shared_memory_name);
			//allocate or open the instance list
			ShmemAllocator alloc_inst(segment->get_segment_manager());
			gcs_instance_list = segment->find<shared_map>("instance_list").first;
		}
	}
	catch (const std::exception& ex)
	{
		error_print("Unable to initiate shared memory space: %s\n", ex.what());
		return false;
	}
	if (gcs_instance_list == nullptr) {
		error_print("Unable to create instance list\n");
		return false;
	}
	//Remove the unexist instance
	remove_unexist_instance_from_list();
	return true;
}

static bool create_existance_indicator() {
	//Create a placeholder in the shared memory which will be destroyed when the instance is closed.
	try
	{
		string key = wstringToString(get_shared_memory_name(get_mount_point()));
		boost::interprocess::permissions perm;
		perm.set_unrestricted();
		//We do this memory leaking on purpose
		existance_indicator = new bi::managed_windows_shared_memory(bi::create_only, key.c_str(), 512, 0, perm);
	}
	catch (const std::exception& ex)
	{
		error_print("Unable to create existance indicator: %s\n", ex.what());
		return false;
	}
	//Insert key and value to the instance list
	ShmString map_key(get_mount_point().c_str(), segment->get_allocator<ShmString>());
	ShmString map_value(get_remote_link().c_str(), segment->get_allocator<ShmString>());
	gcs_instance_list->insert(ValueType(map_key, map_value));

	return true;
}




/*
========================================================================================
Functions to kill a GCSDokan instance when receive a signal
========================================================================================
*/

static unsigned __stdcall thread_monitor_function(void*)
{
	wstring pip_name = get_pip_name(get_mount_point());
	HANDLE pipe = CreateNamedPipe(
		pip_name.c_str(), // name of the pipe
		PIPE_ACCESS_INBOUND, // 1-way pipe -- receive only
		PIPE_TYPE_BYTE, // get data as a byte stream
		1, // only allow 1 instance of this pipe
		0, // no outbound buffer
		0, // no inbound buffer
		0, // use default wait time
		NULL // use default security attributes
	);
	if (pipe == NULL || pipe == INVALID_HANDLE_VALUE) {
		ExitProcess(ERROR_BAD_PIPE);
	}
	// The thread will be blocked by this function
	// until a process wants to connect to this pip
	BOOL result = ConnectNamedPipe(pipe, NULL);
	if (!result) {
		ExitProcess(ERROR_PIPE_NOT_CONNECTED);
	}
	else {
		CloseHandle(pipe);
		ExitProcess(0);
	}
	return 0;
}

/*
========================================================================================
Exported function
========================================================================================
*/
bool start_deamon()
{
	mutex.lock();
	bool success = initial_gcs_instance_list();
	if (!success) {
		mutex.unlock();
		return false;
	}
	success = create_existance_indicator();
	if (!success) {
		mutex.unlock();
		return false;
	}
	HANDLE handle;
	handle = (HANDLE)_beginthreadex(NULL, 0, thread_monitor_function,
		0, 0, 0);
	if (handle == NULL) {
		error_print("Unable to start the daemon!\n");
		mutex.unlock();
		return false;
	}
	mutex.unlock();
	return true;
}

void kill_process()
{
	wstring pip_name = get_pip_name(get_mount_point());
	HANDLE pipe = CreateFile(
		pip_name.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	CloseHandle(pipe);
}



bool list_instances()
{
	mutex.lock();
	bool success = initial_gcs_instance_list();
	if (!success) {
		error_print("Unable to initiate shared memory space!\n");
		mutex.unlock();
		return false;
	}
	// Add padding space
	size_t mount_point_size = 11;
	for (auto i : *gcs_instance_list) {
		mount_point_size = max(i.first.length(), mount_point_size);
	}
	std::vector<wstring> mount_points;
	std::vector<wstring> remote_links;
	mount_points.push_back(L"Mount Point");
	remote_links.push_back(L"Remote Link");

	for (auto i : *gcs_instance_list) {
		wstring mp = add_white_space(i.first.c_str(), mount_point_size);
		mount_points.push_back(mp);
		remote_links.push_back(i.second.c_str());
	}
	for (int i = 0; i < mount_points.size(); i++) {
		wprintf(L"%s\t\t%s\n", mount_points[i].c_str(), remote_links[i].c_str());
	}
	mutex.unlock();
	return true;
}
