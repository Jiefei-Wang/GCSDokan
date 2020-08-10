#pragma once
#include <string>
#include <mutex>
#include <vector>




class file_cache{
    typedef void (*read_data_func)(void* file_info, void* buffer, size_t offset, size_t length);
private:
    struct block_read_info {
        unsigned int file_id;
        unsigned int file_byte_offset;
        unsigned int file_read_length;
    };
    //Default values
    static unsigned int random_access_range_curoff;
    static unsigned int random_access_tolerance_time;
    static unsigned int default_block_size;


    //function which is used to read data
    void* file_info;
    read_data_func data_func;
    //file info
    std::string cache_path;
    std::string file_identifier;
    size_t file_size;
    unsigned int block_size = 0;
    unsigned int block_number = 0;

    //meta info
    void* meta_map_handle = nullptr;
    void* meta_region_handle = nullptr;
    char* meta_ptr = nullptr;
    char* block_indicator = nullptr;

    //current cached block
    unsigned int file_id = 0;
    std::vector<void*> block_map_handles;
    std::vector<void*> block_region_handles;
    std::vector<char*> block_ptrs;

    //Some stat
    size_t last_read_offset = 0;
    size_t last_read_size = 0;
    size_t random_read_time;

    //utilities
    std::vector<std::mutex> mutex;
    std::mutex stat_mutex;
public:
    file_cache(std::string cache_path,
        std::string file_identifier,
        size_t file_size,
        read_data_func,
        void* file_info,
        unsigned int block_size = 0
    );
    ~file_cache();
    bool read_data(char* buffer, size_t offset, size_t read_size);
    //accessor
    void* get_file_info();
    void set_file_info(void* file_info);
private:
    // create or read cache meta files
    // Prerequest: block_number, block_size, file_size
    void initial_cache();
    bool is_meta_file_valid(std::string meta_path);

    // Functions to create meta and block file
    void create_meta_file();
    void create_block_file(unsigned int block_id);

    // Functions to open or close files
    std::string get_meta_file_path();
    std::string get_block_file_path(unsigned int file_index);
    void open_meta_file();
    void open_block_file(unsigned int block_id);
    void close_meta_file();
    void close_block_file(unsigned int block_id);

    //Functions to get and release handles
    void initial_block_handles();
    void release_block_handles();

    void*& get_block_map(unsigned int block_id);
    void*& get_block_region(unsigned int block_id);
    char*& get_block_ptr(unsigned int block_id);

    block_read_info get_block_read_info(size_t offset, size_t read_size);
    //Whether the block has been cached, the result is not relavent to 
    //the status of the physical existance of the block file
    bool block_exists(unsigned int block_id);
    bool block_opened(unsigned int block_id);
    void set_block_bit(unsigned int block_id, bool bit_value);
    unsigned int get_block_size();
    unsigned int get_block_number();


    //Mutex functions
    void create_mutex();
    std::mutex& get_block_mutex(unsigned int block_id);
};




