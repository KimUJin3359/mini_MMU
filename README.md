# mini_MMU

### Requirements
#### Basic Info
- 3 level page table
  - `PD` -> `PMD` -> `PT`
- 8 bits address
  - 2 bits for PDE(Page Directory Entry)
  - 2 bits for PMDE(Page Middle Directory Entry)
  - 2 bits for PTE(Page Table Entry)
  - 2 bits for offset
- 4 bytes page frame
  - 4 PDEs make up PD
  - 4 PMDEs make up PMD
  - 4 PTEs make up PT
- 256 bytes address-space

- format
  - PDE/PMDE/PTE
    - 6 bits(PFN) + 2 bits(Offset, Present)
    - originally, 1bit to present bit but it restricts only 8 bits address so offset bits uses like a present bit 
    - unmapped PTE is filled with zeros
  - Swap  
    - 7 bits(Swap Space offset) + 1 bit(Present bit)

- PDBR == CR3
  - starts at low address

#### Function Info
- int page_fault(char pid, char va)
  - Page Fault Handler
  - Handling a page fault caused by on-demand paging or swapping
    - Page replacement policy : FIFO
  - pid : process id
  - va : virtual address
  - return : 0 for success, -1 for fail
- int mmu_init(unsigned_int mem_size, unsigned int swap_size)
  - resource initialization function
  - mem_size
    - physical memory size in bytes
    - ignore the memory space consumed by page directions and tables
    - not real memory space, simulate its behavior
  - swap_size
    - swap disk size in bytes
    - not real disk space, simulate its behavior
  - return : 0 for success, -1 for fail
- int run_proc(char pid, struct pte \*cr3)
  - create a process or perform a context switch
  - pid : pid of the next process
  - cr3 : base address of the page directory for the next process 
  - return value : 0 for success, -1 for fail

#### Development Info
- Free list : having a info about physical memory\'s data
  - page frame info, pid value, order of input to physic
- when pagefault occurs
  - writes a PFN(Page Frame Number), PID, order num to free list
- Action
  - In virtual address space, find PUD, PMDE, PTE
