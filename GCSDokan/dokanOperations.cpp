#include <malloc.h>
#include "dokanOperations.h"
#include "fileManager.h"
#include "utilities.h"
#include "dataTypes.h"

using std::wstring;


static int event_count = 0;

#define CheckFlag(val, flag)                                             \
  if (val & flag) {                                                            \
    debug_print(L"\t" L#flag L"\n");                                              \
  }

#define CheckVarEqual(val, flag)                                             \
  if (val == flag) {                                                            \
    debug_print(L"\t" L#flag L"\n");                                              \
  }

#define containFlag(val,flag)\
(val&flag)


#define WCSMATCH(x, y) ((wcslen(x) == wcslen(y)) && wcscmp(x,y)==0)


void print_flag(ULONG share_access, ACCESS_MASK desired_access,
	DWORD attr_and_flags, DWORD creation_disposition) {
	if (verbose_level < 2) return;
	debug_print(L"\tShareMode = 0x%x\n", share_access);
	CheckFlag(share_access, FILE_SHARE_READ);
	CheckFlag(share_access, FILE_SHARE_WRITE);
	CheckFlag(share_access, FILE_SHARE_DELETE);

	debug_print(L"\tDesiredAccess = 0x%x\n", desired_access);
	CheckFlag(desired_access, GENERIC_READ);
	CheckFlag(desired_access, GENERIC_WRITE);
	CheckFlag(desired_access, GENERIC_EXECUTE);
	CheckFlag(desired_access, DELETE);
	CheckFlag(desired_access, FILE_READ_DATA);
	CheckFlag(desired_access, FILE_READ_ATTRIBUTES);
	CheckFlag(desired_access, FILE_READ_EA);
	CheckFlag(desired_access, READ_CONTROL);
	CheckFlag(desired_access, FILE_WRITE_DATA);
	CheckFlag(desired_access, FILE_WRITE_ATTRIBUTES);
	CheckFlag(desired_access, FILE_WRITE_EA);
	CheckFlag(desired_access, FILE_APPEND_DATA);
	CheckFlag(desired_access, WRITE_DAC);
	CheckFlag(desired_access, WRITE_OWNER);
	CheckFlag(desired_access, SYNCHRONIZE);
	CheckFlag(desired_access, FILE_EXECUTE);
	CheckFlag(desired_access, STANDARD_RIGHTS_READ);
	CheckFlag(desired_access, STANDARD_RIGHTS_WRITE);
	CheckFlag(desired_access, STANDARD_RIGHTS_EXECUTE);

	debug_print(L"\tFlagsAndAttributes = 0x%x\n", attr_and_flags);

	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_ARCHIVE);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_COMPRESSED);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_DEVICE);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_DIRECTORY);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_ENCRYPTED);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_HIDDEN);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_INTEGRITY_STREAM);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_NORMAL);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_NO_SCRUB_DATA);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_OFFLINE);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_READONLY);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_REPARSE_POINT);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_SPARSE_FILE);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_SYSTEM);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_TEMPORARY);
	CheckFlag(attr_and_flags, FILE_ATTRIBUTE_VIRTUAL);
	CheckFlag(attr_and_flags, FILE_FLAG_WRITE_THROUGH);
	CheckFlag(attr_and_flags, FILE_FLAG_OVERLAPPED);
	CheckFlag(attr_and_flags, FILE_FLAG_NO_BUFFERING);
	CheckFlag(attr_and_flags, FILE_FLAG_RANDOM_ACCESS);
	CheckFlag(attr_and_flags, FILE_FLAG_SEQUENTIAL_SCAN);
	CheckFlag(attr_and_flags, FILE_FLAG_DELETE_ON_CLOSE);
	CheckFlag(attr_and_flags, FILE_FLAG_BACKUP_SEMANTICS);
	CheckFlag(attr_and_flags, FILE_FLAG_POSIX_SEMANTICS);
	CheckFlag(attr_and_flags, FILE_FLAG_OPEN_REPARSE_POINT);
	CheckFlag(attr_and_flags, FILE_FLAG_OPEN_NO_RECALL);
	CheckFlag(attr_and_flags, SECURITY_ANONYMOUS);
	CheckFlag(attr_and_flags, SECURITY_IDENTIFICATION);
	CheckFlag(attr_and_flags, SECURITY_IMPERSONATION);
	CheckFlag(attr_and_flags, SECURITY_DELEGATION);
	CheckFlag(attr_and_flags, SECURITY_CONTEXT_TRACKING);
	CheckFlag(attr_and_flags, SECURITY_EFFECTIVE_ONLY);
	CheckFlag(attr_and_flags, SECURITY_SQOS_PRESENT);

	debug_print(L"creationDisposition:\n");
	if (creation_disposition == CREATE_NEW) {
		debug_print(L"\tCREATE_NEW\n");
	}
	else if (creation_disposition == OPEN_ALWAYS) {
		debug_print(L"\tOPEN_ALWAYS\n");
	}
	else if (creation_disposition == CREATE_ALWAYS) {
		debug_print(L"\tCREATE_ALWAYS\n");
	}
	else if (creation_disposition == OPEN_EXISTING) {
		debug_print(L"\tOPEN_EXISTING\n");
	}
	else if (creation_disposition == TRUNCATE_EXISTING) {
		debug_print(L"\tTRUNCATE_EXISTING\n");
	}
	else {
		debug_print(L"\tUNKNOWN creationDisposition!\n");
	}
}


NTSTATUS DOKAN_CALLBACK
cloud_create_file(LPCWSTR file_name, PDOKAN_IO_SECURITY_CONTEXT security_context,
	ACCESS_MASK desired_access, ULONG file_attributes,
	ULONG share_access, ULONG create_disposition,
	ULONG create_options, PDOKAN_FILE_INFO dokan_file_info) {
	NTSTATUS status = STATUS_SUCCESS;
	DWORD creation_disposition;
	DWORD attr_and_flags;
	ACCESS_MASK generic_desired_access;


	file_private_info* cache_info = new file_private_info();
	cache_info->event_id = event_count++;
	dokan_file_info->Context= (ULONG64)cache_info;
	DokanMapKernelToUserCreateFileFlags(
		desired_access, file_attributes, create_options, create_disposition,
		&generic_desired_access, &attr_and_flags, &creation_disposition);
	debug_print(L"### CreateFile : %d\n", ((file_private_info*)dokan_file_info->Context)->event_id);
	debug_print(L"file name : %s\n", file_name);
	if(endsWith(file_name, L"System Volume Information")||
		endsWith(file_name, L"$RECYCLE.BIN")||
		endsWith(file_name, L"desktop.ini")) {
		debug_print(L"auto created file, no such file\n");
		return STATUS_ACCESS_DENIED;
	}
	print_flag(share_access, desired_access, attr_and_flags, creation_disposition);

	if (containFlag(desired_access, GENERIC_WRITE) ||
		containFlag(desired_access, FILE_WRITE_DATA) ||
		containFlag(desired_access, FILE_WRITE_ATTRIBUTES) ||
		containFlag(desired_access, FILE_WRITE_EA)
		) {
		debug_print(L"cannot write data\n");
	}

	bool exist_path = true;
	if (wcslen(file_name)!=1&&exist_file(file_name)) {
		dokan_file_info->IsDirectory = false;
		if (create_options & FILE_DIRECTORY_FILE) {
			debug_print(L"error: file is not a directory\n");
			return STATUS_NOT_A_DIRECTORY;
		}
		debug_print(L"Directory is a file\n");
	}
	else if (exist_folder(file_name)) {
		dokan_file_info->IsDirectory = true;
		if (create_options & FILE_NON_DIRECTORY_FILE) {
			debug_print(L"error: file is a directory\n");
			return STATUS_FILE_IS_A_DIRECTORY;
		}
		debug_print(L"Directory is a folder\n");
	}
	else {
		exist_path = false;
	}

	if (exist_path) {
		if (creation_disposition == OPEN_ALWAYS) {
			status = ERROR_ALREADY_EXISTS;
		}
		if (creation_disposition == OPEN_EXISTING) {
			status = STATUS_SUCCESS;
		}
		if (creation_disposition == CREATE_NEW) {
			status = ERROR_FILE_EXISTS;
		}
		if (creation_disposition == CREATE_ALWAYS) {
			status = STATUS_ACCESS_DENIED;
		}
		if (creation_disposition == TRUNCATE_EXISTING) {
			status = STATUS_ACCESS_DENIED;
		}
	}
	else {
		if (creation_disposition == CREATE_NEW ||
			creation_disposition == CREATE_ALWAYS ||
			creation_disposition == TRUNCATE_EXISTING ||
			creation_disposition == OPEN_ALWAYS) {
			status = STATUS_ACCESS_DENIED;
		}
		if (creation_disposition == OPEN_EXISTING) {
			status = ERROR_FILE_NOT_FOUND;
		}
	}
	debug_print(L"status code:\n");
	CheckVarEqual(status, ERROR_ALREADY_EXISTS);
	CheckVarEqual(status, STATUS_SUCCESS);
	CheckVarEqual(status, ERROR_FILE_EXISTS);
	CheckVarEqual(status, STATUS_ACCESS_DENIED);
	CheckVarEqual(status, ERROR_FILE_NOT_FOUND);


	debug_print(L"end of createFile function\n");
	return status;
}



void DOKAN_CALLBACK cloud_close_file(LPCWSTR file_name,
	PDOKAN_FILE_INFO dokan_file_info) {
	file_private_info* private_info = (file_private_info*)dokan_file_info->Context;
	debug_print(L"### CloseFile : %llu\n", private_info->event_id);
	debug_print(L"file name: %s\n", file_name);
}


void DOKAN_CALLBACK cloud_cleanup(LPCWSTR file_name,
	PDOKAN_FILE_INFO dokan_file_info) {
	file_private_info* private_info = (file_private_info*)dokan_file_info->Context;
	debug_print(L"### Cleanup : %llu\n", private_info->event_id);
	debug_print(L"file name: %s\n", file_name);
	if (private_info->manager_handle != nullptr) {
		release_file_manager_handle(private_info->manager_handle);
	}
}



NTSTATUS DOKAN_CALLBACK cloud_read_file(LPCWSTR file_name, LPVOID buffer,
	DWORD buffer_length,
	LPDWORD read_len,
	LONGLONG offset,
	PDOKAN_FILE_INFO dokan_file_info) {
	file_private_info* private_info = (file_private_info*)dokan_file_info->Context;
	debug_print(L"### read file : %llu\n", private_info->event_id);
	debug_print(L"file name: %s, offset: %llu, len:%llu\n", file_name ,offset, buffer_length);
	file_meta_info meta = get_file_meta(file_name);
	LONGLONG len = meta.size - offset;
	if (len < 0) {
		len = 0;
	}
	size_t true_read_len = len;
	if (true_read_len > buffer_length) {
		true_read_len = buffer_length;
	}
	*read_len = (DWORD)true_read_len;
	if (true_read_len == 0) {
		return STATUS_SUCCESS;
	}
	//Check whether it is a random access or not
	/*
	}*/
	if (private_info->manager_handle == nullptr) {
		private_info->manager_handle = get_file_manager_handle(file_name);
	}
	//auto handle = get_file_manager_handle(file_name);
	NTSTATUS status = get_file_data(private_info->manager_handle, buffer, offset, true_read_len);
	//release_file_manager_handle(handle);
	debug_print(L"final read size:%llu\n", true_read_len);
	return status;
}

NTSTATUS DOKAN_CALLBACK cloud_write_file(LPCWSTR file_name, LPCVOID buffer,
	DWORD NumberOfBytesToWrite,
	LPDWORD NumberOfBytesWritten,
	LONGLONG Offset,
	PDOKAN_FILE_INFO dokan_file_info) {
	debug_print(L"### WriteFile : %llu\n", ((file_private_info*)dokan_file_info->Context)->event_id);
	debug_print(L"file name: %s\n", file_name);
	return ERROR_ACCESS_DENIED;
}


NTSTATUS DOKAN_CALLBACK
cloud_flush_buffers(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info) {
	debug_print(L"### FlushFileBuffers : %llu\n", ((file_private_info*)dokan_file_info->Context)->event_id);
	debug_print(L"file name: %s\n", file_name);
	return STATUS_SUCCESS;
}



NTSTATUS DOKAN_CALLBACK cloud_get_file_information(
	LPCWSTR file_name, LPBY_HANDLE_FILE_INFORMATION handleFileInformation,
	PDOKAN_FILE_INFO dokan_file_info) {
	debug_print(L"### GetFileInfo : %llu\n", ((file_private_info*)dokan_file_info->Context)->event_id);
	debug_print(L"file name: %s\n", file_name);
	if (endsWith(file_name, L"desktop.ini")) {
		debug_print(L"file not found\n");
		return ERROR_FILE_NOT_FOUND;
	}

	//fill_dummy_info(handleFileInformation);
	//handleFileInformation->dwVolumeSerialNumber = 0x19831116;
	if (dokan_file_info->IsDirectory) {
		handleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		handleFileInformation->ftCreationTime = FILETIME{0,0};
		handleFileInformation->ftLastWriteTime = FILETIME{ 0,0 };
		handleFileInformation->ftLastAccessTime = FILETIME{ 0,0 };
	}
	else {
		if (!exist_file(file_name)) {
			debug_print(L"error file is not a folder\n");
			return ERROR_FILE_NOT_FOUND;
		}
		file_meta_info meta = get_file_meta(file_name);
		handleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		unsigned long long size = meta.size;
		LlongToDwLowHigh(size, handleFileInformation->nFileSizeLow, handleFileInformation->nFileSizeHigh);
		handleFileInformation->ftCreationTime = meta.time_created;
		handleFileInformation->ftLastWriteTime = meta.time_updated;
		handleFileInformation->ftLastAccessTime = meta.time_updated;

	}
	return STATUS_SUCCESS;
}


NTSTATUS DOKAN_CALLBACK
cloud_find_files(LPCWSTR file_name,
	PFillFindData fill_find_data, // function pointer
	PDOKAN_FILE_INFO dokan_file_info) {
	debug_print(L"### FindFiles : %llu\n", ((file_private_info*)dokan_file_info->Context)->event_id);
	debug_print(L"file name: %s\n", file_name);
	wstring base_path;
	if (WCSMATCH(file_name, L"\\")) {
		base_path = L"";
	}
	else {
		base_path = file_name;
	}
	WIN32_FIND_DATAW findData;
	folder_meta_info dir_info = get_folder_meta(file_name);
	for (auto i : dir_info.folder_names) {
		findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		wcscpy_s(findData.cFileName, DIRECTORY_MAX_PATH, i.c_str());
		fill_find_data(&findData, dokan_file_info);
	}

	for (auto i : dir_info.file_meta_vector) {
		findData.cAlternateFileName[0] = L'\0';
		unsigned long long size = i.second.size;
		LlongToDwLowHigh(size, findData.nFileSizeLow, findData.nFileSizeHigh);
		findData.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		findData.ftCreationTime = i.second.time_created;
		findData.ftLastWriteTime = i.second.time_updated;
		findData.ftLastAccessTime = i.second.time_updated;
		wcscpy_s(findData.cFileName, DIRECTORY_MAX_PATH, i.second.local_name.c_str());
		fill_find_data(&findData, dokan_file_info);
	}

	return STATUS_SUCCESS;
}


NTSTATUS DOKAN_CALLBACK cloud_get_colume_info(
	LPWSTR volumename_buffer, DWORD volumename_size,
	LPDWORD volume_serialnumber, LPDWORD maximum_component_length,
	LPDWORD filesystem_flags, LPWSTR filesystem_name_buffer,
	DWORD filesystem_name_size, PDOKAN_FILE_INFO /*dokanfileinfo*/) {
	debug_print(L"### getvolumeinformation\n");
	wcscpy_s(volumename_buffer, volumename_size, L"GCSDokan");
	*volume_serialnumber = 0x19831116;
	*maximum_component_length = 255;
	*filesystem_flags = FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES |
		FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK |
		FILE_NAMED_STREAMS;

	wcscpy_s(filesystem_name_buffer, filesystem_name_size, L"NTFS");
	return STATUS_SUCCESS;
}