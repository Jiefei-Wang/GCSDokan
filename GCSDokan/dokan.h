#pragma once
#include "dokan/dokan.h"

#ifdef WIN10_ENABLE_LONG_PATH
//dirty but should be enough
#define DOKAN_MAX_PATH 32768
#else
#define DOKAN_MAX_PATH MAX_PATH
#endif // DEBUG


NTSTATUS DOKAN_CALLBACK
cloud_create_file(LPCWSTR _fileName, PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
    ACCESS_MASK DesiredAccess, ULONG _fileAttributes,
    ULONG ShareAccess, ULONG CreateDisposition,
    ULONG CreateOptions, PDOKAN_FILE_INFO Dokan_fileInfo);



void DOKAN_CALLBACK cloud_close_file(LPCWSTR _fileName,
    PDOKAN_FILE_INFO Dokan_fileInfo);


void DOKAN_CALLBACK cloud_cleanup(LPCWSTR _fileName,
    PDOKAN_FILE_INFO Dokan_fileInfo);



NTSTATUS DOKAN_CALLBACK cloud_read_file(LPCWSTR _fileName, LPVOID Buffer,
    DWORD BufferLength,
    LPDWORD ReadLength,
    LONGLONG Offset,
    PDOKAN_FILE_INFO Dokan_fileInfo);

NTSTATUS DOKAN_CALLBACK cloud_write_file(LPCWSTR _fileName, LPCVOID Buffer,
    DWORD NumberOfBytesToWrite,
    LPDWORD NumberOfBytesWritten,
    LONGLONG Offset,
    PDOKAN_FILE_INFO Dokan_fileInfo);


NTSTATUS DOKAN_CALLBACK
cloud_flush_buffers(LPCWSTR _fileName, PDOKAN_FILE_INFO Dokan_fileInfo);


NTSTATUS DOKAN_CALLBACK cloud_get_file_information(
    LPCWSTR _fileName, LPBY_HANDLE_FILE_INFORMATION Handle_fileInformation,
    PDOKAN_FILE_INFO Dokan_fileInfo);


NTSTATUS DOKAN_CALLBACK
cloud_find_files(LPCWSTR _fileName,
    PFillFindData FillFindData, // function pointer
    PDOKAN_FILE_INFO Dokan_fileInfo);

NTSTATUS DOKAN_CALLBACK cloud_get_colume_info(
    LPWSTR volumename_buffer, DWORD volumename_size,
    LPDWORD volume_serialnumber, LPDWORD maximum_component_length,
    LPDWORD filesystem_flags, LPWSTR filesystem_name_buffer,
    DWORD filesystem_name_size, PDOKAN_FILE_INFO);