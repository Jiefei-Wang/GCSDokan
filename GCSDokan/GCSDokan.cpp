//#include "utilities.h"
#include <boost/algorithm/string.hpp>
#include "googleAuth.h"
#include "fileManager.h"
#include "dokanOperations.h"
#include "dokanOptions.h"
#include "globalVariables.h"
#include "memoryCache.h"
#include "utilities.h"
#include "daemon.h"

using std::string;
using std::wstring;

#define CHECK_CMD_ARG(commad, argc)												\
  {																				\
    if (++command == argc) {													\
      fwprintf(stderr, L"Option is missing an argument.\n");					\
      return EXIT_FAILURE;														\
    }																			\
  }

#define HAS_OPTIONS(commad, argc)												\
command + 1 != argc &&															\
argv[command + 1][0] != '-'

enum class BLOCK_TYPE { block, no_block, force_block };

#define GCSDOKAN_VERSION "1.0.0"
static BLOCK_TYPE block_status = BLOCK_TYPE::block;

static void show_usage() {
	// clang-format off
	fprintf(stderr, "GCSDokan.exe - Mount a Google Cloud Storage bucket to a virtual driver or folder.\n"
		"Usage: GCSDokan remote mountPoint [arguments]\n"
		"Arguments:\n"
		"  -k, --key <path>         Use a service account credentials file to authenticate with google.\n"
		"  -b, --billing <project>  Set the billing project.\n"
		"  -r, --refresh <time>     The refresh rate of the virtual files in seconds(default 60s). The changes \n"
		"                           in the bucket will not be visible until the local information is expired.\n"
		"  -nb, --noBlock           Running GCSDokan in the background.\n"
		"  -l, --list               List all mounted bucket\n"
		"  -u, --unmount <path>     Unmount a GCSDokan mount point <path>\n"
		"  -v, --verbose            Verbose mode.\n"
		"  -h, --help               Show this help page.\n"
		"  --version                Show the version.\n"
		"\n"
		"Cache options(Default: memory cache):\n"
		"  -nc, --noCache           Do not use cache.\n"
		"  -dc, --diskCache [cache directory] \n"
		"                           Use disk cache, the argument cache directory detemines\n"
		"                           the location of the cache file. The default is a temporary directory.\n"
		"  -mc, --memoryCache [cache size]\n"
		"                           Use memory cache, the argument cache size limits the maximum\n"
		"                           amount of the memory usage in MB. The default is 100MB.\n"
		"\n"
		"Examples:\n"
		"  GCSDokan genomics-public-data Z\n"
		"  GCSDokan gs:://genomics-public-data/clinvar Z -v -mc 200\n"
		"\n"
		"Unmount the drive with \"GCSDokan -u MountPoint\" or Ctrl + C if the program is running in the foreground.\n");
	// clang-format on
}

static void run_program_in_background() {
	wstring command(GetCommandLine());
	command = command + L" -secretBlock";
	WCHAR* command_buffer = DBG_NEW WCHAR[command.length() + 1];
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

static bool match_argument(string argument, string value) {
	boost::to_lower(value);
	return argument == value;
}
static bool match_argument(string argument, string value1, string value2) {
	return match_argument(argument, value1) || match_argument(argument, value2);
}

#define PROCESS_SUCCESS 0
#define PROCESS_FAILURE 1
#define QUIT_PROGRAM 2


static int process_arguments(int argc, char* argv[]) {
	if (argc == 1) {
		show_usage();
		return QUIT_PROGRAM;
	}

	bool temporary_cache_path = false;
	int count = 0;
	for (int command = 1; command < argc; command++) {
		string command_str(argv[command]);
		//Basic command: bucket mount_point
		if (command_str.at(0) != '-') {
			if (count == 0) {
				set_remote_link(command_str);
				if (get_remote_link().length() == 0) {
					error_print(L"Invalid remote path\n");
					return PROCESS_FAILURE;
				}
				count++;
				continue;
			}
			if (count == 1) {
				set_mount_point(argv[command]);
				if (get_mount_point().length() == 0) {
					error_print(L"Invalid mount point\n");
					return PROCESS_FAILURE;
				}
				count++;
				continue;
			}
		}
		boost::to_lower(command_str);
		//credentials file
		if (match_argument(command_str, "-k", "--key")) {
			CHECK_CMD_ARG(command, argc);
			set_credentials_path(argv[command]);
			continue;
		}
		//billing project
		if (match_argument(command_str, "-b", "--billing")) {
			CHECK_CMD_ARG(command, argc);
			set_billing_project(argv[command]);
			continue;
		}
		//refresh rate
		if (match_argument(command_str, "-r", "--refresh")) {
			CHECK_CMD_ARG(command, argc);
			set_refresh_interval(std::stoi(argv[command]));
			continue;
		}
		//verbose
		if (match_argument(command_str, "-v", "--verbose")) {
			verbose_level = 1;
			continue;
		}
		//help page
		if (match_argument(command_str, "-h", "--help")) {
			show_usage();
			return QUIT_PROGRAM;
		}
		//version
		if (match_argument(command_str, "--version")) {
			printf("Version: " GCSDOKAN_VERSION);
			return QUIT_PROGRAM;
		}
		if (match_argument(command_str, "--noBlock") && block_status != BLOCK_TYPE::force_block) {
			block_status = BLOCK_TYPE::no_block;
			continue;
		}
		//This argument will force the program running in the current process
		if (match_argument(command_str, "--secretBlock")) {
			block_status = BLOCK_TYPE::force_block;
			continue;
		}
		//disk cache
		if (match_argument(command_str, "-dc", "--diskCache")) {
			if (HAS_OPTIONS(commad, argc)) {
				CHECK_CMD_ARG(command, argc);
				set_cache_root(stringToWstring(argv[command]));
				set_cache_type(CACHE_TYPE::disk);
				continue;
			}
			else {
				temporary_cache_path = true;
				continue;
			}
		}
		//memory cache
		if (match_argument(command_str, "-mc", "--memoryCache")) {
			set_cache_type(CACHE_TYPE::memory);
			if (HAS_OPTIONS(commad, argc)) {
				CHECK_CMD_ARG(command, argc);
				try {
					memory_cache::memory_cache_block_number = std::stoi(argv[command]);
				}
				catch (std::exception ex) {
					error_print("unknown memory cache option: %s\n", argv[command]);
					return PROCESS_FAILURE;
				}
			}
			continue;
		}
		//no cache
		if (match_argument(command_str, "-nc", "--noCache")) {
			set_cache_type(CACHE_TYPE::none);
			continue;
		}
		if (match_argument(command_str, "-l", "--list")) {
			if (!list_instances()) {
				return PROCESS_FAILURE;
			}
			else {
				return QUIT_PROGRAM;
			}
		}
		if (match_argument(command_str, "-u", "--unmount")) {
			CHECK_CMD_ARG(command, argc);
			set_mount_point(argv[command]);
			kill_process();
			return QUIT_PROGRAM;
		}
		error_print("unknown command: %s\n", argv[command]);
		return PROCESS_FAILURE;
	}

	//If the user uses disk cache but does not provide a cache path
	if (temporary_cache_path) {
		char buffer[DIRECTORY_MAX_PATH];
		size_t len = GetTempPathA(DIRECTORY_MAX_PATH, buffer);
		if (len == 0 || len > DIRECTORY_MAX_PATH) {
			error_print("Fail to get a temporary path\n");
			return PROCESS_FAILURE;
		}
		std::hash<wstring> hash_object;
		set_cache_root(stringToWstring(buffer) + L"GCSDokan" + std::to_wstring(hash_object(get_remote_link())));
		set_cache_type(CACHE_TYPE::disk);
	}


	debug_print(L"verbose mode on\n");
	debug_print(L"remote path: %ls\n", get_remote_link().c_str());
	debug_print(L"mount point: %ls\n", get_mount_point().c_str());
	debug_print(L"refresh interval: %ds\n", get_refresh_interval());
	switch (get_cache_type()) {
	case CACHE_TYPE::disk:
		debug_print(L"Disk cache: %ls\n", get_cache_root().c_str());
		break;
	case CACHE_TYPE::memory:
		debug_print(L"Memory cache size: %dMB\n", memory_cache::memory_cache_block_number);
		break;
	case CACHE_TYPE::none:
		debug_print(L"Cache is disabled\n");
		break;
	}
	return PROCESS_SUCCESS;
}


int main(int argc, char* argv[])
{
	//Try to obtain and verify the arguments in the main process
	int result = process_arguments(argc, argv);
	if (result == PROCESS_FAILURE) {
		return EXIT_FAILURE;
	}
	if (result == QUIT_PROGRAM) {
		return EXIT_SUCCESS;
	}

	//If no block is specified, running dokan in another process
	if (block_status == BLOCK_TYPE::no_block) {
		run_program_in_background();
		return 0;
	}
	//Authenticate with google 
	auto_auth();

	google::cloud::storage::v1::oauth2::Credentials& credentials = get_credentials();
	if(credentials.AccountEmail().length()!=0)
		debug_print("Account:%s\n", credentials.AccountEmail().c_str());

	//Manage the life time of GCSDokan
	if (!start_deamon()) {
		return ERROR_ACCESS_DENIED;
	}

	//Run dokan program
	result = run_dokan();
	if (result != 0) {
		error_print(L"Fail to mount the virtual disk, error code: %d\n", result);
	}
	return result;
}


