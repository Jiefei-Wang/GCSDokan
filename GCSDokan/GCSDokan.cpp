//#include "utilities.h"
#include "googleAuth.h"
#include "fileManager.h"
#include "dokan.h"

//void test1();
int test();

using std::string;
int main(int argc, char* argv[])
{
    gcloud_auth();
    //auto res = get_file_meta(LR"(\myfolder\desktop.ini)");
    int result = test();
    //auto result = exist_folder(LR"(\tt)");
    //auto result3 = get_file_meta(LR"(\testfile1.txt)");
    //auto result1 = get_folder_meta(LR"(\)");
    //auto result2 = get_folder_meta(L"\\1000-genomes-phase-3\\vcf");
    //auto result3 = get_file_meta(LR"(\1000-genomes\bam)");
    
    /*
    size_t buff_len = 10;
    char* buf = new char[buff_len+1];
    buf[buff_len] = '\0';
    get_file_data(LR"(\testFile.txt)",buf,buff_len,0);

    string str(buf);

    delete[] buf;*/
    system("pause");
    return 0;
}

static WCHAR MountPoint[DOKAN_MAX_PATH];
int test()
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

    wcscpy_s(MountPoint, sizeof(MountPoint) / sizeof(WCHAR), L"R:\\");
    dokanOptions.MountPoint = MountPoint;
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



