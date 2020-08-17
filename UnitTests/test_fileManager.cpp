#include "pch.h"
#include "CppUnitTest.h"
#include "../GCSDokan/googleAuth.h"
#include "../GCSDokan/fileManager.h"
#include "../GCSDokan/utilities.h"
#include "../GCSDokan/fileCache.h"
#include "../GCSDokan/memoryCache.h"

#include <Windows.h>
#include "string"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using std::string;
using std::wstring;


#define buffer_len 1024

void file_download_test(file_manager_handle& handle, wstring file_full_path, 
	char* buffer1, char* buffer2, size_t offset, size_t read_len) {

	gcs_read_file(file_full_path, buffer1, offset, read_len);
	get_file_data(handle, buffer2, offset, read_len);
	for (int i = 0; i < read_len; i++) {
		Assert::AreEqual(buffer1[i], buffer2[i]);
	}
}


namespace GCSDokan
{
	TEST_CLASS(file_manager)
	{
	public:
		TEST_METHOD(download_data)
		{
			auth_anonymous();
			wstring bucket(L"genomics-public-data");
			wstring remote_link = bucket + L"/clinvar";
			wstring file(L"variant_summary.txt");
			wstring file_full_path = build_path(remote_link , file);

			char buffer[1024];
			size_t len = GetTempPathA(1024, buffer);
			Assert::IsTrue(len > 0 && len < 1024);
			string cache_path(buffer);
			set_cache_root(cache_path + "GCSDokan");
			set_remote_link(remote_link);



			char buffer1[buffer_len];
			char buffer2[buffer_len];

			//Test for none cache
			set_cache_type(CACHE_TYPE::none);
			file_manager_handle handle = get_file_manager_handle(file);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024*4, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 2 + 10, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024* 2 + 10, 1024);
			release_file_manager_handle(handle);

			//Test for disk cache
			file_cache::default_block_size = 1024;
			set_cache_type(CACHE_TYPE::disk);
			handle = get_file_manager_handle(file);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 4, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 2 + 10, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 2 + 10, 1024);
			release_file_manager_handle(handle);


			//Test for memory cache
			memory_cache::default_block_size = 1024;
			set_cache_type(CACHE_TYPE::memory);
			handle = get_file_manager_handle(file);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 0, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 4, 1024);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 2 + 10, 10);
			file_download_test(handle, file_full_path, buffer1, buffer2, 1024 * 2 + 10, 1024);
			release_file_manager_handle(handle);
		}


	};
}
