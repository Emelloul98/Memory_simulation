#ifndef EX4_SIM_MEM_H
#define EX4_SIM_MEM_H
#define MEMORY_SIZE 200
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <bitset>
#include <cmath>
using namespace std;
extern char main_memory[MEMORY_SIZE+1];// The RAM
typedef struct page_descriptor
{
    bool valid;
    int frame;
    bool dirty;
    int swap_index;
    int reference_counter;
}page_descriptor;
class sim_mem {
    int swapfile_fd;
    int program_fd;
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;
    int num_of_proc;
    int swap_size;
    page_descriptor ** page_table;
    bool* isOccupied;
    bool* isSwapOccupied;
    int numOfPages[4];
public:
    sim_mem(char exe_dile_name[],char swap_file_name[],int text_size,int data_size,int bss_size,int heap_stack_size,int page_size);
    void store(int address,char value);
    char load(int address);
    void print_memory();
    void print_page_table();
    void print_swap();
    ~sim_mem();
    void MMU_calc(int,int*,int*,int*) const;
    int empty_ram_place();
    int empty_swap_place();
    int clean_page();
    int from_file_to_ram(int,int,int);
};


#endif //EX4_SIM_MEM_H

