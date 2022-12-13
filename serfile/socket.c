#include"socket.h"
#define IPS "ipstr"
#define PORT "port"
#define LISMAX "lismax"


struct socket_info
{
	char ips[32];//"192.168.1.1"
	short port;
	short lismax;
};
int read_conf(struct socket_info* dt)
{
	if(dt==NULL){
		return -1;
	}
	FILE* fp=fopen("my.conf","r");//only read
	if(fp==NULL){
		printf("open my.conf err/n");
		return -1;
	}

	int index=0;
	char buff[128] = {0};
	while(fgets(buff,128,fp)!=NULL){//"/n"
		++index;
//		buff[strlen(buff)-1]=0;
		if(strncmp(buff,"#",1)==0||strcmp(buff,"\n")==0){
			continue;
		}
		buff[strlen(buff)-1]=0;
		if(strncmp(buff,IPS,strlen(IPS))==0){
			strcpy(dt->ips,buff+strlen(IPS)+1);		
		}
		else if(strncmp(buff,PORT,strlen(PORT))==0){
			dt->port=atoi(buff+strlen(PORT)+1);
		}	
		else if	(strncmp(buff,LISMAX,strlen(LISMAX))==0){
			dt->lismax=atoi(buff+strlen(LISMAX)+1);
		}
		else{
			printf(" %d rows-err--%s\n",index,buff);
		}
	}

}
int socket_init()
{
	struct socket_info sock;
	if(read_conf(&sock)==-1){
		return -1;
	}
	printf("ip=%s\n",sock.ips);
	printf("port=%d\n",sock.port);
	printf("lismax=%d\n",sock.lismax);

	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		return -1;
	}

	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(sock.port);
	saddr.sin_addr.s_addr=inet_addr(sock.ips);


	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(res==-1)
	{
		printf("bind err\n");
		return -1;
	}
	if(listen(sockfd,sock.lismax)==-1)
	{
		printf("listen err\n");
		return -1;
	}
	return sockfd;
}
