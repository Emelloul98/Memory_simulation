#include "sim_mem.h"
#define EX_TABLE 4
sim_mem::sim_mem(char exe_file_name[],char swap_file_name[],int text_size,int data_size,int bss_size,int heap_stack_size,int page_size)
{
    swapfile_fd=open(swap_file_name,O_RDWR | O_CREAT | O_TRUNC, 0666);//opening a swap file.
    if(swapfile_fd==-1)
    {
        perror("ERR");
        exit(EXIT_FAILURE);
    }
    swap_size=bss_size+heap_stack_size+data_size;//the swap file size will be all the memory - text_size
    int num_of_zeros=swap_size;
    char zero_byte='0';
    //initialized the swap file to zeros in his size.
    for (int i = 0; i < num_of_zeros;i++)
    {
        ssize_t bytesWritten = write(swapfile_fd, &zero_byte, sizeof(char));
        if (bytesWritten ==-1)
        {
            perror("ERR");
            exit(EXIT_FAILURE);
        }
    }
    Time=0;//the os running time
    program_fd= open(exe_file_name,O_RDONLY);//the exe file for the logical addresses
    if(program_fd==-1)
    {
        perror("ERR");
        close(swapfile_fd);
        exit(EXIT_FAILURE);
    }
    //initialized the main memory(RAM) to zero
    for(int j=0;j<MEMORY_SIZE;j++)// Initialize the RAM to zero.
    {
        main_memory[j]='0';
    }
    main_memory[MEMORY_SIZE]='\0';
    isSwapOccupied=new bool[(swap_size/page_size)];//swap free pages
    isOccupied=new bool[(MEMORY_SIZE/page_size)];//memory free pages
    //initialized the class attributes:
    this->text_size=text_size;
    this->data_size=data_size;
    this->bss_size=bss_size;
    this->heap_stack_size=heap_stack_size;
    this->page_size=page_size;
    page_table=new page_descriptor*[EX_TABLE];
    //initialize the page table Matrix:
    for (int i=0; i <EX_TABLE;i++)
    {
        int temp_size=0;
        switch(i)
        {
            case 0:
            {
                temp_size=text_size/page_size;
                numOfPages[i]=temp_size;
                break;
            }
            case 1:
            {
                temp_size=data_size/page_size;
                numOfPages[i]=temp_size;
                break;
            }
            case 2:
            {
                temp_size=bss_size/page_size;
                numOfPages[i]=temp_size;
                break;
            }
            case 3:
                temp_size=heap_stack_size/page_size;
                numOfPages[i]=temp_size;
                break;
        }
        page_table[i]=new page_descriptor[temp_size];
        //initialized the struct attribute to his default values
        for(int j=0;j<temp_size;j++)
        {
            page_table[i][j].valid = false;
            page_table[i][j].frame = -1;
            page_table[i][j].dirty = false;
            page_table[i][j].swap_index = -1;
            page_table[i][j].Time_counter=-1;
        }
    }
}
void sim_mem::store(int address,char value)
{
    if(address<0 || address>4095) // more than 12 bits or a negative number.
    {
        printf("ERR\n");
        return;
    }
    Time++;//increase the os run time by one.
    int free_index;
    int mem_place,page_place,offset_place;
    MMU_calc(address,&mem_place,&page_place,&offset_place);// convert from logical to physical address.
    if(page_place>=numOfPages[mem_place])//if the logical address is incorrect
    {
        printf("ERR\n");
        return;
    }
    if(mem_place==0)//the page is a Text page.
    {
        printf("ERR\n");
        return;
    }
    if(page_table[mem_place][page_place].valid)//==True means if the page is in the RAM
    {
        int pageNum=page_table[mem_place][page_place].frame*page_size;
        main_memory[pageNum+offset_place]=value;
        page_table[mem_place][page_place].dirty=true;
        page_table[mem_place][page_place].Time_counter=Time;
    }
    else if(page_table[mem_place][page_place].dirty)//==True means that the page is in the disks swap file
    {
        free_index= from_file_to_ram(page_place,swapfile_fd,mem_place);
        main_memory[(free_index*page_size)+offset_place]=value;
        isSwapOccupied[page_table[mem_place][page_place].swap_index]=false;
        page_table[mem_place][page_place].swap_index=-1;
    }
    else if(mem_place!=1)//it is a malloc
    {
        free_index=empty_ram_place();
        for(int i=0;i<page_size;i++)
        {
            if(i!=offset_place)
            {
                main_memory[(free_index*page_size)+i]='0';
            }
            else main_memory[(free_index*page_size)+i]=value;
        }
        page_table[mem_place][page_place].valid=true;
        page_table[mem_place][page_place].frame=free_index;
        page_table[mem_place][page_place].dirty=true;
        isOccupied[free_index]=true;
        page_table[mem_place][page_place].Time_counter=Time;
    }
    else //data-from the exe file
    {
        free_index=from_file_to_ram(page_place,program_fd,mem_place);
        main_memory[(free_index*page_size)+offset_place]=value;
        page_table[mem_place][page_place].dirty=true;
    }
}
char sim_mem::load(int address)
{
    if(address<0 || address>4095)// more than 12 bits or a negative number.
    {
        printf("ERR\n");
        return '\0';
    }
    Time++;//increase the os run time by one.
    int free_index;
    int mem_place,page_place,offset_place;
    MMU_calc(address,&mem_place,&page_place,&offset_place);
    if(page_place>=numOfPages[mem_place])//if the logical address is incorrect
    {
        printf("ERR\n");
        return '\0';
    }
    if(page_table[mem_place][page_place].valid)//==True means if the page is in the RAM
    {
        int pageNum=page_table[mem_place][page_place].frame*page_size;
        page_table[mem_place][page_place].Time_counter=Time;
        return main_memory[pageNum+offset_place];
    }
    else if(mem_place==0)//the page is a Text page which means that the page is in the exe file.
    {
        free_index= from_file_to_ram(page_place,program_fd,mem_place);
        return main_memory[(free_index*page_size)+offset_place];
    }
    else if(page_table[mem_place][page_place].dirty)//==True means that the page is in the disks swap file
    {
        free_index= from_file_to_ram(page_place,swapfile_fd,mem_place);
        isSwapOccupied[page_table[mem_place][page_place].swap_index]=false;
        page_table[mem_place][page_place].swap_index=-1;
        return main_memory[(free_index*page_size)+offset_place];
    }
    else if(mem_place==2||mem_place==1) // bss or data that is not dirty.
    {
        free_index= from_file_to_ram(page_place,program_fd,mem_place);
        return main_memory[(free_index*page_size)+offset_place];
    }
    else//it is a malloc
    {
        printf("ERR\n");
        return '\0';
    }
}
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
void sim_mem::print_page_table() {
    int i;
    int num_of_txt_pages = text_size / page_size;
    int num_of_data_pages = data_size / page_size;
    int num_of_bss_pages = bss_size / page_size;
    int num_of_stack_heap_pages = heap_stack_size / page_size;
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_txt_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[0][i].valid,
               page_table[0][i].dirty,
               page_table[0][i].frame ,
               page_table[0][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_data_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[1][i].valid,
               page_table[1][i].dirty,
               page_table[1][i].frame ,
               page_table[1][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_bss_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[2][i].valid,
               page_table[2][i].dirty,
               page_table[2][i].frame ,
               page_table[2][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_stack_heap_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[3][i].valid,
               page_table[3][i].dirty,
               page_table[3][i].frame ,
               page_table[3][i].swap_index);
    }
}
void sim_mem::print_swap() {
    char* str = (char*)malloc(this->page_size *sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}
// a destructor for sim_mem class:
sim_mem::~sim_mem()
{
    for (int i = 0; i <EX_TABLE;i++)
    {
        delete[] page_table[i];
    }
    delete[] page_table;
    delete[] isOccupied;
    delete[] isSwapOccupied;
    close(program_fd);
    close(swapfile_fd);
}
void sim_mem::MMU_calc(int address,int* mem_place,int* page_place,int* offset_place) const
{
    size_t endPtr;
    int offset_size=(int)log2(page_size);
    // Convert the number to binary and pad with zeros to make it 12 bits long
    string binary = bitset<12>(address).to_string();
    // Separate the address:
    string first = binary.substr(0, 2);//the first two number
    string second = binary.substr(2, 10-offset_size);//the middle numbers
    string third = binary.substr(12-offset_size);//from this number to the end.
    // conversion back to int:
    *mem_place=(int)stol(first,&endPtr,2);
    *page_place=(int)stol(second,&endPtr,2);
    *offset_place=(int)stol(third,&endPtr,2);

}
int sim_mem::from_file_to_ram(int page_place,int file_fd,int mem_place) {
    int res, real_index;
    char onePage[page_size];
    int file_place = 0;
    if (mem_place == 1)
    {
        file_place += (text_size);
    }
    else if(mem_place == 2)
    {
        file_place+=(text_size+data_size);
    }
    if (file_fd == program_fd) {
        lseek(file_fd, (file_place + (page_place * page_size)), SEEK_SET);//skips to the page place in the file
    }
    else//swap file
    {
        lseek(file_fd, page_table[mem_place][page_place].swap_index * page_size, SEEK_SET);//jumps to the page
    }
    ssize_t bytesRead = read(file_fd, onePage, page_size);//reads one page
    if (bytesRead < 0) {
        perror("ERR");
        exit(EXIT_FAILURE);
    }
    if(file_fd==swapfile_fd)//cleans the swap
    {
        char zeros[page_size];
        char zero='0';
        memset(zeros, zero, page_size);
        lseek(file_fd, page_table[mem_place][page_place].swap_index * page_size, SEEK_SET);
        //resets the cursor.
        ssize_t bytesWrite= write(file_fd,zeros,page_size);
        if (bytesWrite < 0)
        {
            perror("ERR");
            exit(EXIT_FAILURE);
        }
    }
        res=empty_ram_place();//finding an empty place in the main memory
        real_index = res * page_size;//the index in the main memory array
        for (int i = 0; i < page_size; i++) {
            main_memory[real_index + i] = onePage[i];//cooping the page to the RAM
        }
        // Updating the page table:
        page_table[mem_place][page_place].valid = true;
        page_table[mem_place][page_place].frame = res;
        isOccupied[res] = true;
        page_table[mem_place][page_place].Time_counter=Time;
        return res;
    }

    int sim_mem::empty_ram_place() {
        for (int i = 0; i < MEMORY_SIZE / page_size; i++) {
            if (!isOccupied[i]) {
                return i;
            }
        }
        // if the RAM is full
        return clean_page();// I need to make this function to clean the RAM

    }
    // A function that finds an empty swap place
    int sim_mem::empty_swap_place() {
        int swapSize = (bss_size + heap_stack_size + data_size) / page_size;
        for (int i = 0; i < swapSize; i++) {
            if (!isSwapOccupied[i]) {
                return i;
            }
        }
        return -1;//not possible because the swap size is equals to all the data that can go to the swap area.
    }
    // A function that clears a page from memory in LRU method
    int sim_mem::clean_page() {
        int out_table_num, page_num, sum = numeric_limits<int>::max();
        // The loop finds the Ram page that is the Last Recently Used
        for (int i = 0; i < EX_TABLE; i++) {
            for (int j = 0; j < numOfPages[i]; j++) {
                if(page_table[i][j].valid) //search only in the RAM pages.
                {
                    int temp = page_table[i][j].Time_counter;
                    if (temp < sum)
                    {
                        sum = temp;
                        page_num = j;
                        out_table_num = i;
                    }
                }
            }
        }
        int cleanInd = page_table[out_table_num][page_num].frame;
        //if the page is dirty,saving him in the swap,else delete(hi is already in the exe_file..)
        if (page_table[out_table_num][page_num].dirty)
        {
            int swap_index = empty_swap_place();
            if(swap_index==-1)
            {
                printf("ERR\n");
                exit(EXIT_FAILURE);
            }
            lseek(swapfile_fd, swap_index * page_size, SEEK_SET);//jumps to the empty swap page
            //write the page to the swap
            ssize_t bytesWrite=write(swapfile_fd,&main_memory[cleanInd * page_size], page_size);
            if (bytesWrite < 0)
            {
                perror("ERR");
                exit(EXIT_FAILURE);
            }
            //Updating the page table:
            page_table[out_table_num][page_num].swap_index = swap_index;
            isSwapOccupied[swap_index] = true;
        }
        page_table[out_table_num][page_num].valid = false;
        page_table[out_table_num][page_num].Time_counter=-1;
        page_table[out_table_num][page_num].frame=-1;
        isOccupied[cleanInd] = false;
        return cleanInd;
    }



