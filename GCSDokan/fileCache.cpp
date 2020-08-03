#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "fileCache.h"

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

//The size of each block in byte
static UINT block_size = 256 * 1024;

/*
=====================================================================
					private nonstatic functions
=====================================================================
*/

void file_cache::initial_cache() {
	//If cache_info has not been initialized
	if (meta_file_handle == NULL) {
		if (!bf::is_directory(cache_path)) {
			bool result = bf::create_directories(cache_path);
			if (!result) {
				throw("Unable to create the cache path: %s", cache_path.c_str());
			}
		}
		string meta_path = get_meta_path();
		//Create the cache path if it does not exist
		//Create the meta file if it does not exist
		if (bf::exists(meta_path) && !bf::is_regular_file(meta_path)) {
			bf::remove_all(meta_path);
		}
		bool is_new_meta = !bf::exists(meta_path);
		if (is_new_meta) {
			std::ofstream meta_file(meta_path.c_str(), std::ios::out | std::ios::binary);
			if (!meta_file) {
				throw("Cannot open meta file %s", meta_path.c_str());
			}
			// complete indicator(bool) + file_identifier size(UINT) + block_size(UINT) + block_number(UINT) + 
			// file_identifier(string) +  + block_indicator(in bit format)
			size_t file_size = sizeof(bool) + 3 * sizeof(UINT) +
				(file_identifier.length() + 1) * sizeof(char) + block_number;
			char* data_buffer = new char[file_size];
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

		meta_file_handle = new bi::file_mapping(meta_path.c_str(), bi::read_write);
		meta_map_handle = new bi::mapped_region(*(bi::file_mapping*)meta_file_handle, bi::read_write);
		meta_ptr = (char*)((bi::mapped_region*)meta_map_handle)->get_address();
		if (meta_ptr == NULL) {
			throw("fail to map the meta data");
		}

		
		if (!is_new_meta) {
			//If the file identifier does not match
			//Delete the meta and create a new one
			size_t file_size = bf::file_size(meta_path);
			size_t desired_file_size = get_meta_total_offset(meta_ptr);
			string desired_file_identifier = get_meta_identifier_ptr(meta_ptr);
			if (file_size < get_meta_minimum_size() ||
				desired_file_size != file_size ||
				desired_file_identifier != file_identifier) {
				close_meta_file();
				try {
					bool success = bf::remove(meta_path);
				}
				catch (bf::filesystem_error e) {
					throw("Cannot delete meta file %s", meta_path.c_str());
				}
				initial_cache();
			}
		}
		else {
			block_number = get_meta_block_number(meta_ptr);
			block_size = get_meta_block_size(meta_ptr);
			block_indicator = get_meta_block_indicator(meta_ptr);
		}
	}
}

void file_cache::close_meta_file() {
	if (meta_map_handle != NULL) {
		((bi::mapped_region*) meta_map_handle)->flush();
		delete meta_map_handle;
	}
	if (meta_file_handle != NULL) {
		//auto handle = (bi::file_mapping*) meta_file_handle;
		//handle->remove(handle->get_name());
		delete meta_file_handle;
	}

	meta_file_handle = NULL;
	meta_map_handle = NULL;
	meta_ptr = NULL;
	block_indicator = NULL;
}
void file_cache::close_cache_file() {
	if (cache_map_handle != NULL) {
		((bi::mapped_region*) cache_map_handle)->flush();
		delete cache_map_handle;
	}
	if (cache_file_handle != NULL) {
		//auto handle = (bi::file_mapping*) cache_file_handle;
		//handle->remove(handle->get_name());
		delete cache_file_handle;
	}

	cache_file_handle = NULL;
	cache_map_handle = NULL;
	cached_ptr = NULL;
}

string file_cache::get_meta_path() {
	string result = (cache_path / bf::path("meta_info")).string();
	return result;
}
string file_cache::get_cache_path(unsigned int file_index) {
	bf::path cache_name = bf::path("temp_" + std::to_string(file_index));
	string result = (cache_path / cache_name).string();
	return result;
}




file_cache::block_read_info file_cache::get_block_read_info(
	size_t offset,
	size_t read_size) {
	UINT file_id = (UINT)(offset / block_size);
	UINT file_byte_offset = (UINT)(offset - file_id * (size_t)block_size);
	UINT read_length = (UINT)min((size_t)block_size - file_byte_offset, read_size);

	block_read_info read_info;
	read_info.file_id = file_id;
	read_info.file_byte_offset = file_byte_offset;
	read_info.file_read_length = (UINT)read_size;
	return read_info;
}

bool file_cache::block_exists(UINT block_id) {
	UINT byte_offset = block_id / 8;
	UINT within_byte_offset = block_id - byte_offset * 8;

	char* indicator = block_indicator;
	indicator = indicator + byte_offset;
	return ((*block_indicator) >> within_byte_offset) & 0x1;
}
void file_cache::set_block_bit(UINT block_id, bool bit_value) {
	UINT byte_offset = block_id / 8;
	UINT within_byte_offset = block_id - byte_offset * 8;

	char* indicator = block_indicator;
	indicator = indicator + byte_offset;
	char bit_mask = 1 << within_byte_offset;
	if (bit_value) {
		*block_indicator |= bit_mask;
	}
	else {
		*block_indicator &= ~bit_mask;
	}
}

unsigned int file_cache::get_block_size() {
	return block_size;
};
unsigned int file_cache::get_block_number() {
	return block_number;
}

/*
=====================================================================
					private static functions
=====================================================================
*/

unsigned int file_cache::get_meta_minimum_size() {
	return get_meta_identifier_offset(NULL);
}
UINT file_cache::get_meta_complete_indicator_offset(char* buffer) {
	return 0;
}
UINT file_cache::get_meta_identifier_size_offset(char* buffer) {
	return get_meta_complete_indicator_offset(buffer) + sizeof(bool);
}
UINT file_cache::get_meta_block_size_offset(char* buffer) {
	return get_meta_identifier_size_offset(buffer) + +sizeof(UINT);
}
UINT file_cache::get_meta_block_number_offset(char* buffer) {
	return get_meta_block_size_offset(buffer) + sizeof(UINT);
}
UINT file_cache::get_meta_identifier_offset(char* buffer) {
	return  get_meta_block_number_offset(buffer) + sizeof(UINT);
}
UINT file_cache::get_meta_block_indicator_offset(char* buffer) {
	return get_meta_identifier_offset(buffer) + get_meta_identifier_size(buffer);
}
unsigned int file_cache::get_meta_total_offset(char* buffer) {
	return get_meta_identifier_offset(buffer) + get_meta_identifier_size(buffer)
		+ get_meta_block_size(buffer);
}


bool& file_cache::get_meta_complete_indicator(char* buffer) {
	return *((bool*)(buffer + get_meta_complete_indicator_offset(buffer)));
}
UINT& file_cache::get_meta_identifier_size(char* buffer) {
	return  *((UINT*)(buffer + get_meta_identifier_size_offset(buffer)));
}
UINT& file_cache::get_meta_block_size(char* buffer) {
	return  *((UINT*)(buffer + get_meta_block_size_offset(buffer)));
}
UINT& file_cache::get_meta_block_number(char* buffer) {
	return  *((UINT*)(buffer + get_meta_block_number_offset(buffer)));
}
char* file_cache::get_meta_identifier_ptr(char* buffer) {
	return  buffer + get_meta_identifier_offset(buffer);
}
char* file_cache::get_meta_block_indicator(char* buffer) {
	return  buffer + get_meta_block_indicator_offset(buffer);
}


void file_cache::write_file_identifier(char* buffer, string file_identifier) {
	get_meta_identifier_size(buffer) = (UINT)((file_identifier.length() + 1) * sizeof(char));
	memcpy_s(get_meta_identifier_ptr(buffer), get_meta_identifier_size(buffer),
		file_identifier.c_str(), get_meta_identifier_size(buffer));
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
	this->block_size = block_size;
	this->block_number = (UINT)(std::ceil(file_size / (double)block_size));
	initial_cache();
}

file_cache::~file_cache() {
	close_meta_file();
	close_cache_file();
}

//Open or create the cache file
void file_cache::open_cache_file(UINT block_id) {
	UINT file_index = block_id / 8;
	//Check if the required cache file has been opened
	//If not, close the current cache file and open the requried one
	if (cache_file_handle != NULL) {
		if (file_id == file_index) {
			return;
		}
		else {
			close_cache_file();
		}
	}

	size_t cache_file_size = min(block_size, file_size - (size_t)block_id * block_size);
	string file_full_path = get_cache_path(file_index);
	//If the file does not exist, initialize the file
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
	cache_file_handle = new bi::file_mapping(file_full_path.c_str(), bi::read_write);
	cache_map_handle = new bi::mapped_region(*(bi::file_mapping*)cache_file_handle, bi::read_write);
	cached_ptr = (char*)((bi::mapped_region*)cache_map_handle)->get_address();
	if (cached_ptr == NULL) {
		throw("fail to map the meta data");
	}
	file_id = file_index;
}


bool file_cache::read_data(char* buffer, size_t offset, size_t read_size) {
	void* cloud_buffer = NULL;
	while (read_size != 0) {
		block_read_info read_info = get_block_read_info(offset, read_size);
		open_cache_file(read_info.file_id);
		char* target_cache_ptr = cached_ptr + read_info.file_byte_offset;
		size_t target_read_size = read_info.file_read_length;

		if (!block_exists(read_info.file_id)) {
			//If the data does not exist, we download it from the cloud
			size_t cloud_file_offset = read_info.file_id * block_size;
			size_t cloud_file_len = min(block_size, file_size - read_info.file_id * block_size);
			try {
				(*data_func)(file_info, cached_ptr, cloud_file_offset, cloud_file_len);
			}
			catch (std::exception e) {
				return false;
			}
			((bi::mapped_region*) cache_map_handle)->flush();
			set_block_bit(read_info.file_id, true);
		}
		memcpy_s(buffer, target_read_size, target_cache_ptr, target_read_size);

		buffer = buffer + target_read_size;
		offset = offset + target_read_size;
		read_size = read_size - target_read_size;
	}
	return true;
}

