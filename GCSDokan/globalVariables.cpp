#include "globalVariables.h"
#include "utilities.h"
using std::wstring;
using std::string;

//wstring remote_link = L"genomics-public-data";
wstring remote_link = L"";// L"bioconductor_test";
wstring mount_point = L"";//L"R:\\";
wstring cache_root = L"D:\\test";
size_t update_interval = 60 * 10;

CACHE_TYPE cache_type = CACHE_TYPE::none;

short verbose_level = 0;

void set_mount_point(std::wstring path) {
	mount_point = to_linux_slash(path);
}

void set_remote_link(std::wstring link) {
	remote_link = link;
}