//Paramveer Raol 170459
//Basic process outline is that we get the size of the subdirectories using a recursive function and store these sizes in a pipe and then 
//using this data we get the size of root directory
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include<string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h> 


struct node{
	struct node *next;
};


struct node * create();
struct node * push(struct node * head);
struct node * pop(struct node * head);
//stack is just used to keep track of how deep the function is in the directory if stack is null than after completion of process
//instead of going to the previous directory the size is returned
long long int size_dir(struct node * head);
//Main recursion finction to get the size of files in the given directory
char * int_str(long long int a); // converts and integer to string
int size_of_int(long long int a); // gives the number of digits in the integer 
long long int str_int(char * str);  //converts the given string into an integer
char * print_root(char *str);  //Basically prints only the root directory's name and nothing else given

int main(int args,char *argv[])
{
	if(chdir(argv[1])!=0)            //changing current position to the root directory
	{
		perror("line 26");
		exit(1);
	}
	char newl='\n';                            

	DIR *dp;
	dp=opendir("./");              //geting all the sub-directories and files in the root directory 
	struct node *head=NULL;        //stack is just for counting purposes
	struct dirent *ep;             

	int pfd[2];
	pipe(pfd);                   //creating pipe where the sizes of sub-directories of root and its files will be stored

	while(ep=readdir(dp))         //getting the directory / file name one by one
	{
		int pid=fork();            // creating a fork where the child gets the size of sub directory / file and stores in file 
		if(pid<0)				  // the parent process is immediately terminated so the it does not intergeres while giving input in the 
		{                         //pipe and the final reading of the pipe is done only once
			perror("line 37");
			exit(1);                //every child gets a new directory or file and forks again but every parent dies
		}
		else if(pid==0)             //process inside the child        
		{
			if(strcmp(ep->d_name,"..")==0 || strcmp(ep->d_name,".")==0)   //making sure same directory or its parent is not accessed
				continue;                                
			long long int pipe_var;                                   // the varible that stores the size of the sub-file or the sub-directory
			if(chdir(ep->d_name)==0){                                 // of the root directory
		
				head=push(head);
				pipe_var=size_dir(head);
				printf("%s,%lld\n",ep->d_name,pipe_var);              //given case is when a directory is there then the size is stored the the
																	 //variable pipe_var and the directory name and the size is outputed					
				if(head){
					head=pop(head);                             
					chdir("../");
				}
			}
			else
			{
				head=push(head);
				struct stat size;
				if(stat(ep->d_name,&size)!=0){
						perror("line 79");
				}
				pipe_var=size.st_size;     	                    //if the sub one is a file the simpli get the file's size and store it in 
				head=pop(head);                               // variable pip_var
			}
			char *pipe_str=int_str(pipe_var);                    //converting pipe_var to a string pipe_str
			if(write(pfd[1],pipe_str,strlen(pipe_str))==-1)      // writing the string in the pipe
				perror("line 86");
			if(write(pfd[1],&newl,1)==-1)                       // just adding a new line character for differentiating various sizes
				perror("line 88");
		}
		else
		{
			close(pfd[1]);                                     //closing write port of pipe
			wait(NULL);
			return 0;
		}
	}

	
	int i=0;                                                      //from here reading of the pipe data begins
	char c; 
	char arr[50];                                                //just stores the string cointaining the size 
	close(pfd[1]);                                                //closing the write port of pipe so that reading gets an EOF
	long long int final=0;

	while(read(pfd[0],&c,1)>0)
	{
		if(c=='\n')
		{
			arr[i]='\0';                                      //on identifyting the end of the string of size convert the string 
			i=0;                                              //to a number and add it to the "final" size storing variable
			final=final+str_int(arr);
		}
		else
		{
			arr[i]=c;
			++i;
		}
	}
	printf("%s,%lld\n",print_root(argv[1]),final);                  // printing the final size and only the root directory's name

	
	close(pfd[0]);

	return 0;
}

//Basic stack functions
struct node * create()
{
	struct node *dummy=(struct node*)malloc(sizeof(struct node));
	dummy->next=NULL;
	return dummy;
}
struct node * push(struct node * head)
{
	if (head==NULL)
		head=create();
	else
	{
		struct node * dummy=create();
		dummy->next=head;
		head=dummy;
	}
	return head;
}
struct node * pop(struct node * head)
{
	if(head==NULL)
		return NULL;
	else
	{
		head=head->next;
		return head;
	}
}


//a recursive fucntion similar to that one in the first question just instead of substring matiching 
//size of the 
long long int size_dir(struct node * head)
{
	
	DIR *dp;
	dp=opendir("./");

	struct dirent *ep;
	long long int ans=0;
	
	if(chdir("./")==0)
	{
		if(dp!=NULL)
		{
			while(ep=readdir(dp))
			{
				if(strcmp(ep->d_name,"..")==0 || strcmp(ep->d_name,".")==0)
					continue;
				if(chdir(ep->d_name)==0)
				{
					head=push(head);
					ans=ans+size_dir(head);
					head=pop(head);
				}
				else
				{
					head=push(head);
					struct stat size;

					if(stat(ep->d_name,&size)==0){
						ans=ans+size.st_size;
					}

					head=pop(head);
				}
			}
			head=pop(head);
			if(head)
				chdir("../");
			
		}
	}
	else
		perror("line 101");

	return ans;
}


int size_of_int(long long int a)
{
	if(a==0)
		return 1;

	int count=0;
	while(a!=0)
	{
		++count;
		a=a/10;
	}
	return count;
}

char * int_str(long long int a)
{
	int size=size_of_int(a)+1;
	char * dummy=(char *)malloc(sizeof(char)*size);
	dummy[size-1]='\0';
	for(int i=0;i<size-1;++i)
	{
		int r=a%10;
	dummy[size-2-i]=(char)(r+(int)'0');
			a=a/10;		
	}
	return dummy;
}


long long int str_int(char * str)
{
	int a=strlen(str);
	int no=0;
	for(int i=0;i<a;++i)
		no=no*10+(int)(str[i])-(int)'0';
	return no;
}

char * print_root(char *str)
{
	long int a=strlen(str);
	int first;
	if(str[a-1]=='/')
	{
		for(int i=a-2;i>=0;--i)
		{
			if(str[i]=='/')
			{
				first=i;
				break;
			}
		}
		char * dummy=(char *)malloc(sizeof(char)*(a-2-first));
		for(int i=first+1;i<=a-2;++i)
			dummy[i-first-1]=str[i];
		return dummy;
	}
	else
	{
		for(int i=a-1;i>=0;--i)
		{
			if(str[i]=='/')
			{
				first=i;
				break;
			}
		}
		char * dummy=(char *)malloc(sizeof(char)*(a-1-first));
		for(int i=first+1;i<=a-1;++i)
			dummy[i-first-1]=str[i];
		return dummy;
	}
}