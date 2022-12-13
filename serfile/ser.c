#include"socket.h"
#include"work_pthread.h"

int main()
{

	int sockfd=socket_init();
	if(sockfd==-1)
	{
		exit(0);
	}

	struct sockaddr_in caddr;
	int len=sizeof(caddr);

	while(1)
	{
		int c=accept(sockfd,(struct sockaddr*)&caddr,&len);
		if(c<0)
		{
			continue;
		}
		printf("accpet c=%d\n",c);
		start_pthread(c);
	}
	

}


