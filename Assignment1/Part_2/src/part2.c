//Paramveer Raol 170459
#include<stdlib.h> 
#include<unistd.h> 
#include<stdio.h> 
#include<fcntl.h> 
#include<string.h>


int main(int args,char *argc[])
{
	if(strcmp(argc[1],"@")==0) // the case where fixed type of computations are expected
	{
		int fd[2];                // creating a pipe
		if(pipe(fd)<0)
		{
			perror("line 15");
			exit(1);
		}	
		int pid=fork();           // creating a fork so that one process greping can be carried out in one process and 
								  //written in pipe then after that reading can be done from the same pipe
		if(pid<0)
		{
			printf("error\n");
			exit(1);
		}
		else if(pid==0)                 // In child process the dup2 (b/w stdout and write fd)is used and grep is applied so that the stdout 
										//gets written in the pipe's write filedescriptor
		{
			dup2(fd[1],1);			
			char *argv[]={"grep","-rF",argc[2],argc[3],NULL};
			execvp("/bin/grep",argv);

		}
		else                                        // In Parent process the dup2 (b/w stdin and read fd) is used and the wc -l is applied
		{
			   dup2(fd[0], 0);						//so basically the input is from the pipe when the wc -l is applied
        	   close(fd[1]);
        	   close(fd[0]);
        	   char *argv[]={"wc","-l",NULL};
        	   execv("/usr/bin/wc",argv);
		}
		return 0;
	}


	else if(strcmp(argc[1],"$")==0)
	{
		int pfd1[2];
		int pfd2[2];                                            // create 2 file descriptors

		if(pipe(pfd1)<0 || pipe(pfd2)<0)
		{
			printf("error\n");
			exit(1);
		}

		int filedesc=open(argc[4], O_WRONLY | O_TRUNC | O_CREAT, 0700); //creating/opening the file with the same name as argument #5
		int pid=fork();                                                 //use fork to carry out two piped processes
		if(pid<0)
		{
			printf("error\n");
			exit(1);
		}
		else if(pid==0)
		{
			dup2(pfd1[1],1);                                          // so that stdout is written in pipe1
			char *argv[]={"grep","-rF",argc[2],argc[3],NULL};		  //execution of grep and other parameters required my grep like
			execvp("grep",argv);									  // the pattern in argc[2] and the address in argc[3]
		}
		else
		{
			dup2(pfd2[0],0);                                    //dup2 of pipe2's reading port and stdin                                      
			char c[1];               
			close(pfd1[1]);                                      //closing writing port of pipe1 so that while reading EOF will be there
			while(read(pfd1[0],c,1))                            //reading from pipe1 and writing in pipe 2 and on the file made      
			{
				write(pfd2[1],c,1);
				write(filedesc,c,1);                        
			}
			close(pfd2[1]);                                   //as dup2 was done for pipe2's reading port and stdin so all the pipe2's data is 
			close(pfd2[0]);                                   //in stdin now close all the ports and run the remaining commands
			close(pfd1[0]);
			if(args<=4)
				printf("error\n");

			char *argv[args-4];                              // since the first four arguments are not the required ones
			for(int i=0;i<args-5;++i)
				argv[i]=argc[5+i];                           // getting an appropriate argument's array
			argv[args-4-1]=NULL;
		
			if(execvp(argv[0],argv)!=0)                       //executing the arguments on stdin
				perror("");                                 
		}

		return 0;
	}
	else
	{
		printf("please enter @ or $ as second argument\n");
		exit(1);
	}
}