#include"pthread.h"
#include<stdio.h>
#define ARGC_MAX 10
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>
#include<sys/stat.h>
#define ERR "err"
#include<openssl/md5.h>
#define MD5_LEN   16
#define MD5_SIZE  1024*16
#define MD5_OK  "md5 is right"
struct ArgNode
{
	int c;
};
void send_err(int c)
{
	send(c,ERR,strlen(ERR),0);
}
char *get_cmd(char buff[],char* myargv[])
{
	if(buff==NULL||myargv==NULL){
		return NULL;
	}
	char *ptr=NULL;
	int i=0;
	char *s=strtok_r(buff," ",&ptr);
	while(s!=NULL)
	{
		myargv[i++]=s;
		s=strtok_r(NULL," ",&ptr);
	}
	return myargv[0];
}
void print_md5(unsigned char md[],char buff[])
{
	int i=0;
	char tmp[128]={0};
	for(;i<MD5_LEN;i++)
	{
		sprintf(tmp,"%02x",md[i]);
		strncat(buff,tmp,strlen(tmp));
	}

}
void fun_md5(int fd,char buff[])
{
	MD5_CTX ctx;
	unsigned char md[MD5_LEN]={0};
	MD5_Init(&ctx);

	unsigned long len=0;
	char tmp_buff[128]={0};
	while((len=read(fd,tmp_buff,MD5_SIZE))>0)
	{
		MD5_Update(&ctx,tmp_buff,MD5_SIZE);
	}
	MD5_Final(md,&ctx);
	print_md5(md,buff);
}
void send_file(int c,char*filename)
{
	if(filename==NULL)
	{
		send_err(c);
		return;
	}
	int fd=open(filename,O_RDONLY);
	if(fd==-1)
	{
		send_err(c);
		return;
	}
	int filesize=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	
	char buff_status[128]={0};
	sprintf(buff_status,"ok#%d ",filesize);//ok#size#md5

	send(c,buff_status,strlen(buff_status),0);
	if(filesize==0)
	{
		close(fd);
		return;
	}
	memset(buff_status,0,128);
	int num=recv(c,buff_status,127,0);
	if(num<=0)
	{
		close(fd);
		return;
	}
	if(strncmp(buff_status,"ok",2)!=0)
	{
		close(fd);
		return;	
	}	
	while((num=read(fd,buff_status,1024))>0)
	{
		send(c,buff_status,num,0);
	}
	memset(buff_status,0,128);
	num=recv(c,buff_status,127,0);
	if(strncmp(buff_status,"ok",2)!=0)
	{
		return;
	}
	lseek(fd,0,SEEK_SET);
	char md_buff[64]={0};
	fun_md5(fd,md_buff);
	send(c,md_buff,strlen(md_buff),0);
	
	close(fd);
	//计算md5
	//send(md5);
}
void recv_file(int c,char *filename,char *cmd_str)
{
	if(filename==NULL||cmd_str==NULL)
	{
		return;
	}
	send(c,cmd_str,strlen(cmd_str),0);

	char buff[128]={0};
	int num=recv(c,buff,127,0);
	if(num<=0)
	{
		printf("client close\n");
		return;
	}
	int filesize=atoi(buff+3);
	printf("filesize:%d\n",filesize);

	int fd=open(filename,O_CREAT|O_WRONLY,0600);
	if(fd==-1)
	{
		send(c,"err",3,0);
		return;
	}
	if(filesize==0)
	{
		close(fd);
		return;
	}
	send(c,"ok",2,0);

	char data[512]={0};
	int curr_size=0;
	int n=0;

	while((n=recv(c,data,512,0))>=0)
	{
		write(fd,data,n);
		curr_size+=n;
		float f=curr_size*100.0/filesize;
		printf("percent : %.2f%%",f);
		fflush(stdout);
		if(curr_size>=filesize)
		{
			break;
		}
	}
	printf(" recv down ok\n");
	close(c);

}
void *work_fun(void *arg)
{
	struct ArgNode *p=(struct ArgNode*)arg;
	if(p==NULL)	return NULL;
	int c=p->c;
	free(p);	
	while(1)
	{
		char buff[128]={0};
		int num=recv(c,buff,127,0);
		if(num<=0)
		{
			break;
		}
		char* myargv[ARGC_MAX]={0};
		char* cmd=get_cmd(buff,myargv);
		if(cmd==NULL){
			send_err(c);
			continue;
		}
		else if(strcmp(cmd,"get")==0)
		{
			send_file(c,myargv[1]);
		}
		else if(strcmp(cmd,"up")==0)
		{
			recv_file(c,myargv[1],cmd);
		}
		else
		{
			//fork+exec
			int pipefd[2];
			if(pipe(pipefd)==-1)
			{
				send_err(c);
				continue;
			}
			pid_t pid=fork();
			if(pid==-1)
			{
				send_err(c);
				continue;
			}
			if(pid==0)
			{
				close(pipefd[0]);//close read
				dup2(pipefd[1],1);
				dup2(pipefd[2],2);
				execvp(cmd,myargv);
				perror("exec err\n");

				exit(0);
			}
			close(pipefd[1]);//close write
			wait(NULL);
			char send_buff[1024]={"ok#"};
			read(pipefd[0],send_buff+3,1020);
			send(c,send_buff,strlen(send_buff),0);
		}
	
	}
	close(c);
	printf("client close\n");
}

void start_pthread(int c)
{
//	if(pipe(pipefd)==-1)
//	{
//		printf("err\n");
//	}
	
	pthread_t id;
	struct ArgNode *p=(struct ArgNode*)malloc(sizeof(struct ArgNode));
	if(p==NULL)
	{
		return ;
	}
	p->c=c;
	if(pthread_create(&id,NULL,work_fun,(void*)p)!=0)
	{
		close(c);
		printf("open pthread err\n");
		return;
	}
//	close(pipefd[1]);
}
