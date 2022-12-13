#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<fcntl.h>
#include<openssl/md5.h>

char *get_cmd(char *get_cmd,char *myargv[])
{
	if(get_cmd==NULL||myargv==NULL)
	{
		return NULL;
	}
	int i=0;
	char *ptr=NULL;
	char *s=strtok_r(get_cmd," ",&ptr);
	while(s!=NULL)
	{
		myargv[i++]=s;
		s=strtok_r(NULL," ",&ptr);
	}
	return myargv[0];
}
int connect_ser();
void print_md5(unsigned char md[],char buff[])
{
	int i=0;
	char tmp[128]={0};
	for(;i<strlen(md);i++)
	{
		sprintf(tmp,"%02x",md[i]);
		strncat(buff,tmp,strlen(tmp));
	}

}
void fun_md5(int fd,char buff[])
{
	MD5_CTX  ctx;
	unsigned char md[64]={0};
	MD5_Init(&ctx);

	unsigned long len=0;
	char md_buff[64]={0};
	while((len=read(fd,md_buff,64))>0)
	{
		MD5_Update(&ctx,md_buff,64);
	}
	MD5_Final(md,&ctx);
	print_md5(md,buff);
}
void send_file(int c,char* filename)
{
	if(filename==NULL)
	{
		send(c,"filename err\n",15,0);
		return;
	}
	int fd=open(filename,O_RDONLY);
	if(fd==-1)
	{
		send(c,"file not open\n",20,0);
	}
	int filesize=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	if(filesize==0)
	{
		close(c);
		return;
	}
	char buff_status[128]={0};
	sprintf(buff_status,"ok#%d",filesize);

	send(c,buff_status,strlen(buff_status),0);
	memset(buff_status,0,128);

	int num=recv(c,buff_status,127,0);
	if(num<=0||strncmp(buff_status,"ok",2)!=0)
	{
		close(fd);
		return;
	}
	if(num>2)
	{
		char buff[1024]={0};
		while((num=read(fd,buff,strlen(buff)))>0)
		{
			send(c,buff,num,0);
		}
	}
	close(fd);	
}
void recv_file(int c,char* filename,char* cmd_str)
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
		printf("ser close\n");
		return;
	}

	if(strncmp(buff,"ok#",3)!=0)
	{
		printf("not find file\n ");
		return;
	}
	int filesize=atoi(buff+3);
	printf("filesize:%d\n",filesize);
	
	int fd=open(filename,O_CREAT|O_RDWR,0600);
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
	
	char data[1024]={0};
	int curr_size=0;
	int n=0;
	while((n=recv(c,data,1024,0))>=0)
	{
		write(fd,data,n);
		curr_size+=n;
		float f=curr_size*100.0/ filesize;
		printf("percent :%.2f%%\r",f);
		fflush(stdout);
		if(curr_size>=filesize)
		{
			break;
		}
		
	}
	printf("\ndownload ok\n");
	send(c,"ok",2,0);
	lseek(fd,0,SEEK_SET);
	char md_buff[64]={0};
	fun_md5(fd,md_buff);
	printf("%s\n",md_buff);
	memset(data,0,1024);
	n=recv(c,data,1023,0);
	printf("%s\n",data);
	if(strncmp(data,md_buff,strlen(data))==0)
	{
		printf("md5 is right\n");
	}
	close(fd);
}
int main()
{
	int c=connect_ser();
	if(c==-1)
	{
		exit(0);
	}
	while(1)
	{
		printf("connect->>");
		fflush(stdout);

		char buff[128]={0};
		fgets(buff,128,stdin);//"get a.c"

		buff[strlen(buff)-1]=0;//"ls" "rm a.c"

		char cmd_buff[128]={0};
		strcpy(cmd_buff,buff);

		char *myargv[10]={0};
		char *cmd=get_cmd(buff,myargv);
		if(cmd==NULL)
		{
			continue;
		}
		if(strcmp(cmd, "end")==0)
		{

			break;
		}
		
		else if(strcmp(buff,"get")==0)
		{
			recv_file(c,myargv[1],cmd_buff);
		}
		else if(strcmp(buff,"up")==0)
		{}
		else{
			send(c,cmd_buff,strlen(cmd_buff),0);
			char recv_buff[1024]={0};
			if(recv(c,recv_buff,1023,0)<=0)
			{
				printf("ser close\n");
			}
			if(strncmp(recv_buff,"ok#",3)==0)
			{
				printf("%s\n",recv_buff+3);
			}
			else
			{
				printf("err\n");
			}
		}
		//send(c,buff,strlen(buff),0);
		//memset(buff,127,0);
		//recv(c,buff,127,0);
		//printf("buff=%s\n",buff);
	}
	close(c);
	exit(0);
}
int connect_ser()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)	{
		printf("connect err\n");	
		return -1;
	}
	
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(6000);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");

	int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(res==-1){
		printf("connect ser err\n");
		return -1;
	}
	return sockfd;
}

