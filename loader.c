#define _FILE_OFFSET_BITS 64
#include <elf.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *,int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start; // elf header
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff); // program header
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], arg);
    }
    return 0;
}
char* get_type(uint32_t type) {
    switch (type) {
        case PT_LOAD:    return "LOAD";
        case PT_DYNAMIC: return "DYNAMIC";
        case PT_INTERP:  return "INTERP";
        case PT_NOTE:    return "NOTE";
        case PT_PHDR:    return "PHDR";
        case PT_TLS:     return "TLS";
        case 0x6474e551: return "GNU_STACK";
        default:         return "UNKNOWN";
    }
}

void print_phdr(Elf32_Phdr *phdr, int arg) {
    // decode flags using bitwise AND operation
    char flags[4] = "   ";
    if (phdr->p_flags & PF_R) flags[0] = 'R';
    if (phdr->p_flags & PF_W) flags[1] = 'W';
    if (phdr->p_flags & PF_X) flags[2] = 'E';
    printf("%-15s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %s 0x%x\n", // 6,8,5 are the width of the output in hex
        get_type(phdr->p_type),phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
        phdr->p_filesz, phdr->p_memsz, flags, phdr->p_align);

    // print mapping information if the segment is loadable
    if (phdr->p_type == PT_LOAD) {   
        printf("  mmap prot flags: ");
        if (phdr->p_flags & PF_R) printf("PROT_READ ");
        if (phdr->p_flags & PF_W) printf("PROT_WRITE ");
        if (phdr->p_flags & PF_X) printf("PROT_EXEC ");
        printf("\n  mmap map flags: MAP_PRIVATE | MAP_FIXED\n");
    }
}

/** 
    get a file and an offset, 
    write into memory all parts which are type pt_load
    print the ELF file using the print_phdr function
*/
void load_phdr(Elf32_Phdr *phdr, int fd) {
    
    if (phdr->p_type == PT_LOAD) {   
        // assign prot values based on elf stats
        int prot = 0;
        if (phdr->p_flags & PF_R) prot |= PROT_READ;
        if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
        if (phdr->p_flags & PF_X) prot |= PROT_EXEC;
        
        // align address to start of page
        void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
        // align offset to start of page 
        int offset = phdr->p_offset & 0xfffff000;
        // padding is the additional space added after the alignment 
        int padding = phdr->p_vaddr & 0xfff;

        // assign pt load section to memory
        if(mmap(vaddr, phdr->p_memsz+padding, prot, MAP_PRIVATE | MAP_FIXED, fd, offset) // map file to memory
            == MAP_FAILED) { perror("mmap failed"); return; } // in case of mmap fail print it
    }

    // print ELF file stats
    print_phdr(phdr,fd);
}

int startup(int argc, char **argv, void (*start)());

int main(int argc, char **argv) {
    int fd = open(argv[1], O_RDONLY);
    
    struct stat st; // struct to hold file information
    fstat(fd, &st);

    printf("Type            Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0); // map file to memory

    foreach_phdr(map_start, load_phdr, fd);

    // after all segments are mapped into mem, we find e_entry and run the program from it with the args given
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    // send argc-1 args to program, the args are argv/argv[1] 
    startup(argc-1,argv+1, (void *)ehdr->e_entry);

    return 0;
}

