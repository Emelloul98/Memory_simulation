The program is a simulation of processor approaches to memory (RAM), We use the mechanism "paging" that allow running a program when only a portion of it is in the memory.
The virtual memory is divided into pages ,this pages are brought to the main memory (RAM) by need.
In this program we use one proccess as the virtual memory, the simulation will be implemented by  two main approaches,load an address to the main memory and store address in the main memory by using the Hard disk.
Program DATABASE:
struct database that contain sub databases, the sub databases are:
1.page_table=array of structs, the page table serves as a table of contents ,we can get information about the RAM,address,and swap file.
2.swap_fd= file descriptor,hold the access to the swap file , the swap file simulate the Hard disk .
3.program_fd= file descriptor,hold the access to the executable file, this file simulate a process! 
4.main_memory=array of char size 200, this array simulate the RAM (random access memory).
functions:
two main functions:
1.load- this method receive an address and a Database , and ensure that the requested address will be in the main memory.
2.store- this method is very similar to "load" function,receive an address, Database and a value,
to store the address in the RAM the method need to allocate memory (same as "malloc"), and insert the value in the RAM.
==Program Files==
mem_sim.cpp- the file contain only functions
mem_sim.h- an header file ,contain structs,declerations of functions.
main.cpp- contain the main only.
Makefile-to compile the program.
==How to compile?==
compile: g++ main.cpp sim_mem.cpp -o main
run: ./main

==Input:==
A logical address to load and logical a logical address and a char to store.
==Output:==
The main memory (RAM)
The swap file
A char (that the load function returns)
The page_table


