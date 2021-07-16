#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
/***********************************************************************
 * Use this function to allocate pipe info && Don't Modify below function
 ***********************************************************************/
struct pipe_info* alloc_pipe_info()
{
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    pipe ->pipe_buff = buffer;
    return pipe;
}


void free_pipe_info(struct pipe_info *p_info)
{
    if(p_info)
    {
        os_page_free(OS_DS_REG ,p_info->pipe_buff);
        os_page_free(OS_DS_REG ,p_info);
    }
}
/*************************************************************************/
/*************************************************************************/


int pipe_read(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Read the contect from buff (pipe_info -> pipe_buff) and write to the buff(argument 2);
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    //permission checking
    if( filep->mode & O_READ )
    {
        //current read pointer
        int r_index = filep->pipe->read_pos;
        //storing offset so that in case of error changes can be made
        int ioffset = filep->pipe->buffer_offset;
        //read count stores # of letter counted
        int read_count = 0;
        for(int i=0;i<count;++i)
        {
            // if no letter then break for further checking 
            if( filep->pipe->buffer_offset <= 0)
                break;
            //index finiding for cyclicity
            int index = ( i + r_index)%4096;
            //part of reading
            buff[i] = filep->pipe->pipe_buff[index];
            //offset changes
            filep->pipe->buffer_offset = filep->pipe->buffer_offset - 1;
            filep->pipe->read_pos = index;
            //to keep characters conted
            ++read_count;
        }
        //case when reading of entire count is not done
        if( count != read_count )
        {
            //restoring to initial conditions
            filep->pipe->read_pos = r_index;
            filep->pipe->buffer_offset = ioffset;
            //returning the offset
            return -EOTHERS;
        }
        return read_count;
    }
    else
        return -EACCES;
    
}


int pipe_write(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Write the contect from   the buff(argument 2);  and write to buff(pipe_info -> pipe_buff)
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */

    //permission check
    
   
    if( filep->mode & O_WRITE )
    {
        //storing info so that in case of error restoration can take place
    	int w_index = filep->pipe->write_pos;
        int ioffset = filep->pipe->buffer_offset;
    	// writing part
    	for(int i=0;i<count;++i)
    	{	
    		//for cyclicity
    		int index = (w_index + i)%4096;
    		//part of writing
    		filep->pipe->pipe_buff[index] = buff[i];
    		//incrementing buffer_offset and write position
    		filep->pipe->buffer_offset = filep->pipe->buffer_offset + 1;
    		filep->pipe->write_pos = index;
    		// check of writing part if mempry exceeded then error
    		if( filep->pipe->buffer_offset > 4096)
    			break;
    	}
        //in case of error restoring everything
        if( filep->pipe->buffer_offset > 4096 )
        {
            filep->pipe->write_pos = w_index;
            filep->pipe->buffer_offset = ioffset;
            return -EOTHERS;
        }
    	return count;
    }
    else
    	return -EACCES;
}

int create_pipe(struct exec_context *current, int *fd)
{
    /**
    *  TODO:: Implementation of Pipe Create
    *  Create file struct by invoking the alloc_file() function, 
    *  Create pipe_info struct by invoking the alloc_pipe_info() function
    *  fill the valid file descriptor in *fd param
    *  Incase of Error return valid Error code 
    */

    // just finding appropriate fds

    int i=0;
    for(;i<32;++i)
    {
        if( current->files[i] == NULL )
            break;
    }
    int j=i+1;
    for(;j<32;++j)
    {
        if( current->files[j] == NULL )
            break;
    }
    if(i>=32 || j>=32)
        return -EOTHERS;
    fd[0]=i;
    fd[1]=j;

    //making of the main pipe block
    struct pipe_info* pipe_block = alloc_pipe_info();
    pipe_block->is_ropen = 1;
    pipe_block->is_wopen = 1;
    pipe_block->read_pos = 0;
    pipe_block->write_pos = 0;
    pipe_block->buffer_offset = 0;
    //done making the pipe_block

    //Preparing the write port
    struct file* wport = alloc_file();
    wport->type = PIPE;
    wport->mode = O_WRITE;
    wport->offp = 0;                        //does not matter 
    wport->ref_count = 1;                   //important as it keeps track of write ports
    wport->inode = NULL;                    // for completness
    wport->fops->write = pipe_write;              //maping the functions
    wport->fops->close = generic_close;              //different close
    wport->pipe = pipe_block;
    current->files[j] = wport;

    //Preparing the read port
    struct file* rport = alloc_file();
    rport->type = PIPE;
    rport->mode = O_READ;
    rport->offp = 0;                              //will not matter
    rport->ref_count = 1;                         //keeping track of write ports
    rport->inode = NULL;
    rport->fops->read = pipe_read;
    rport->fops->close = generic_close;
    rport->pipe = pipe_block;
    current->files[i] = rport;


 
    return 0;
}

