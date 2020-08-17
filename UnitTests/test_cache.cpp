#include "pch.h"
#include "CppUnitTest.h"
#include "../GCSDokan/fileCache.h"
#include "../GCSDokan/memoryCache.h"
#include <boost/filesystem.hpp>
#include <Windows.h>
#include "string"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using std::string;
using std::wstring;

#define data_size 1024 * 10


int data[data_size];
int old_id = -1;
size_t read_count = 0;
size_t read_offset = 0;
size_t read_length = 0;

void download(void* id, void* buffer, size_t offset, size_t length) {
	read_count++;
	read_offset = offset;
	read_length = length;
	int ID = *(int*)id;
	if (old_id != ID) {
		for (int i = 0; i < data_size; i++) {
			data[i] = i * ID;
		}
	}
	memcpy_s(buffer, length, ((char*)data) + offset, length);
}

template<class T>
void file_cache_test_method(T* cache, int id, unsigned int block_size) {
	read_count = 0;
	read_offset = 0;
	read_length = 0;
	int read_data[100];
	size_t offset;
	size_t length;

	offset = 0;
	length = 100;
	cache->read_data((char*)read_data, offset, length);
	Assert::IsTrue(read_count == 1);
	Assert::IsTrue(read_offset == offset);
	Assert::IsTrue(read_length == block_size);
	for (auto i = 0; i < length / sizeof(int); i++) {
		Assert::IsTrue(read_data[i] == i * id);
	}

	size_t offset1;
	offset1 = offset + 100;
	cache->read_data((char*)read_data, offset1, length);
	Assert::IsTrue(read_count == 1);
	Assert::IsTrue(read_offset == offset);
	Assert::IsTrue(read_length == block_size);
	for (auto i = 0; i < length / sizeof(int); i++) {
		Assert::IsTrue(read_data[i] == (i + offset1 / sizeof(int)) * id);
	}

	offset = block_size * 2;
	length = 100;
	cache->read_data((char*)read_data, offset, length);
	Assert::IsTrue(read_count == 2);
	Assert::IsTrue(read_offset == offset);
	Assert::IsTrue(read_length == block_size);
	for (auto i = 0; i < length / sizeof(int); i++) {
		Assert::IsTrue(read_data[i] == (i + offset / sizeof(int)) * id);
	}


	offset1 = offset + 100;
	cache->read_data((char*)read_data, offset1, length);
	Assert::IsTrue(read_count == 2);
	Assert::IsTrue(read_offset == offset);
	Assert::IsTrue(read_length == block_size);
	for (auto i = 0; i < length / sizeof(int); i++) {
		Assert::IsTrue(read_data[i] == (i + offset1 / sizeof(int)) * id);
	}

	offset = block_size + 100;
	length = 100;
	cache->read_data((char*)read_data, offset, length);
	Assert::IsTrue(read_count == 3);
	Assert::IsTrue(read_offset == block_size);
	Assert::IsTrue(read_length == block_size);
	for (auto i = 0; i < length / sizeof(int); i++) {
		Assert::IsTrue(read_data[i] == (i + offset / sizeof(int)) * id);
	}

}


namespace GCSDokan
{
	TEST_CLASS(cache_system)
	{
	public:
		TEST_METHOD(file_cache_test)
		{
			char buffer[1024];
			size_t len = GetTempPathA(1024, buffer);
			Assert::IsTrue(len > 0 && len < 1024);

			string cache_path(buffer);
			cache_path = cache_path + "GCSDokan";

			int id = 1;
			unsigned int block_size = 1024;
			boost::filesystem::remove_all(cache_path);

			file_cache* cache = new file_cache(
				cache_path,
				std::to_string(id),
				data_size * sizeof(int),
				&download,
				&id,
				block_size);
			file_cache_test_method(cache, id, block_size);
			delete cache;

			//Change file identifier
			id = 2;
			cache = new file_cache(
				cache_path,
				std::to_string(id),
				data_size * sizeof(int),
				&download,
				&id,
				block_size);
			file_cache_test_method(cache, id, block_size);
			delete cache;
		}

		TEST_METHOD(memory_cache_test)
		{

			int id = 1;
			unsigned int block_size = 1024;

			memory_cache* cache = new memory_cache(
				std::to_string(id),
				data_size * sizeof(int),
				&download,
				&id,
				block_size);
			file_cache_test_method(cache, id, block_size);
			delete cache;

			//Change file identifier
			id = 2;
			cache = new memory_cache(
				std::to_string(id),
				data_size * sizeof(int),
				&download,
				&id,
				block_size);
			file_cache_test_method(cache, id, block_size);
			delete cache;
		}

	};
}
