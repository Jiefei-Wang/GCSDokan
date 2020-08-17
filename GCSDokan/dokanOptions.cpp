#include "dokanOperations.h"
#include "globalVariables.h"

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
	dokanOptions.MountPoint = get_mount_point().c_str();
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