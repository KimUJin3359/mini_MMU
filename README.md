# mini_MMU

### Requirements
#### Basic Info
- 3 level page table
  - `PUD` -> `PMD` -> `PTE`
- 8bits address
  - 2bits for PUD(Page Upper Directory) entry
  - 2bits for PMDE(Page Mid-level Directory) entry
  - 2bits for PTE(Page Table Entry) entry
  - 2bits for offset
- 4bytes page frame

#### Development Info
- Free list : having a info about physical memory\'s data
  - 

- Action
  - In virtual address space, find PUD, PMDE, PTE
