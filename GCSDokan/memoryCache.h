#pragma once
#include <string>
#include <mutex>

class memory_cache {
	typedef void (*read_data_func)(void* file_info, void* buffer, size_t offset, size_t length);

public:
	//Default values
	static unsigned int random_access_range_cutoff;
	static unsigned int random_access_tolerance_time;
	static unsigned int default_block_size;
	static size_t memory_cache_block_number;
private:
	//function which is used to read data
	void* file_info;
	read_data_func data_func;
	//file info
	std::string file_identifier;
	size_t file_size;
	unsigned int block_size = 0;
	unsigned int block_number = 0;

	//Some stat
	size_t last_read_offset = 0;
	size_t last_read_size = 0;
	size_t random_read_time;

	//utilities
	/*
	The global mutex, used for inserting object to 
	the cache
	*/
	static std::mutex static_mutex;
	// Object mutex, used for checking random access
	std::mutex file_mutex;
public:
	memory_cache(
		std::string file_identifier,
		size_t file_size,
		read_data_func,
		void* file_info,
		unsigned int block_size = 0
	);
	~memory_cache();
	bool read_data(char* buffer, size_t offset, size_t read_size);
	//accessor
	void* get_file_info();
	void set_file_info(void* file_info);

private:
	bool is_random_read(size_t offset, size_t read_size);
};