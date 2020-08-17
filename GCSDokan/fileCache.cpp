#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "fileCache.h"

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define DBG_NEW new
#endif

namespace bf = boost::filesystem;
namespace bi = boost::interprocess;
using std::string;

#ifdef UINT
#undef UINT
#endif
#define UINT unsigned int

#ifdef min
#undef min
#endif
#define min(a,b)            (((a) < (b)) ? (a) : (b))

unsigned int file_cache::default_block_size = 1 * 1024 * 1024;
unsigned int file_cache::random_access_tolerance_time = 5;
unsigned int file_cache::random_access_range_cutoff = default_block_size / 2;

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
static unsigned int get_meta_minimum_size();
static unsigned int get_meta_complete_indicator_offset(char* buffer);
static unsigned int get_meta_identifier_size_offset(char* buffer);
static unsigned int get_meta_block_size_offset(char* buffer);
static unsigned int get_meta_block_number_offset(char* buffer);
static unsigned int get_meta_identifier_offset(char* buffer);
static unsigned int get_meta_block_indicator_offset(char* buffer);
static unsigned int get_meta_total_offset(char* buffer);

static bool& get_meta_complete_indicator(char* buffer);
static unsigned int& get_meta_identifier_size(char* buffer);
static unsigned int& get_meta_block_size(char* buffer);
static unsigned int& get_meta_block_number(char* buffer);
static char* get_meta_identifier_ptr(char* buffer);
static char* get_meta_block_indicator(char* buffer);


static unsigned int get_meta_minimum_size() {
	return get_meta_identifier_offset(nullptr);
}
static UINT get_meta_complete_indicator_offset(char* buffer) {
	return 0;
}
static UINT get_meta_identifier_size_offset(char* buffer) {
	return get_meta_complete_indicator_offset(buffer) + sizeof(bool);
}
static UINT get_meta_block_size_offset(char* buffer) {
	return get_meta_identifier_size_offset(buffer) + +sizeof(UINT);
}
static UINT get_meta_block_number_offset(char* buffer) {
	return get_meta_block_size_offset(buffer) + sizeof(UINT);
}
static UINT get_meta_identifier_offset(char* buffer) {
	return  get_meta_block_number_offset(buffer) + sizeof(UINT);
}
static UINT get_meta_block_indicator_offset(char* buffer) {
	return get_meta_identifier_offset(buffer) + get_meta_identifier_size(buffer);
}
static unsigned int get_meta_total_offset(char* buffer) {
	return get_meta_identifier_offset(buffer) + get_meta_identifier_size(buffer)
		+ (UINT)std::ceil(get_meta_block_number(buffer) / 8.0);
}


static bool& get_meta_complete_indicator(char* buffer) {
	return *((bool*)(buffer + get_meta_complete_indicator_offset(buffer)));
}
static UINT& get_meta_identifier_size(char* buffer) {
	return  *((UINT*)(buffer + get_meta_identifier_size_offset(buffer)));
}
static UINT& get_meta_block_size(char* buffer) {
	return  *((UINT*)(buffer + get_meta_block_size_offset(buffer)));
}
static UINT& get_meta_block_number(char* buffer) {
	return  *((UINT*)(buffer + get_meta_block_number_offset(buffer)));
}
static char* get_meta_identifier_ptr(char* buffer) {
	return  buffer + get_meta_identifier_offset(buffer);
}
static char* get_meta_block_indicator(char* buffer) {
	return  buffer + get_meta_block_indicator_offset(buffer);
}



static size_t get_block_expected_size(size_t total_size, size_t  block_size, size_t block_id) {
	size_t offset = block_id * block_size;
	if (offset > total_size)
		return 0;
	else
		return min(block_size, total_size - block_id * block_size);
}

static void close_file(bi::file_mapping* file_handle, bi::mapped_region* map_handle) {
	if (map_handle != nullptr) {
		auto handle = (bi::mapped_region*) map_handle;
		handle->flush();
		delete handle;
	}
	if (file_handle != nullptr) {
		auto handle = (bi::file_mapping*) file_handle;
		delete handle;
	}

}
static size_t compute_meta_file_size(size_t identifier_size, size_t block_number)
{
	// complete indicator(bool) + file_identifier size(UINT) + block_size(UINT) + block_number(UINT) + 
	// file_identifier(string) +  + block_indicator(in bit format)
	size_t file_size = sizeof(bool) + 3 * sizeof(UINT) +
		identifier_size * sizeof(char) + (UINT)ceil(block_number / 8.0);
	return file_size;
}
static size_t compute_meta_file_size(string identifier, size_t block_number)
{
	return compute_meta_file_size(identifier.length() + 1, block_number);
}
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

static void write_file_identifier(char* buffer, string file_identifier) {
	get_meta_identifier_size(buffer) = (UINT)((file_identifier.length() + 1) * sizeof(char));
	memcpy_s(get_meta_identifier_ptr(buffer), get_meta_identifier_size(buffer),
		file_identifier.c_str(), get_meta_identifier_size(buffer));
}
/*
=====================================================================
					private nonstatic functions
=====================================================================
*/

void file_cache::initial_cache() {
	//If the cache path does not exist, create it
	if (!bf::is_directory(cache_path)) {
		bool result = bf::create_directories(cache_path);
		if (!result) {
			throw("Unable to create the cache path: %s", cache_path.c_str());
		}
	}
	string meta_path = get_meta_file_path();
	//If the meta file path is not a file, we will delete it
	if (bf::exists(meta_path) && !bf::is_regular_file(meta_path)) {
		try {
			bf::remove_all(meta_path);
		}
		catch (bf::filesystem_error e) {
			throw("Cannot delete meta file %s, message:%s", meta_path.c_str(), e.what());
		}
	}
	/*
		If meta file does not exist
		Create the file and open it
		If the meta file exists
		Open and validate if the file identifier matches
	*/
	if (!bf::exists(meta_path)) {
		create_meta_file();
		open_meta_file();
	}
	else {
		open_meta_file();
		if (!is_meta_file_valid(meta_path)) {
			close_meta_file();
			try {
				bf::remove(meta_path);
			}
			catch (bf::filesystem_error e) {
				throw("Cannot delete meta file %s, message:%s", meta_path.c_str(), e.what());
			}
			initial_cache();
			return;
		}
	}
	//Load the true block number,size,indicator from the meta file
	block_size = get_meta_block_size(meta_ptr);
	block_number = get_meta_block_number(meta_ptr);
	block_indicator = get_meta_block_indicator(meta_ptr);
	//Initial the vector which stores block handles
	initial_block_handles();
}



bool file_cache::is_meta_file_valid(string meta_path)
{
	//If the file identifier does not match
	//Delete the meta and create a new one
	size_t file_size = bf::file_size(meta_path);
	if (file_size < get_meta_minimum_size()) {
		return false;
	}
	//Compute the file size using the data in the meta file
	size_t expected_file_size = compute_meta_file_size(
		get_meta_identifier_size(meta_ptr),
		get_meta_block_number(meta_ptr)
	);
	if (file_size != expected_file_size) {
		return false;
	}

	string meta_file_identifier = get_meta_identifier_ptr(meta_ptr);
	if (meta_file_identifier != file_identifier) {
		return false;
	}


	UINT meta_block_size = get_meta_block_size(meta_ptr);
	UINT meta_block_number = get_meta_block_number(meta_ptr);
	if (meta_block_size == 0)
		return false;
	UINT expected_block_number = compute_block_number(this->file_size, meta_block_size);
	if (meta_block_number != expected_block_number)
		return false;

	return true;
}



/*
create meta file and write block size, number and file identifier to the file.
other values are set to 0
*/
void file_cache::create_meta_file()
{
	string meta_path = get_meta_file_path();
	std::ofstream meta_file(meta_path.c_str(), std::ios::out | std::ios::binary);
	if (!meta_file) {
		throw("Cannot open meta file %s", meta_path.c_str());
	}
	size_t file_size = compute_meta_file_size(file_identifier, block_number);
	char* data_buffer = DBG_NEW char[file_size];
	memset(data_buffer, 0, file_size);
	get_meta_block_size(data_buffer) = block_size;
	get_meta_block_number(data_buffer) = block_number;
	write_file_identifier(data_buffer, file_identifier);

	meta_file.write(data_buffer, file_size);
	delete[] data_buffer;
	meta_file.close();
	if (!meta_file.good()) {
		throw("Cannot write to meta file %s", meta_path.c_str());
	}
}


//Create the cache file
void file_cache::create_block_file_if_not(UINT block_index) {
	size_t cache_file_size = get_block_expected_size(file_size, block_size, block_index);
	string file_full_path = get_block_file_path(block_index);
	//If the file does not exist, initialize the file
	try {
		if (!bf::exists(file_full_path)) {
			std::ofstream output(file_full_path);
			output.close();
			if (!output.good()) {
				throw("Unable to create cache file: %s", file_full_path.c_str());
			}
		}
		if (bf::file_size(file_full_path) != cache_file_size) {
			bf::resize_file(file_full_path, cache_file_size);
		}
	}
	catch (bf::filesystem_error e) {
		throw("Cannot create block file %u, message:%s", block_index, e.what());
	}
}


void file_cache::initial_block_handles() {
	if (block_map_handles.size() != 0)
		throw("The block_map_handles has been initialized");
	block_map_handles.resize(block_number, nullptr);
	block_region_handles.resize(block_number, nullptr);
	block_ptrs.resize(block_number, nullptr);
}
void file_cache::release_block_handles() {
	for (UINT i = 0; i < block_number; i++) {
		close_block_file(i);
	}
}


void*& file_cache::get_block_map(unsigned int block_id)
{
	return block_map_handles[block_id];
}
void*& file_cache::get_block_region(unsigned int block_id)
{
	return block_region_handles[block_id];
}
char*& file_cache::get_block_ptr(unsigned int block_id)
{
	return block_ptrs[block_id];
}


void open_file(string path, void*& map_handle, void*& region_handle, char*& ptr) {
	if (map_handle != nullptr)
		throw("Oops, something is wrong");
	map_handle = DBG_NEW bi::file_mapping(path.c_str(), bi::read_write);
	region_handle = DBG_NEW bi::mapped_region(*(bi::file_mapping*)map_handle, bi::read_write);
	ptr = (char*)((bi::mapped_region*)region_handle)->get_address();
	if (ptr == nullptr) {
		throw("fail to map the meta data");
	}

}


string file_cache::get_meta_file_path() {
	string result = (cache_path / bf::path("meta_info")).string();
	return result;
}
string file_cache::get_block_file_path(unsigned int file_index) {
	bf::path cache_name = bf::path("temp_" + std::to_string(file_index));
	string result = (cache_path / cache_name).string();
	return result;
}
void file_cache::open_meta_file()
{
	if (meta_map_handle != nullptr)
		throw("Oops, something is wrong");
	string meta_path = get_meta_file_path();
	open_file(meta_path, meta_map_handle, meta_region_handle, meta_ptr);

}

void file_cache::open_block_file(unsigned int block_id)
{
	if (block_opened(block_id))
		return;
	string block_path = get_block_file_path(block_id);
	open_file(block_path, get_block_map(block_id),
		get_block_region(block_id),
		get_block_ptr(block_id));
}

void file_cache::close_meta_file() {
	close_file((bi::file_mapping*)meta_map_handle, (bi::mapped_region*) meta_region_handle);

	meta_map_handle = nullptr;
	meta_region_handle = nullptr;
	meta_ptr = nullptr;
	block_indicator = nullptr;
}
void file_cache::close_block_file(UINT block_id) {
	bi::mapped_region*& region_handle = (bi::mapped_region*&) get_block_region(block_id);
	bi::file_mapping*& map_handle = (bi::file_mapping*&) get_block_map(block_id);
	close_file(map_handle, region_handle);

	region_handle = nullptr;
	map_handle = nullptr;
	get_block_ptr(block_id) = nullptr;
}







bool file_cache::block_opened(unsigned int block_id)
{
	return get_block_map(block_id) != nullptr;
}

bool file_cache::block_exists(UINT block_id) {
	UINT byte_offset = block_id / 8;
	UINT within_byte_offset = block_id - byte_offset * 8;

	char* indicator = block_indicator;
	indicator = indicator + byte_offset;
	return ((*indicator) >> within_byte_offset) & 0x1;
}
void file_cache::set_block_bit(UINT block_id, bool bit_value) {
	UINT byte_offset = block_id / 8;
	UINT within_byte_offset = block_id - byte_offset * 8;

	char* indicator = block_indicator;
	indicator = indicator + byte_offset;
	char bit_mask = 1 << within_byte_offset;
	if (bit_value) {
		*indicator |= bit_mask;
	}
	else {
		*indicator &= ~bit_mask;
	}
}

unsigned int file_cache::get_block_size() {
	return block_size;
};
unsigned int file_cache::get_block_number() {
	return block_number;
}

void file_cache::create_mutex()
{
	mutex = std::vector<std::mutex>(block_number);
}

std::mutex& file_cache::get_block_mutex(unsigned int block_id)
{
	return mutex[block_id];
}

bool file_cache::is_random_read(size_t offset, size_t read_size) {
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
file_cache::file_cache(std::string cache_path, std::string file_identifier, size_t file_size,
	read_data_func data_func, void* file_info, unsigned int block_size)
{
	this->data_func = data_func;
	this->file_info = file_info;

	bf::path path(cache_path);
	this->cache_path = path.make_preferred().string();
	this->file_identifier = file_identifier + std::to_string(file_size);
	this->file_size = file_size;
	if (block_size != 0)
		this->block_size = block_size;
	else
		this->block_size = default_block_size;
	this->block_number = compute_block_number(file_size, this->block_size);
	try {
		initial_cache();
	}
	catch (bf::filesystem_error e) {
		throw("Cannot initial the cache file: %s", e.what());
	}
	create_mutex();
}

file_cache::~file_cache() {
	release_block_handles();
	close_meta_file();
}


//boost::mutex mutex;

void file_cache::read_data(char* buffer, size_t offset, size_t read_size) {
	if (offset + read_size > file_size) {
		throw("Read out-of-bound values");
	}

	bool random_read = is_random_read(offset, read_size);
	if (random_read) {
		try {
			(*data_func)(file_info, buffer, offset, read_size);
		}
		catch (std::exception e) {
			throw(e);
		}
	}

	//mutex.lock();
	while (read_size != 0) {
		block_read_info read_info = get_block_read_info(offset, read_size,block_size);
		UINT target_block = read_info.file_id;

		get_block_mutex(target_block).lock();
		//If the block has not been created on disk, we create and open it
		//Otherwise, we just open it if it haven't.
		if (!block_opened(target_block)) {
			create_block_file_if_not(target_block);
			open_block_file(target_block);
		}

		//If the data does not exist but the block has been opened
		//It means the block is a newly created one, we download its data from the cloud
		if (!block_exists(target_block)) {
			size_t cloud_file_offset = target_block * (size_t)block_size;
			size_t cloud_file_len = get_block_expected_size(file_size, block_size, target_block);
			try {
				(*data_func)(file_info, get_block_ptr(target_block), cloud_file_offset, cloud_file_len);
			}
			catch (std::exception e) {
				throw(e);
			}
			((bi::mapped_region*) get_block_region(target_block))->flush();
			set_block_bit(target_block, true);
		}
		get_block_mutex(target_block).unlock();

		// Copy the data from cache to the pointer
		char* target_cache_ptr = get_block_ptr(target_block) + read_info.file_byte_offset;
		size_t target_read_size = read_info.file_read_length;
		memcpy_s(buffer, target_read_size, target_cache_ptr, target_read_size);

		buffer = buffer + target_read_size;
		offset = offset + target_read_size;
		read_size = read_size - target_read_size;
	}
}

void* file_cache::get_file_info()
{
	return file_info;
}

void file_cache::set_file_info(void* file_info)
{
	this->file_info = file_info;
}

