//#include "utilities.h"
#include "googleAuth.h"
#include "fileManager.h"
#include "dokanOperations.h"
#include "globalVariables.h"
#include "utilities.h"
#include <boost/algorithm/string.hpp>




using std::string;
using std::wstring;






int run_dokan()
{
	DOKAN_OPERATIONS dokanOperations;
	ZeroMemory(&dokanOperations, sizeof(DOKAN_OPERATIONS));
	dokanOperations.ZwCreateFile = cloud_create_file;
	dokanOperations.Cleanup = cloud_cleanup;
	dokanOperations.CloseFile = cloud_close_file;
	dokanOperations.ReadFile = cloud_read_file;
	dokanOperations.WriteFile = cloud_write_file;
	dokanOperations.FlushFileBuffers = cloud_flush_buffers;
	dokanOperations.GetFileInformation = cloud_get_file_information;
	dokanOperations.FindFiles = cloud_find_files;
	dokanOperations.FindFilesWithPattern = NULL;
	dokanOperations.SetFileAttributes = NULL;
	dokanOperations.SetFileTime = NULL;
	dokanOperations.DeleteFile = NULL;
	dokanOperations.DeleteDirectory = NULL;
	dokanOperations.MoveFile = NULL;
	dokanOperations.SetEndOfFile = NULL;
	dokanOperations.SetAllocationSize = NULL;
	dokanOperations.LockFile = NULL;
	dokanOperations.UnlockFile = NULL;
	dokanOperations.GetFileSecurity = NULL;
	dokanOperations.SetFileSecurity = NULL;
	dokanOperations.GetDiskFreeSpace = NULL;
	dokanOperations.GetVolumeInformation = cloud_get_colume_info;
	dokanOperations.Unmounted = NULL;
	dokanOperations.FindStreams = NULL;
	dokanOperations.Mounted = NULL;

	DOKAN_OPTIONS dokanOptions;
	dokanOptions.MountPoint = mount_point.c_str();
	dokanOptions.Version = DOKAN_VERSION;
	dokanOptions.ThreadCount = 0;
	dokanOptions.UNCName = L"";
	//dokanOptions.Timeout;
	//dokanOptions.AllocationUnitSize;
	dokanOptions.SectorSize;
	dokanOptions.Options = 0;

	int status = DokanMain(&dokanOptions, &dokanOperations);
	return status;
}


void run_program_in_background() {
	wstring command(GetCommandLine());
	command = command + L" -secretBlock";
	WCHAR* command_buffer = new WCHAR[command.length() + 1];
	memcpy_s(command_buffer, (command.length() + 1) * sizeof(WCHAR),
		command.c_str(), (command.length() + 1) * sizeof(WCHAR));
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;
	DWORD dwCreationFlags = 0;
	if (verbose_level == 0) {
		dwCreationFlags = DETACHED_PROCESS;
	}
	bool success = CreateProcess(NULL, command_buffer, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi);
	if (!success) {
		error_print(L"Fail to run the process in backgroud\n");
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}

#define CHECK_CMD_ARG(commad, argc)                                            \
  {                                                                            \
    if (++command == argc) {                                                   \
      fwprintf(stderr, L"Option is missing an argument.\n");                   \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  }


int process_arguments(int argc, char* argv[]) {
	int count = 0;
	for (int command = 1; command < argc; command++) {
		string command_str(argv[command]);
		//Basic command: bucket mount_point
		if (command_str.at(0) != '-') {
			if (count == 0) {
				set_remote_link(stringToWstring(command_str));
				if (remote_link.length() == 0) {
					error_print(L"Invalid remote path\n");
					return EXIT_FAILURE;
				}
				count++;
				continue;
			}
			if (count == 1) {
				mount_point = stringToWstring(argv[command]);
				if (mount_point.length() == 0) {
					error_print(L"Invalid RootDirectory\n");
					return EXIT_FAILURE;
				}
				count++;
				continue;
			}
		}
		boost::to_lower(command_str);
		//verbose
		if (command_str == "-v"|| command_str == "-verbose") {
			verbose_level = 1;
			continue;
		}
		//disk cache
		if (command_str == "-dc") {
			if (command + 1 != argc &&
				argv[command + 1][0] != '-') {
				CHECK_CMD_ARG(command, argc);
				cache_root = stringToWstring(argv[command]);
				cache_type = CACHE_TYPE::disk;
				continue;
			}
			else {
				//TODO: get a temporary cache path
				char buffer[DIRECTORY_MAX_PATH];
				size_t len = GetTempPathA(DIRECTORY_MAX_PATH, buffer);
				if (len == 0 || len > DIRECTORY_MAX_PATH) {
					error_print("Fail to get a temporary path\n");
					return EXIT_FAILURE;
				}
				cache_root = stringToWstring(buffer);
				cache_type = CACHE_TYPE::disk;
				continue;
			}
		}
		//memory cache
		if (command_str == "-mc") {
			cache_type = CACHE_TYPE::memory;
			if (command + 1 != argc &&
				argv[command + 1][0] != '-') {
				CHECK_CMD_ARG(command, argc);
				try {
					memory_cache_block_number = std::stoi(argv[command]);
				}
				catch (std::exception ex) {
					error_print("unknown memory cache option: %s\n", argv[command]);
					return EXIT_FAILURE;
				}
			}
			continue;
		}
		//no cache
		if (command_str == "-nc") {
			cache_type = CACHE_TYPE::none;
			continue;
		}
		//refresh rate
		if (command_str == "-r") {
			CHECK_CMD_ARG(command, argc);
			update_interval = std::stoi(argv[command]);
			continue;
		}
		if (command_str == "-nonblock" || command_str == "-secretblock") {
			continue;
		}
		error_print("unknown command: %s\n", argv[command]);
		return EXIT_FAILURE;
	}

	debug_print(L"verbose mode on\n");
	debug_print(L"remote path: %ls\n", remote_link.c_str());
	debug_print(L"mount point: %ls\n", mount_point.c_str());
	debug_print(L"refresh interval: %ds\n", update_interval);
	debug_print(L"cache path: %ls\n", cache_root.c_str());
	return 0;
}



int main(int argc, char* argv[])
{
	bool non_block = false;
	for (int command = 1; command < argc; command++) {
		string command_str(argv[command]);
		boost::to_lower(command_str);
		if (command_str == "-nonblock") {
			non_block = true;
		}
		if (command_str == "-secretblock") {
			non_block = false;
			break;
		}
	}

	//Try to obtain and verify the arguments in the main process
	int result = process_arguments(argc, argv);
	if (result != 0)
		return result;

	if (non_block) {
		run_program_in_background();
		return 0;
	}


	auto_auth();

	result = run_dokan();
	if (result != 0) {
		error_print(L"Fail to mount the virtual disk, error code: %d", result);
	}

	//gcloud_auth();
	//auto result = exist_folder(LR"(\tt)");
	//auto result3 = get_file_meta(LR"(\testfile1.txt)");
	//auto result1 = get_folder_meta(LR"(\)");
	//auto result2 = get_folder_meta(L"\\1000-genomes-phase-3\\vcf");
	//auto result3 = get_file_meta(LR"(\1000-genomes\bam)");
	//system("pause");
	return result;
}