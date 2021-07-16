//170459 Paramveer Raol
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

struct node
{
	char s[5000];
	struct node* next;
};//for storing the path basically id the stack is null then one must not go deeper and on comming out from the directory the last directory is poped;

 
//functions for stack created for storing path
struct node * create(char *s);
struct node * push(struct node * head,char *s);
struct node * pop(struct node * head);
char * top(struct node * head);
//function for printing stack just in appropriate strcuture
void print_stack(struct node* head);
//function for recursion .ie for printing the directories mor importantly navigating in directories
void mytree(struct node * head,char *s);
//printing the strings that have tha substrings
void file_substr_lines(char *file,char *substr,struct node *head);



int main (int args,char *argc[])
{
	
	if(chdir(argc[2])==0){
		struct node * head=NULL;
		head=push(head,argc[2]);
		mytree(head,argc[1]);
	}
	else  // Basically this case is when file is given directly then chdir won't give output 0 
	{
		struct node * head=NULL;
		file_substr_lines(argc[2],argc[1],head);
	}
  return 0;
}







// creating 
struct node * create(char *s)
{
	struct node* dummy=(struct node*)malloc(sizeof(struct node));
	strcpy(dummy->s,s);
	dummy->next=NULL;
	return dummy;	
}
//pushing
struct node * push(struct node * head,char *s)
{
	struct node * dummy= create(s);
	if(head==NULL)
		head=dummy;
	else
	{
		dummy->next=head;
		head=dummy;
	}
	return head;
}
//popin make sure that you don't have a NULL value given
struct node * pop(struct node * head)
{
	if(head==NULL)
		return head;
	else
	{
		head=head->next;
		return head;
	}
} 
//top data reading make sure you don't give a null head
char * top(struct node * head)
{
	if(head!=NULL)
		return head->s;
}

//printing the stack
void print_stack(struct node* head)
{
	if(head==NULL)  // used when only the file's path is given in to the main program
		return;
	struct node * dummy=NULL;
	struct node * idummy=head;
	while(idummy)                               // first reverse the stack and then start printing
	{
		dummy=push(dummy,idummy->s);
		idummy=idummy->next;
	}
	while(dummy->next)
	{
		if(dummy->s[strlen(dummy->s)-1]!='/'){             // just for some edge cases where / is already present in the address provided
			printf("%s/",dummy->s);
			dummy=dummy->next;
		}
		else
		{
			printf("%s",dummy->s);
			dummy=dummy->next;	
		}
	}
	printf("%s:",dummy->s);
	return;
}

// main recursion funtion yuha
void mytree(struct node * head,char * substr)
{
	DIR *dp;
	dp=opendir("./");
	struct dirent *ep;
	if(chdir("./")==0)
	{
		if(dp!=NULL){
			while(ep=readdir(dp))
			{
				if(strcmp(ep->d_name,"..")==0 || strcmp(ep->d_name,".")==0)
					continue;
				
				if(chdir(ep->d_name)==0)             // if directory then go deeper
				{
					head=push(head,ep->d_name);
					mytree(head,substr);
					head=pop(head);
				}
				else                                // if file then search it and print appropriately
				{
					head=push(head,ep->d_name);
										
						file_substr_lines(ep->d_name,substr,head);
						
					head=pop(head);
				}
			}
			head=pop(head);
			if(head)
				chdir("../");	
		}
		else
		{
			head=pop(head);
			if(head)
				chdir("../");//empty directory
		}
	}
	else // the cases where chdir is not cover are checked prior to passing as they will be files
	{
		printf("this will never occur just chill");
	}
	return;
}

//printing the strings
void file_substr_lines(char *file,char *substr,struct node *head)
{
	//the caracters are stored in arr till '\n' or EOF is found then '\0' is appended to the arr last element's end
	int filedesc = open(file, O_RDONLY);
	char arr[1000000];
	char c;
	int i=0;
	while(read(filedesc,&c,1)==1)
	{
		if(i==100000)
			printf("string size is too long\n");
		else if(c=='\n')
		{//if we find new line character then the string till then is checked for the substring and printed if the substring is found
			char *dummy=NULL;
			arr[i]='\0';
			dummy=strstr(arr,substr);
			if(dummy){
				print_stack(head);
				printf("%s\n",arr);
			}
			i=0;
			//substr found or not again arr is started with new string
		}
		else
		{
			arr[i]=c;
			++i;
		}	
	}
	char *dummy=NULL;
	arr[i]='\0';
	dummy=strstr(arr,substr); //just the last case where EOF would have been encountered
	if(dummy){
		print_stack(head);
		printf("%s\n",arr);
	}

	if(close(filedesc) < 0)
    	printf("error in closing file\n");

	i=0;
	return;
}
