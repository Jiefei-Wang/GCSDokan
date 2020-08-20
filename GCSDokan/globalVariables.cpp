#include "globalVariables.h"
#include "utilities.h"
using std::wstring;
using std::string;

static wstring mount_point;
short verbose_level = 0;

void set_mount_point(std::wstring path) {
	mount_point = remove_tailing_symbol(remove_tailing_slash(to_linux_slash(path)),':');
}

std::wstring& get_mount_point() {
	return mount_point;
}


