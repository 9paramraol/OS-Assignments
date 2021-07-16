#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>
#include<pipe.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/
void free_file_object(struct file *filep)
{
    if(filep)
    {
       os_page_free(OS_DS_REG ,filep);
       stats->file_objects--;
    }
}

struct file *alloc_file()
{
  
  struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
  file->fops = (struct fileops *) (file + sizeof(struct file)); 
  bzero((char *)file->fops, sizeof(struct fileops));
  stats->file_objects++;
  return file; 
}

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
  kbd_read(buff);
  return 1;
}

static int do_write_console(struct file* filep, char * buff, u32 count)
{
  struct exec_context *current = get_current_ctx();
  return do_write(current, (u64)buff, (u64)count);
}

struct file *create_standard_IO(int type)
{
  struct file *filep = alloc_file();
  filep->type = type;
  if(type == STDIN)
     filep->mode = O_READ;
  else
      filep->mode = O_WRITE;
  if(type == STDIN){
        filep->fops->read = do_read_kbd;
  }else{
        filep->fops->write = do_write_console;
  }
  filep->fops->close = generic_close;
  filep->ref_count = 1;
  return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
   int fd = type;
   struct file *filep = ctx->files[type];
   if(!filep){
        filep = create_standard_IO(type);
   }else{
         filep->ref_count++;
         fd = 3;
         while(ctx->files[fd])
             fd++; 
   }
   ctx->files[fd] = filep;
   return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/



void do_file_fork(struct exec_context *child)
{
   /*TODO the child fds are a copy of the parent. Adjust the refcount*/
   for(int i=0;i<32;++i)
   {
      if( child->files[i] )
          child->files[i]->ref_count = child->files[i]->ref_count + 1;
   }
}

void do_file_exit(struct exec_context *ctx)
{
   /*TODO the process is exiting. Adjust the ref_count
     of files*/
   for(int i=0;i<32;++i)
   {
      if( ctx->files[i] )
      {
          generic_close(ctx->files[i]);
      }
      ctx->files[i] = NULL;
   }

}

long generic_close(struct file *filep)
{
  /** TODO Implementation of close (pipe, file) based on the type 
   * Adjust the ref_count, free file object
   * Incase of Error return valid Error code 
   * 
   */
    //case when the filep is null or not there
    if( filep == NULL )
      return -EINVAL;
    else if( filep->type == PIPE )
    {
      filep->ref_count = filep->ref_count - 1; 
      if( filep->ref_count == 0)
      {
        //checking which port to close
        if( filep->mode & O_READ )
          filep->pipe->is_ropen = 0;
        // in case when all write port are closed EOF character has to be stored in buffer
        else if( filep->mode & O_WRITE ){
          //checking offset if full then cant print eof
          //if buffer is not full then just print EOF and close the write port
          filep->pipe->is_wopen = 0;
        }
        //checking is the pipe structure is required or not
        //finally just close the pipe part
        if( filep->pipe->is_ropen==0 && filep->pipe->is_wopen==0)
          free_pipe_info( filep->pipe );
        //since the ref_count is zero so no need of the file structure
        free_file_object(filep);
        return 0;
      }
    }
    else if( filep->type == REGULAR )
    {
      //case 1 if there is just one link
        if( filep->ref_count == 1 )
        {
            free_file_object(filep);
            return 0;  // if the exectution goes all well
        }
        // more than one links
        else if( filep->ref_count > 1 )
        {
            filep->ref_count = filep->ref_count - 1;
            filep = NULL;
            return 0;  // if the exectution goes all well
        }  
        else
          return -EOTHERS; // some other random error
    }
    //case for stdin stderr stdout
    else
    {
        if( filep->ref_count == 1 )
        {
            free_file_object(filep);
            return 0;  // if the exectution goes all well
        }
        // more than one links
        else if( filep->ref_count > 1 )
        {
            filep->ref_count = filep->ref_count - 1;
            filep = NULL;
            return 0;  // if the exectution goes all well
        }  
        else
          return -EOTHERS; // some other random error
    }


    
}

static int do_read_regular(struct file *filep, char * buff, u32 count)
{
   /** TODO Implementation of File Read, 
    *  You should be reading the content from File using file system read function call and fill the buf
    *  Validate the permission, file existence, Max length etc
    *  Incase of Error return valid Error code 
    * */
    // in case the fd given to us is null
    if( filep == NULL)
      return -EINVAL;
    // to ensure that reading permission are given
    if( filep->mode & 0x1 )
    {
       int bytes_read = flat_read( filep->inode , buff , count , &(filep->offp) );
       //updating the offset
       if( count != bytes_read )
        return -EOTHERS;
       filep->offp = filep->offp + bytes_read;
       //if EOF is reached the o is given out and # bytes takes case of it
       return bytes_read;
    }
    //if permission to read is not given in file object
    else
    {
      return -EACCES;
    }
    
}


static int do_write_regular(struct file *filep, char * buff, u32 count)
{
    /** TODO Implementation of File write, 
    *   You should be writing the content from buff to File by using File system write function
    *   Validate the permission, file existence, Max length etc
    *   Incase of Error return valid Error code 
    * */
    // in case the fd given to us does not exist
    if( filep == NULL)
      return -EINVAL;
    //to ensure that wrtie permission is given to the fd
    if( filep->mode & 0x2 )
    {
        int bytes_wrote = flat_write( filep->inode , buff , count , &(filep->offp) );
        //checking that the for writing enough space is there
        if ( (bytes_wrote == -1) || ( (filep->inode->file_size) > 4096) )
            return -ENOMEM;
        //uodating the offset and then going on
        filep->offp = filep->offp + bytes_wrote;
        //checking that the write is not more than 4KB
        return bytes_wrote;
    }
    //if permission to write is not given to the fd
    else
    {
      return -EACCES;
    }
}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{
    /** TODO Implementation of lseek 
    *   Set, Adjust the ofset based on the whence
    *   Incase of Error return valid Error code 
    * */

    if( filep == NULL )
      return -EINVAL;
    // case when you have to give pointer from start
    if( whence == SEEK_SET ){
        //error checking that the pointer does not runs out
       if( offset > 0x1000 )
          return -ENOMEM;
       filep->offp = offset;
    }
    // give pointer from current poistion
    else if( whence == SEEK_CUR ){
     //case when pointer foes out of memory
      if( filep->offp + offset > 0x1000 )
        return -ENOMEM;
      filep->offp = filep->offp + offset;
    }
    // gives pointer from the end
    else if( whence == SEEK_END ){
    //case where mempry is exceeded
      if ( ( filep->inode->max_pos - filep->inode->s_pos + offset ) > 0x1000 )
        return -ENOMEM;
      filep->offp = filep->inode->max_pos - filep->inode->s_pos + offset;
    }
    

    // if everything goes right
    return filep->offp;
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{ 
  /**  TODO Implementation of file open, 
    *  You should be creating file(use the alloc_file function to creat file), 
    *  To create or Get inode use File system function calls, 
    *  Handle mode and flags 
    *  Validate file existence, Max File count is 32, Max Size is 4KB, etc
    *  Incase of Error return valid Error code 
    * */

    int ret_fd;
    //getting the free index
    // CHECK # 1 for file descriptor avilability
    int i=3;
    for(;i<32;++i)
    {
      if(ctx->files[i]==NULL)
        break;
    }
    //no file descriptor is avilable
    if (i==32)
    {
      ret_fd = -EOTHERS;
      return ret_fd;
    }

    
        
    // after that making the inode and file structure   
    struct inode* curr_inode;
    struct file* opfile = alloc_file();
    if( opfile == NULL )
      return -EOTHERS;  // just caution

    //Part where u were given the right to open a new file
    if( flags & 0x8 )
    {
        // if file present then don't open and return error
        curr_inode = lookup_inode(filename);
        if(curr_inode != NULL){
           free_file_object(opfile);    
           return -EOTHERS;       
        }
        // if not present then do open
        else{
          //passing the mode and the file name
          curr_inode = create_inode(filename,mode);
          if(curr_inode == NULL)
            return -EOTHERS;
          opfile->inode = curr_inode;
        }
    }
    //when O_OPEN was not given
    else  
    {
       curr_inode = lookup_inode(filename);
       // could not finde file so retuen error
       if(curr_inode == NULL)
       {
          ret_fd = -EINVAL;
          return ret_fd;   
       }
       opfile->inode = curr_inode;
    }

  //check permission of inode and give the permissions to file
   opfile->mode = 0x0;
   //read part in flag
   if ((flags & 0x1))
   {  //also there in inode mode
      if((curr_inode->mode & 0x1)){
          opfile->mode = opfile->mode | 0x1;
      }
      else
      {
        ret_fd = -EACCES;
        return ret_fd;
      }
   }
   //write part in flag
   if (flags & 0x2)
   {
      //access also provided by inode
      if(curr_inode->mode & 0x2){
          opfile->mode = opfile->mode | 0x2;
      }
      else
      {
        ret_fd = -EACCES;
        return ret_fd;
      }

   }
   //exec part permission is given 
   if ((flags & 0x4))
   {
      //also there in the inode part
      if((curr_inode->mode & 0x4))
          opfile->mode = opfile->mode | 0x4;
      else
      {
        ret_fd = -EACCES;
        return ret_fd;
      }

   }
   //file operation of close,lseek,read and write 
   opfile->fops->lseek = do_lseek_regular;
   opfile->fops->read = do_read_regular;
   opfile->fops->write = do_write_regular;
   opfile->fops->close = generic_close;
   //before executing the functions of read and write do check the permissions
   //final touches   
   ret_fd = i;
   //returning fd
   opfile->offp = 0x0;
   // no offset is there as of now as no read or write operations have occured
   // mode has already been set remember
   opfile->ref_count = 1;
   //ref count is number of fd's pointing to this file currently one
   opfile->type = REGULAR;
   //not pipe so regular
   ctx->files[i] = opfile;
   //associating file descriptor with the file structure     
   return ret_fd;
}

int fd_dup(struct exec_context *current, int oldfd)
{
     /** TODO Implementation of dup 
      *  Read the man page of dup and implement accordingly 
      *  return the file descriptor,
      *  Incase of Error return valid Error code 
      * */

    //oldfd was null then also error must be raised
    if ( current->files[oldfd] == NULL )
        return -EINVAL;
    //checking that number of fd's corrseponding to a given dup call
    int ret_fd=0;
    //avilability of file descriptor
    for(;ret_fd<32;++ret_fd)
    {
      if( current->files[ret_fd] == NULL )
          break;
    }
    // no file descriptor avilable 
    if( ret_fd == 32 )
    {
      return -EOTHERS;
    }
    //main part of duping
    current->files[ret_fd] = current->files[oldfd];
    //incresing refrence count
    current->files[oldfd]->ref_count = current->files[oldfd]->ref_count + 1;

    return ret_fd;
}


int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
  /** TODO Implementation of the dup2 
    *  Read the man page of dup2 and implement accordingly 
    *  return the file descriptor,
    *  Incase of Error return valid Error code 
    * */

    //invalid file descriptor
    if(newfd < 0 || newfd > 31)
      return -EINVAL;  
    //case 1 old fd is null 
    if( current->files[oldfd] == NULL )
      return -EINVAL;
    //both the file descriptors point to same file structure no work
    if( current->files[oldfd] == current->files[newfd])
      return newfd;
    //first close the newfd make is same as oldfd
    else
    {   //the new fd is not null then only close it
        if( current->files[newfd] != NULL )
        {
          int check=generic_close(current->files[newfd]);
          if( check != 0 )
            return check;
        }
        current->files[oldfd]->ref_count = current->files[oldfd]->ref_count + 1;
        current->files[newfd] = current->files[oldfd];
        return newfd;
    }    
}
