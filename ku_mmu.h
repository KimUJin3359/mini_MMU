#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PCB_SIZE 128
// 2^13 + 1
#define MAX_PRIOR 8193
#define PD_MASK = (char)0xC0;
#define PMD_MASK = (char)0x30;
#define PT_MASK = (char)0xC;
#define PR_BIT = (char)0x1;

// address size : 1 byte 
typedef struct	s_pte
{
	char	pte;
}				t_pte;

// page frame size : 4 bytes 
typedef struct	s_page 
{
	char	pte[4];
}				t_page;

// manage for list about physiccal, swap memory
typedef struct	t_list 
{
	char	pid;			//if pid == -1 -> not allocated, pid !=0 -> allocated
	short	priority;		//for FIFO(physical memory), no use for SWAPMEMORY
}				s_list;

typedef struct	t_pcb 
{
	char pid;
	char cr3;
}				s_pcb;

typedef struct	t_memory
{
	int phys_size;
	int swap_size;
	int pcb_size = 0;
	int phys_index;
	int swap_index;
	int priority = 0;

	struct page *phys_mem;
	struct list *phys_list;
	struct list *swap_list;
	struct pcb *pcb;
}				s_memory;

// memory init
// allocate and initialize a physical memory
void* mmu_init(unsigned int mem_size, unsigned int swap_size);
// occur a page fault(when using page is missing)
// 1. memory not full -> allocate
// 2. memory is full -> swap out -> allocate
int page_fault(char pid, char va);
// runnig a process
// 1. pid in pcb -> return cr3
// 2. pid not in pcb -> page fault
int run_proc(char pid, struct pte **cr3);

//init phys_memory, swap_memory, phys_memory list, swap_memory list, and pcb
void* mmu_init(s_memory mem, unsigned int mem_size, unsigned int swap_size) {
	int init = 0;

	mem.phys_index = mem_size / 4;
	mem.swap_index = swap_size / 4;
	mem.phys_size = mem_size;
	mem.swap_size = swap_size;
	//allocate phys memory, swap memory
	mem.phys_mem = malloc(sizeof(s_page) * phys_index);
	//allocate free list (phys memory, swap memory)
	mem.phys_list = malloc(sizeof(s_list) * phys_index);
	mem.swap_list = malloc(sizeof(s_list) * swap_index);
	//allocate pcb
	mem.pcb = malloc(sizeof(s_pcb) * PCB_SIZE);

	//initialize
	for (int i = 0; i < mem.phys_index; i++) 
	{
		memcpy(&(mem.phys_mem[i]), &init, sizeof(struct page));
		mem.phys_list[i].pid = -1;
		mem.phys_list[i].priority = MAX_PRIOR;	// for maximum
	}
	for (int i = 0; i < mem.swap_index; i++) 
	{
		mem.swap_list[i].pid = -1;
		mem.swap_list[i].priority = 0;	// in swap, priority is not used
	}
	for (int i = 0; i < PCB_SIZE; i++) 
	{
		mem.pcb[i].pid = -1;
		mem.pcb[i].cr3 = -1;
	}

	return (mem.phys_mem);
}

int find_pid_in_phys(s_memory mem, char pid)
{
	for (int i = 0; i < mem.phys_size; i++)
	{
		if (mem.phys_list[i].pid == pid)
			return (i);
	}
	return (-1);
}

// run process
int run_proc(s_memory mem, char pid, s_pte **cr3) {;
	int find = 0;
	int init = 0;

	// find pid
	// if pid exist -> return it's cr3
	// else -> allocate it
	find = find_pid_in_phys(mem, pid);
	// pid exist
	if (find != -1) {
		// cr3 is base address of physical memory
		*(cr3) = &(mem.phys_mem[mem.pcb[find].cr3].pte[0]);
		return (0);
	}

	// pid not exist
	// find empty page table entry
	find = find_pid_in_phys(mem, -1);
	//if find ==1, exist free page for new PD
	if (find != -1) 
	{
		mem.pcb[mem.pcb_size].pid = pid;
		mem.pcb[mem.pcb_size].cr3 = find;
		mem.pcb_size++;
		// cr3 is base address of physical memory
		*(cr3) = &(mem.phys_mem[find].pte[0]);
		//not initialize priority because PD page is not swap out
		//list is set but not allocated, PMD so can't allocate in the real memory
		mem.phys_list[find].pid = pid;
		//not initialized priority because PD page is not swap out
		//list is set but not allocated PMD so can't allocate in the real memory
		return (0);
	}
	// not exist free page so swap out
	else 
	{
		int prior = 0;
		int p_i, s_i;	// phyiscal memory idx, swap memory idx
		int pcb_i;

		find = 0;
		// first, check (can swap out page frame is exist)
		while (!find) 
		{
			for (p_i = 0; p_i < mem.phys_index; p_i++) 
			{
				// i is swap out page index
				if (mem.phys_list[p_i].priority == prior) 
				{
					find = 1;
					break;	
				}
			}
			prior++;
			// not exist page to swapout
			if (prior == MAX_PRIOR) 
				return (-1);
		}
		find = 0;
		// second, check (can swap out, if swap page is full or not full)
		for (s_i = 0; s_i < mem.swap_index; s_i++) 
		{
			if (mem.swap_list[s_i].pid == -1) 
			{
				find = 1;
				break;
			}
		}
		// third, exist and can swap out -> swap out(change PTE to swap index+0, and allocate PD)
		if (find) {
			char cr3;
			char pd, pmd, pt;

			// exist swap memory to swap out
			// cr3 of swap out page frame
			for (pcb_i = 0; pcb_i < mem.pcb_size; pcb_i++) 
			{
				if (mem.pcb[pcb_i].pid == phys_list[p_i].pid) 
				{
					cr3 = mem.pcb[pcb_i].cr3;
					break;
				}
			}
			//look cr3 -> find all PD, PMD, PT->PTE
			find = 0;
			for (int i = 0; i < 4; i++) 
			{
				if (mem.phys_mem[cr3].pte[i] != 0) 
				{
					pd = mem.phys_mem[cr3].pte[i];
					pd >>= 2;
					for (int j = 0; j < 4; j++) 
					{
						if (mem.phys_mem[pd].pte[j] != 0) 
						{
							pmd = mem.phys_mem[pd].pte[j];
							pmd >>= 2;
							for (int k = 0; k < 4; k++) 
							{
								pt = mem.phys_mem[pmd].pte[k];
								pt >>= 2;
								if (pt == p_i) {
									char pte;

									pte = s_i;
									// set present bit
									pte++;
									pte <<= 1;
									mem.phys_mem[pt].pte[k] = pte;
									find = 1;
									break;
								}
							}
							if (find)
								break;
						}
					}
					if (find)
						break;
				}
			}
			mem.phys_list[p_i].pid = pid;
			mem.phys_list[p_i].priority = MAX_PRIOR;
			mem.pcb[mem.pcb_size].pid = pid;
			mem.pcb[mem.pcb_size].cr3 = p_i;
			mem.pcb_size++;
			*(cr3) = &(mem.phys_mem[p_i].pte[0]);
			mem.swap_list[s_i].pid = mem.pcb[pcb_i].pid;
		}
		else 
			//not exist swap memory to swap out
			return (-1);
	}
	return (-1);
}

int find_cr3(s_memory mem, char pid)
{
	// find cr3
	for (int i = 0; i < mem.pcb_size; i++)
	{
		if (mem.pcb[i].pid == pid)
			return (mem.pcb[i].cr3);
	}
	// not in virtual memory space
	return (-1);
}

// 210426
int page_fault(s_memory mem, char pid, char va) {
	char cr3 = -1;
	// INDEX
	char pde_index;
	char pmde_index;
	char pte_index;
	// BASE
	char pmd_base;
	char pt_base;

	int i, j, k, h;

	// find cr3
	cr3 = find_cr3(mem, pid);
	if (cr3 == -1) {
		return (-1);
	}

	// find PD, PMD, PT index
	// example
	// #################################################################
	// # PD_MASK 0xC0 -> 11000000
	// # va & PD_MASK -> get 2bits from the front
	// # when shift right 6 bits -> get 2bits from the front only
	// #################################################################
	pde_index = PD_MASK & va;
	pde_index = pde_index >> 6;
	pmde_index = PMD_MASK & va;
	pmde_index = pmde_index >> 4;
	pte_index = PT_MASK & va;
	pte_index = pte_index >> 2;

	//PD is already exist, because of run_proc
	if ((ku_h_phys_mem[cr3].pte[pde_index] & get_Present) == 0) {
		int flag = 0;
		//new PMD in PDE, and allocate in real_memory
		for (i = 0; i < ku_h_phys_index; i++) {
			if (ku_h_phys_list[i].pid == -1) {
				flag = 1;
				break;	//if flag == 1, exist free page for new PD
				//use i for PMD
			}
		}

		if (flag == 1) {
			ku_h_phys_list[i].pid = pid;

			pmd_base = i;
			ku_h_phys_mem[cr3].pte[pde_index] = (i << 2) + 1;
		}
		else {
			//swap out
			int now = 0;
			//First, check(can swap out page frame is exist)
			while (flag == 0) {
				for (j = 0; j < ku_h_phys_index; j++) {
					if (ku_h_phys_list[j].priority == now) {
						flag = 1;
						break;
					}
				}
				now++;

				if (now == 8193) {
					return -1;
					//doesn't exist page to swap out
				}
			}

			flag = 0;
			//Second check(can swap out, if swap page is full or not full)
			for (k = 0; k < ku_h_swap_index; k++) {
				if (ku_h_swap_list[k].pid == -1) {
					flag = 1;
					break;
				}
			}
			//Third, exist and can swap out -> swap out (change PTE to swap index+1, and allocate PD)
			if (flag == 1) {
				char s_cr3;
				char buf, buf2, buf3;
				int a, b, c;

				//exist swap memory to swap out
				for (h = 0; h < ku_h_pcb_size; h++) {
					if (ku_h_pcb[h].pid == ku_h_phys_list[j].pid) {
						s_cr3 = ku_h_pcb[h].ku_cr3;
						break;
					}
				}
				//look ku_cr3 -> find all PD, PMD, PT->PTE
				for (a = 0; a < 4; a++) {
					if (ku_h_phys_mem[s_cr3].pte[a] != 0) {
						buf = ku_h_phys_mem[s_cr3].pte[a];
						buf = buf >> 2;
						for (b = 0; b < 4; b++) {
							if (ku_h_phys_mem[buf].pte[b] != 0) {
								buf2 = ku_h_phys_mem[buf].pte[b];
								buf2 = buf2 >> 2;
								for (c = 0; c < 4; c++) {
									buf3 = ku_h_phys_mem[buf2].pte[c];
									buf3 = buf3 >> 2;
									if (buf3 == j) {
										char d;
										d = k;
										d++;
										d = d << 1;

										ku_h_phys_mem[buf2].pte[c] = d;
									}
								}
							}
						}
					}
				}

				ku_h_phys_list[j].pid = pid;
				ku_h_phys_list[j].priority = 8194;
				ku_h_swap_list[k].pid = ku_h_pcb[h].pid;

				ku_h_phys_mem[cr3].pte[pde_index]=(j<<2)+1;
				pmd_base = j;
			}
			else {
				//not exist swap free page
				return -1;
			}
		}
	}
	else {
		//PMD is allocated
		pmd_base = ku_h_phys_mem[cr3].pte[pde_index] >> 2;
	}

	//PMD is allocated
	if ((ku_h_phys_mem[pmd_base].pte[pmde_index] & get_Present) == 0) {
		int flag = 0;
		//new PT in PMDE, and allocate in real_memory
		for (i = 0; i < ku_h_phys_index; i++) {
			if (ku_h_phys_list[i].pid == -1) {
				flag = 1;
				break;	//if flag == 1, exist free page for new PD
				//use i for PT
			}
		}

		if (flag == 1) {
			ku_h_phys_list[i].pid = pid;

			pt_base = i;
			ku_h_phys_mem[pmd_base].pte[pmde_index] = (i << 2) + 1;
		}
		else {
			//swap out
			int now = 0;
			//First, check(can swap out page frame is exist)
			while (flag == 0) {
				for (j = 0; j < ku_h_phys_index; j++) {
					if (ku_h_phys_list[j].priority == now) {
						flag = 1;
						break;
					}
				}
				now++;

				if (now == 8193) {
					return -1;
					//doesn't exist page to swap out
				}
			}

			flag = 0;
			//Second check(can swap out, if swap page is full or not full)
			for (k = 0; k < ku_h_swap_index; k++) {
				if (ku_h_swap_list[k].pid == -1) {
					flag = 1;
					break;
				}
			}
			//Third, exist and can swap out -> swap out (change PTE to swap index+1, and allocate PD)
			if (flag == 1) {
				char s_cr3;
				char buf, buf2, buf3;
				int a, b, c;

				//exist swap memory to swap out
				for (h = 0; h < ku_h_pcb_size; h++) {
					if (ku_h_pcb[h].pid == ku_h_phys_list[j].pid) {
						s_cr3 = ku_h_pcb[h].ku_cr3;
						break;
					}
				}
				//look ku_cr3 -> find all PD, PMD, PT -> PTE
				for (a = 0; a < 4; a++) {
					if (ku_h_phys_mem[s_cr3].pte[a] != 0) {
						buf = ku_h_phys_mem[s_cr3].pte[a];
						buf = buf >> 2;
						for (b = 0; b < 4; b++) {
							if (ku_h_phys_mem[buf].pte[b] != 0) {
								buf2 = ku_h_phys_mem[buf].pte[b];
								buf2 = buf2 >> 2;
								for (c = 0; c < 4; c++) {
									buf3 = ku_h_phys_mem[buf2].pte[c];
									buf3 = buf3 >> 2;
									if (buf3 == j) {
										char d;
										d = k;
										d++;
										d = d << 1;

										ku_h_phys_mem[buf2].pte[c] = d;
									}
								}
							}
						}
					}
				}

				ku_h_phys_list[j].pid = pid;
				ku_h_phys_list[j].priority = 8194;
				ku_h_swap_list[k].pid = ku_h_pcb[h].pid;
				pt_base = j;

				ku_h_phys_mem[pmd_base].pte[pmde_index]=(j<<2)+1;
			}
			else {
				//not exist swap free page
				return -1;
			}
		}
	}
	else {
		//PMDE is allocated
		pt_base = ku_h_phys_mem[pmd_base].pte[pmde_index] >> 2;
	}

	//PT is allocated
	//first. new allocated Page Frame, second. swapped out Page frame
	if (ku_h_phys_mem[pt_base].pte[pte_index] == 0) {
		//new PF, allocate in real memory
		int flag = 0;

		for (i = 0; i < ku_h_phys_index; i++) {
			if (ku_h_phys_list[i].pid == -1) {
				flag = 1;
				break;	//if flag == 1, exist free page for new PD
			}
		}

		if (flag == 1) {
			ku_h_phys_list[i].pid = pid;
			ku_h_phys_list[i].priority = ku_h_priority;
			ku_h_priority++;
			ku_h_phys_mem[pt_base].pte[pte_index] = (i << 2) + 1;
			
			return 0;
		}
		else {
			//swap out
			int now = 0;
			//First, check(can swap out page frame is exist)
			while (flag == 0) {
				for (j = 0; j < ku_h_phys_index; j++) {
					if (ku_h_phys_list[j].priority == now) {
						flag = 1;
						break;
					}
				}
				now++;

				if (now == 8193) {
					return -1;
					//doesn't exist page to swap out
				}
			}

			flag = 0;
			//Second, check can swap out, if swap page is full or not full
			for (k = 0; k < ku_h_swap_index; k++) {
				if (ku_h_swap_list[k].pid == -1) {
					flag = 1;
					break;
				}
			}
			//Third, exist and can swap out -> swap out (change PTE to swap index + 1, and allocate PD)
			if (flag == 1) {
				char s_cr3;
				char buf, buf2, buf3;
				int a, b, c;

				//exist swap memory to swap out
				for (h = 0; h < ku_h_pcb_size; h++) {
					if (ku_h_pcb[h].pid == ku_h_phys_list[j].pid) {
						s_cr3 = ku_h_pcb[h].ku_cr3;
						break;
					}
				}
				//look ku_cr3 => find all PD, PMD, PT -> PTE
				for (a = 0; a < 4; a++) {
					if (ku_h_phys_mem[s_cr3].pte[a] != 0) {
						buf = ku_h_phys_mem[s_cr3].pte[a];
						buf = buf >> 2;
						for (b = 0; b < 4; b++) {
							if (ku_h_phys_mem[buf].pte[b] != 0) {
								buf2 = ku_h_phys_mem[buf].pte[b];
								buf2 = buf2 >> 2;
								for (c = 0; c < 4; c++) {
									buf3 = ku_h_phys_mem[buf2].pte[c];
									buf3 = buf3 >> 2;
									if (buf3 == j) {
										char d;
										d = k;
										d++;
										d = d << 1;

										ku_h_phys_mem[buf2].pte[c] = d;
									}
								}
							}
						}
					}
				}

				ku_h_phys_list[j].pid = pid;
				ku_h_phys_list[j].priority = ku_h_priority;
				ku_h_priority++;
				ku_h_swap_list[k].pid = ku_h_pcb[h].pid;

				ku_h_phys_mem[pt_base].pte[pte_index] = (j << 2) + 1;
			
				return 0;
			}
			else {
				//not exist swap free page
				return -1;
			}

			return -1;
		}
	}
	else {
		//swapped out PF
		//swap index>>1 , -1 -> swap real index
		int flag = 0;
		char swap_s;
		//new PF allocate in real _memory
		for (i = 0; i < ku_h_phys_index; i++) {
			if (ku_h_phys_list[i].pid == -1) {
				flag = 1;
				break;
			}
		}

		if (flag == 1) {
			ku_h_phys_list[i].pid = pid;
			swap_s = ku_h_phys_mem[pt_base].pte[pte_index];
			swap_s = swap_s >> 1;
			swap_s--;

			ku_h_swap_list[swap_s].pid = -1;
			ku_h_phys_mem[pt_base].pte[pte_index] = (i << 2) + 1;

			return 0;
		}

		else {
			//swap out
			int now = 0;
			while (flag == 0) {
				for (j = 0; j < ku_h_phys_index; j++) {
					if (ku_h_phys_list[j].priority == now) {
						flag = 1;
						break;
					}
				}
				now++;

				if (now == 8193) {
					return -1;
					//doesn't exist page to swap out
				}
			}

			flag = 0;
			//Second check(can swap out, if swap page is full or not full)
			for (k = 0; k < ku_h_swap_index; k++) {
				if (ku_h_swap_list[k].pid == -1) {
					flag = 1;
					break;
				}
			}
			//Third, exist and can swap out-> swap out (change PTe to swap index+1, and allocate PF)
			if (flag == 1) {
				char s_cr3;
				char buf, buf2, buf3;
				int a, b, c;

				//exist swap memory to swap out
				for (h = 0; h < ku_h_swap_index; h++) {
					if (ku_h_pcb[h].pid == ku_h_phys_list[j].pid) {
						s_cr3 = ku_h_pcb[h].ku_cr3;
						break;
					}
				}

				for (a = 0; a < 4; a++) {
					if (ku_h_phys_mem[s_cr3].pte[a] != 0) {
						buf = ku_h_phys_mem[s_cr3].pte[a];
						buf = buf >> 2;
						for (b = 0; b < 4; b++) {
							if (ku_h_phys_mem[buf].pte[b] != 0) {
								buf2 = ku_h_phys_mem[buf].pte[b];
								buf2 = buf2 >> 2;
								for (c = 0; c < 4; c++) {
									buf3 = ku_h_phys_mem[buf2].pte[c];
									buf3 = buf3 >> 2;
									if (buf3 == j) {
										char d;
										d = k;
										d++;
										d = d << 1;

										ku_h_phys_mem[buf2].pte[c] = d;
									}
								}
							}
						}
					}
				}

				ku_h_phys_list[j].pid = pid;
				ku_h_phys_list[j].priority=ku_h_priority;
				ku_h_priority++;
				ku_h_swap_list[k].pid = ku_h_pcb[h].pid;

				swap_s = ku_h_phys_mem[pt_base].pte[pte_index];
				swap_s = swap_s >> 1;
				swap_s--;

				ku_h_swap_list[swap_s].pid = -1;
				ku_h_phys_mem[pt_base].pte[pte_index] = (j << 2) + 1;

				return 0;
			}
			else {
				//not exist swap free page
				return -1;
			}
		}
		//change swap list to initialize value
	}

	return -1;
}