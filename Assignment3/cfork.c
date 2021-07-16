#include <cfork.h>
#include <page.h>
#include <mmap.h>

//Paramveer Raol 170459


/* You need to implement cfork_copy_mm which will be called from do_cfork in entry.c. Don't remove copy_os_pts()*/
void cfork_copy_mm(struct exec_context *child, struct exec_context *parent )
{
    //just physcial maping stuff
    void *os_addr;
    void *pos_addr;
    u64 vaddr; 
    struct mm_segment *seg;

    child->pgd = os_pfn_alloc(OS_PT_REG);

    os_addr = osmap(child->pgd);
    pos_addr = osmap(parent->pgd);
    bzero((char *)os_addr, PAGE_SIZE);
     
    //CODE segment
    seg = &parent->mms[MM_SEG_CODE];
    for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
        u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
        if(parent_pte){
             map_physical_page((u64) os_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
             map_physical_page((u64) pos_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
             asm volatile ("invlpg (%0);" 
                    :: "r"((vaddr)) 
                    : "memory");   // Flush TLB
             struct pfn_info * p = get_pfn_info((*parent_pte & FLAG_MASK) >> PAGE_SHIFT) ; 
             increment_pfn_info_refcount(p);   
        }
    } 
     //RODATA segment
     
     seg = &parent->mms[MM_SEG_RODATA];
     for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
        u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
        if(parent_pte){
             map_physical_page((u64) os_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
             map_physical_page((u64) pos_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
             asm volatile ("invlpg (%0);" 
                    :: "r"((vaddr)) 
                    : "memory");   // Flush TLB
             struct pfn_info * p = get_pfn_info((*parent_pte & FLAG_MASK) >> PAGE_SHIFT) ; 
             increment_pfn_info_refcount(p);
        }
     } 
     
     //DATA segment
    seg = &parent->mms[MM_SEG_DATA];
    for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
        u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
        if(parent_pte){
      		map_physical_page((u64) os_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
          map_physical_page((u64) pos_addr, vaddr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
          asm volatile ("invlpg (%0);" 
                    :: "r"((vaddr)) 
                    : "memory");   // Flush TLB
          struct pfn_info * p = get_pfn_info((*parent_pte & FLAG_MASK) >> PAGE_SHIFT) ; 
          increment_pfn_info_refcount(p); 
        }
    } 
    
    //STACK segment
    seg = &parent->mms[MM_SEG_STACK];
    for(vaddr = seg->end - PAGE_SIZE; vaddr >= seg->next_free; vaddr -= PAGE_SIZE){
        u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
        
       if(parent_pte){
             u64 pfn = install_ptable((u64)os_addr, seg, vaddr, 0);  //Returns the blank page  
             pfn = (u64)osmap(pfn);                                  //guess no pain
             memcpy((char *)pfn, (char *)(*parent_pte & FLAG_MASK), PAGE_SIZE);
             asm volatile ("invlpg (%0);" 
                    :: "r"((vaddr)) 
                    : "memory");   // Flush TLB 
        }
    } 
    ///heap part
    //copying virtual memory
    //checkin left
    struct vm_area * curr = parent->vm_area;
    struct vm_area * head = NULL;
    if( curr!=NULL ){
    	head = alloc_vm_area();
    	head->vm_start = curr->vm_start;
    	head->vm_end = curr->vm_end;
    	head->access_flags = curr->access_flags;
    	
    	struct vm_area * curr_next = head;
    	
    	curr = curr->vm_next;

    	while( curr )
    	{
    		struct vm_area * icurr = alloc_vm_area();
    		icurr->vm_start = curr->vm_start;
    		icurr->vm_end = curr->vm_end;
    		icurr->access_flags = curr->access_flags;
    		curr_next->vm_next = icurr;

    		curr = curr->vm_next;
    		curr_next = curr_next->vm_next;
    	} 

    	curr_next->vm_next = NULL;  // final one

    }

    // finally the phy memory
    //assuming previous one is correct
    child->vm_area = head;
    curr = child->vm_area;
    while(curr)
    {			
    		u64 s_addr = curr->vm_start;
    		u64 e_addr = curr->vm_end;

    		while( s_addr < e_addr){
    	 		 u64 *parent_pte =  get_user_pte(parent, s_addr, 0);
    				// maping both parent's and child's pgd to the same pte an making it read permissible
      		  	if(parent_pte){
            			 map_physical_page((u64) os_addr, s_addr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT); // child part
            			 map_physical_page((u64) pos_addr, s_addr, PROT_READ ,(*parent_pte & FLAG_MASK) >> PAGE_SHIFT); // parent part
                   asm volatile ("invlpg (%0);" 
                    :: "r"((s_addr)) 
                    : "memory");   // Flush TLB
            			 struct pfn_info * p = get_pfn_info((*parent_pte & FLAG_MASK) >> PAGE_SHIFT) ; 
        			     increment_pfn_info_refcount(p);
        			}
        			s_addr = s_addr + PAGE_SIZE;
        	}   
        	curr = curr->vm_next;      
    }

    //// 
    copy_os_pts(parent->pgd, child->pgd);

    return;
      
}

/* You need to implement cfork_copy_mm which will be called from do_vfork in entry.c.*/
void vfork_copy_mm(struct exec_context *child, struct exec_context *parent ){

    child->pgd = parent->pgd;
    parent->state = WAITING;

    void *os_addr = osmap(child->pgd);
    
    //new stack segment
    struct mm_segment * seg = &child->mms[MM_SEG_STACK];
    u64 new_start = (seg->next_free/PAGE_SIZE)*PAGE_SIZE;

    for( u64 vaddr = seg->end - PAGE_SIZE; vaddr >= seg->next_free; vaddr -= PAGE_SIZE){
        u64 *parent_pte =  get_user_pte(parent, vaddr, 0);
        
       if(parent_pte){

          u64 * check = get_user_pte(parent, new_start+vaddr-seg->end , 0);
          if( check==NULL ){ 
             u64 pfn = install_ptable((u64)os_addr, seg, new_start+vaddr-seg->end , 0);  //Returns the blank page  
             pfn = (u64)osmap(pfn);
             memcpy((char *)pfn, (char *)(*parent_pte & FLAG_MASK), PAGE_SIZE);
          }
          else{
              u64 check_pfn = (*check & FLAG_MASK)>>PAGE_SHIFT;
              u64 pfn = install_ptable((u64)os_addr, seg, new_start+vaddr-seg->end , check_pfn);  //Returns the blank page  
              pfn = (u64)osmap(pfn);
              memcpy((char *)pfn, (char *)(*parent_pte & FLAG_MASK), PAGE_SIZE);
          }
          asm volatile ("invlpg (%0);" 
                    :: "r"((new_start+vaddr-seg->end)) 
                    : "memory");   // Flush TLB             
        }
    }

    child->regs.entry_rsp = new_start + child->regs.entry_rsp - seg->end;
    child->regs.rbp = new_start + child->regs.rbp - seg->end;
    
    seg->next_free =   new_start + new_start - seg->end;
    return;    
}

/*You need to implement handle_cow_fault which will be called from do_page_fault 
incase of a copy-on-write fault

* For valid acess. Map the physical page 
 * Return 1
 * 
 * For invalid access,
 * Return -1. 
*/

int handle_cow_fault(struct exec_context *current, u64 cr2){
  // checking if address is present or not
  int present = 0;
  u64 vaddr;
  struct mm_segment * seg = &current->mms[MM_SEG_CODE];
  //CODE segment
  // since this part is read only always
  for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
      if( vaddr<=cr2 & (vaddr+PAGE_SIZE)>cr2 )
        if( seg->access_flags & PROT_WRITE )          // will never occur as the code segment is read only part
          present = 1;     
  } 
  //RODATA segment		   
  seg = &current->mms[MM_SEG_RODATA];
  for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
     if( vaddr<=cr2 & (vaddr+PAGE_SIZE)>cr2 )
     		if( seg->access_flags & PROT_WRITE )
     			present = 1;		  
  } 
   
   //DATA segment
  seg = &current->mms[MM_SEG_DATA];
  for(vaddr = seg->start; vaddr < seg->next_free; vaddr += PAGE_SIZE){
  		if( vaddr<=cr2 & (vaddr+PAGE_SIZE)>cr2 )
      		if( seg->access_flags & PROT_WRITE )
      			present = 1;    
  } 
  /* 
  //STACK segment
  seg = &current->mms[MM_SEG_STACK];
  for(vaddr = seg->end - PAGE_SIZE; vaddr >= seg->next_free; vaddr -= PAGE_SIZE){
      if( vaddr==cr2 )
      		if( seg->access_flags & PROT_WRITE )
      			present = 1;
  } 
  */
  // checking if the address is present in the heap part
  struct vm_area * curr = current->vm_area;
  while( curr )
  {
  	if( cr2>=curr->vm_start && cr2<curr->vm_end )
  	{
  		if( curr->access_flags & PROT_WRITE ){
  			present = 1;
  			break;
  		}
  	}
  	curr = curr->vm_next;
  }

  u64 *pte =  get_user_pte(current, cr2, 0);
  int refcount = 0;

  //checking if present and permissions are also ok
  if( present==0 )
  	return -1;
  
  if(pte){
        struct pfn_info * p = get_pfn_info((*pte & FLAG_MASK) >> PAGE_SHIFT);
        refcount = get_pfn_info_refcount( p );
  }

  if( refcount==1 && present==1 )
  {	
     asm volatile ("invlpg (%0);" 
                  :: "r"((cr2)) 
                  : "memory");   // Flush TLB     
  	  // if permissions are ok
	  u64* upfn=get_user_pte(current,cr2,0);
	  //changing the permission
	  *upfn = (((*upfn)>>3)<<3) | 0x7;
	  //ref count remains same
  }
  else if( refcount>1 && present==1 )
  {   

     asm volatile ("invlpg (%0);" 
                  :: "r"((cr2)) 
                  : "memory");   // Flush TLB     
  	//getting pte details
  	  u64 *ori_pte =  get_user_pte(current, cr2 , 0);     // the origial pte
        u64 dummy_ori = *ori_pte;
        struct pfn_info * p = get_pfn_info(((*ori_pte) >> PAGE_SHIFT));
        decrement_pfn_info_refcount(p);
        int refcount = get_pfn_info_refcount(p);
       // finding blank page for the memory address
        void * os_addr = osmap(current->pgd);
        u64 pfn = os_pfn_alloc(USER_REG);
        pfn = pfn << PAGE_SHIFT;
        memcpy((char *)pfn, (char *)(dummy_ori & FLAG_MASK), PAGE_SIZE); // copying the phy page
        *ori_pte = pfn | 0x7;
  }
  else
  	return -1;

  return 1;
}

/* You need to handle any specific exit case for vfork here, called from do_exit*/
void vfork_exit_handle(struct exec_context *ctx){
  //getting ppid and then 1.changing head 2.
  if( ctx==NULL )
    return;
  struct exec_context * parent = get_ctx_by_pid(ctx->ppid);
  if( parent==NULL || parent->regs.rax!=ctx->pid || parent->pgd!=ctx->pgd )
    return;
  parent->state = READY;
  parent->vm_area = ctx->vm_area;
  parent->mms[MM_SEG_DATA] = ctx->mms[MM_SEG_DATA];
  return;
}