#pragma once
#include <string>


typedef void (*read_data_func)(void* file_info, void* buffer, size_t offset, size_t length);



class file_cache{
private:
    struct block_read_info {
        unsigned int file_id;
        unsigned int file_byte_offset;
        unsigned int file_read_length;
    };

    //function which is used to read data
    void* file_info;
    read_data_func data_func;
    //file info
    std::string cache_path;
    std::string file_identifier;
    size_t file_size;

    //meta info
    void* meta_file_handle = NULL;
    void* meta_map_handle = NULL;
    unsigned int block_size = 0;
    unsigned int block_number = 0;
    char* meta_ptr = NULL;
    char* block_indicator = NULL;

    //current cached block
    unsigned int file_id = 0;
    void* cache_file_handle = NULL;
    void* cache_map_handle = NULL;
    char* cached_ptr = NULL;
public:
    file_cache(std::string cache_path,
        std::string file_identifier,
        size_t file_size,
        read_data_func,
        void* file_info,
        unsigned int block_size = 256*1024
    );
    ~file_cache();
    bool read_data(char* buffer, size_t offset, size_t read_size);
private:
    //Some utilities
    void close_meta_file();
    void close_cache_file();
    void initial_cache();

    std::string get_meta_path();
    std::string get_cache_path(unsigned int file_index);
    block_read_info get_block_read_info(size_t offset, size_t read_size);
    bool block_exists(unsigned int block_id);
    void set_block_bit(unsigned int block_id, bool bit_value);
    void open_cache_file(unsigned int block_id);
    unsigned int get_block_size();
    unsigned int get_block_number();

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

    static void write_file_identifier(char* buffer, std::string file_identifier);
};




