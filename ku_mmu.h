#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ku_pte {
	char pte;
};

struct page {
	char pte[4];
};

struct list {
	char pid;			//if pid == -1 -> not allocated, pid !=0 -> allocated
	short priority;		//for FIFO(physical memory), no use for SWAPMEMORY
};

struct pcb {
	char pid;
	char ku_cr3;
};

int ku_page_fault(char pid, char va);
void* ku_mmu_init(unsigned int mem_size, unsigned int swap_size);
int ku_run_proc(char pid, struct ku_pte **ku_cr3);

int ku_h_phys_size;
int ku_h_swap_size;
int ku_h_pcb_size = 0;
int ku_h_phys_index;
int ku_h_swap_index;
int ku_h_priority = 0;

struct page *ku_h_phys_mem;
struct list *ku_h_phys_list;
struct list *ku_h_swap_list;
struct pcb *ku_h_pcb;

int ku_page_fault(char pid, char va) {
	char cr3 = -1;
	int i, j, k, h;
	char PD_MASK = 0xC0;
	char PMD_MASK = 0x30;
	char PT_MASK = 0xC;
	char get_Present = 0x1;
	char pde_index;
	char pmde_index;
	char pte_index;

	char pmd_base;
	char pt_base;

	//Find cr3
	for (i = 0; i < ku_h_pcb_size; i++) {
		if (ku_h_pcb[i].pid == pid) {
			cr3 = ku_h_pcb[i].ku_cr3;
			break;
		}
	}
	//no exist pid
	if (cr3 == -1) {
		return -1;
	}

	//Find PD, PMD, PT index
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

//init phys_memory, swap_memory, phys_memory list, swap_memory list, and pcb
void* ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
	int i;
	ku_h_phys_index = mem_size / 4;
	ku_h_swap_index = swap_size / 4;
	ku_h_phys_size = mem_size;
	ku_h_swap_size = swap_size;
	int in = 0;
	//allocate phys memory, swap memory
	ku_h_phys_mem = malloc(sizeof(struct page)*ku_h_phys_index);
	//allocate free list (phys memory, swap memory)
	ku_h_phys_list = malloc(sizeof(struct list)*ku_h_phys_index);
	ku_h_swap_list = malloc(sizeof(struct list)*ku_h_swap_index);
	//allocate pcb
	ku_h_pcb = malloc(sizeof(struct pcb) * 128);

	//initialize
	for (i = 0; i < ku_h_phys_index; i++) {
		memcpy(&ku_h_phys_mem[i], &in, sizeof(struct page));
		ku_h_phys_list[i].pid = -1;
		ku_h_phys_list[i].priority = 8194;	//for maximum
	}
	for (i = 0; i < ku_h_swap_index; i++) {
		ku_h_swap_list[i].pid = -1;
		ku_h_swap_list[i].priority = 0;	//in swap priority is not used
	}
	for (i = 0; i < 128; i++) {
		ku_h_pcb[i].pid = -1;
		ku_h_pcb[i].ku_cr3 = -1;
	}

	return ku_h_phys_mem;
}

int ku_run_proc(char pid, struct ku_pte **ku_cr3) {
	int i, j, k, h, g;
	int flag = 0;
	int in = 0;

	//for first process
	if (ku_h_pcb_size == 0) {
		ku_h_pcb[0].pid = pid;
		ku_h_pcb[0].ku_cr3 = 0;
		ku_h_pcb_size++;

		*(ku_cr3) = &(ku_h_phys_mem[ku_h_pcb[0].ku_cr3].pte[0]);
	
		ku_h_phys_list[0].pid = pid;
		//not initialized priority because PD page is not swap out
		//list is set but not allocated PMD so can't allocate in the real memory
		return 0;
	}

	else {
		for (i = 0; i < ku_h_phys_size; i++) {
			if (ku_h_phys_list[i].pid == pid) {
				flag = 1;
				break;		//i is for ku_h_pcb index
			}
		}
		//exist pid
		if (flag == 1) {
			*(ku_cr3)=&(ku_h_phys_mem[ku_h_pcb[i].ku_cr3].pte[0]);
			return 0;
		}

		//not exist pid
		else {
			for (j = 0; j < ku_h_phys_index; j++) {
				if (ku_h_phys_list[j].pid == -1) {
					flag = 1;
					break;		//if flag ==1, exist free page for new PD
				}
			}
		}
		if (flag == 1) {
			ku_h_pcb[ku_h_pcb_size].pid = pid;
			ku_h_pcb[ku_h_pcb_size].ku_cr3 = j;
			ku_h_pcb_size++;

			*(ku_cr3)=&(ku_h_phys_mem[j].pte[0]);
	
			ku_h_phys_list[j].pid = pid;
			//not initialized priority because PD page is not swap out
			//list is set but not allocated PMD so can't allocate in the real memory

			return 0;
		}
		else {
			//need swap out
			int now = 0;
			//First, check (can swap out page frame is exist)
			while (flag == 0) {
				for (k = 0; k < ku_h_phys_index; k++) {
					if (ku_h_phys_list[k].priority == now) {
						flag = 1;
						break;	//k is now to swap out page index
					}
				}
				now++;

				if (now == 8193) {
					return -1;
					//doesn't exist page to swapout
				}
			}
			flag = 0;
			//Second, check (can swap out, if swap page is full or not full)
			for (h = 0; h < ku_h_swap_index; h++) {
				if (ku_h_swap_list[h].pid == -1) {
					flag = 1;
					break;
				}
			}
			//Third, exist and can swap out -> swap out(change PTE to swap index+0, and allocate PD)
			if (flag == 1) {
				char cr3;
				char buf, buf2, buf3;
				int a, b, c;

				//exist swap memory to swap out
				//find ku_cr3 of swap out page frame
				for (g = 0; g < ku_h_pcb_size; g++) {
					if (ku_h_pcb[g].pid == ku_h_phys_list[k].pid) {
						cr3 = ku_h_pcb[g].ku_cr3;
						break;
					}
				}
				//look ku_cr3 -> find all PD, PMD, PT->PTE
				for (a = 0; a < 4; a++) {
					if (ku_h_phys_mem[cr3].pte[a] != 0) {
						buf = ku_h_phys_mem[cr3].pte[a];
						buf = buf >> 2;
						for (b = 0; b < 4; b++) {
							if (ku_h_phys_mem[buf].pte[b] != 0) {
								buf2 = ku_h_phys_mem[buf].pte[b];
								buf2 = buf2 >> 2;
								for (c = 0; c < 4; c++) {
									buf3 = ku_h_phys_mem[buf2].pte[c];
									buf3 = buf3 >> 2;
									if (buf3 == k) {
										char d;
										d = h;
										d++;
										d = d << 1;

										ku_h_phys_mem[buf2].pte[c] = d;
									}
								}
							}
						}
					}
				}

				ku_h_phys_list[k].pid = pid;
				ku_h_phys_list[k].priority = 8194;

				ku_h_pcb[ku_h_pcb_size].pid = pid;
				ku_h_pcb[ku_h_pcb_size].ku_cr3 = k;
				ku_h_pcb_size++;

				*(ku_cr3)=&(ku_h_phys_mem[k].pte[0]);

				ku_h_swap_list[h].pid = ku_h_pcb[g].pid;
				//if pte>>2 == k
				//memcpy &pte, h+1<<1  + 1
				//list -> pid = pid, list->priority=8194
				//pcb[pcb_size].pid=pid, pcb[ku_cr3]=k
				//(*ku_cr3)=k
				//swap_list[h].pid=ku_h_pcb[g].pid


			}
			else {
				//not exist swap memory to swap out
				return -1;
			}
		}
	}
	return -1;
}

