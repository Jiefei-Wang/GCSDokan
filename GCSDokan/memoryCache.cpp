#include <memory>
#include "memoryCache.h"
#define USE_BOOST_OPTIONAL
#include "stlCache/stlcache.hpp"
#include "globalVariables.h"

using std::string;
using std::shared_ptr;


#ifdef UINT
#undef UINT
#endif
#define UINT unsigned int

#ifdef min
#undef min
#endif
#define min(a,b)            (((a) < (b)) ? (a) : (b))

unsigned int memory_cache::default_block_size = 1 * 1024 * 1024;
unsigned int memory_cache::random_access_tolerance_time = 5;
unsigned int memory_cache::random_access_range_cutoff = default_block_size / 2;

std::mutex memory_cache::static_mutex;
typedef stlcache::cache<string, shared_ptr<char[]>,stlcache::policy_lru> CacheLRU;
CacheLRU* cache = nullptr;

struct block_read_info {
	unsigned int file_id;
	unsigned int file_byte_offset;
	unsigned int file_read_length;
};


/*
=====================================================================
					utilities
=====================================================================
*/
static UINT compute_block_number(size_t file_size, UINT block_size) {
	return (UINT)(std::ceil(file_size / (double)block_size));
}

static block_read_info get_block_read_info(
	size_t offset,
	size_t read_size,
	size_t block_size) {
	UINT file_id = (UINT)(offset / block_size);
	UINT file_byte_offset = (UINT)(offset - file_id * (size_t)block_size);
	UINT read_length = (UINT)min(block_size > file_byte_offset ? (size_t)block_size - file_byte_offset : 0,
		read_size);

	block_read_info read_info;
	read_info.file_id = file_id;
	read_info.file_byte_offset = file_byte_offset;
	read_info.file_read_length = (UINT)read_length;
	return read_info;
}

static size_t get_block_expected_size(size_t total_size, size_t  block_size, size_t block_id) {
	size_t offset = block_id * block_size;
	if (offset > total_size)
		return 0;
	else
		return min(block_size, total_size - block_id * block_size);
}
std::string get_block_key(string file_identifier, unsigned int block_id)
{
	return file_identifier + "_" + std::to_string(block_id);
}

const shared_ptr<char[]>& get_block_ptr(string block_key) {
	return cache->fetch(block_key);
}

bool block_exists(string block_key) {
	return cache->count(block_key);
}

/*
=====================================================================
					private nonstatic functions
=====================================================================
*/

bool memory_cache::is_random_read(size_t offset, size_t read_size) {
	file_mutex.lock();
	size_t lower_bound = last_read_offset > random_access_range_cutoff ?
		last_read_offset - random_access_range_cutoff : 0;
	size_t upper_bound = last_read_offset + last_read_size + random_access_range_cutoff;
	if (!(offset <= upper_bound && lower_bound <= offset + read_size)) {
		random_read_time++;
	}
	else {
		random_read_time = 0;
	}
	last_read_offset = offset;
	last_read_size = read_size;
	file_mutex.unlock();
	return random_read_time > random_access_tolerance_time && read_size < block_size / 8;
}




/*
=====================================================================
					public functions
=====================================================================
*/
memory_cache::memory_cache(std::string file_identifier, size_t file_size,
	read_data_func data_func, void* file_info, unsigned int block_size)
{
	this->data_func = data_func;
	this->file_info = file_info;

	this->file_identifier = file_identifier + std::to_string(file_size);
	this->file_size = file_size;
	if (block_size != 0)
		this->block_size = block_size;
	else
		this->block_size = default_block_size;
	this->block_number = compute_block_number(file_size, this->block_size);
	static_mutex.lock();
	if (cache == nullptr) {
		cache = new CacheLRU(memory_cache_block_number);
	}
	static_mutex.unlock();
}

memory_cache::~memory_cache() {
}




bool memory_cache::read_data(char* buffer, size_t offset, size_t read_size) {
	bool success = true;
	bool random_read = is_random_read(offset, read_size);
	if (random_read) {
		try {
			(*data_func)(file_info, buffer, offset, read_size);
		}
		catch (std::exception e) {
			success = false;
		}
	}

	while (read_size != 0 && success) {
		block_read_info read_info = get_block_read_info(offset, read_size, block_size);
		UINT target_block = read_info.file_id;
		string block_key = get_block_key(file_identifier, target_block);

		//If the data does not exist, we download it from the cloud
		if (!block_exists(block_key)) {
			shared_ptr<char[]> block_ptr(new char[block_size]);
			size_t cloud_file_offset = target_block * block_size;
			size_t cloud_file_len = get_block_expected_size(file_size, block_size, target_block);
			try {
				(*data_func)(file_info, block_ptr.get(), cloud_file_offset, cloud_file_len);
			}
			catch (std::exception e) {
				success = false;
				break;
			}
			//We only lock the cache when we do operation on it
			static_mutex.lock();
			cache->insert(block_key, block_ptr);
			static_mutex.unlock();
		}
		// Copy the data from cache to the pointer
		size_t target_read_size = read_info.file_read_length;
		const shared_ptr<char[]>& block_ptr = get_block_ptr(block_key);
		char* target_cache_ptr = block_ptr.get() + read_info.file_byte_offset;
		memcpy_s(buffer, target_read_size, target_cache_ptr, target_read_size);

		buffer = buffer + target_read_size;
		offset = offset + target_read_size;
		read_size = read_size - target_read_size;
	}
	return success;
}

void* memory_cache::get_file_info()
{
	return file_info;
}

void memory_cache::set_file_info(void* file_info)
{
	this->file_info = file_info;
}

