#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>

int connect_server (const char *seraddr,const short port);
int send_data (int sockfd,char *message);

int main ()
{
	short port;
	char seraddr[20];
	char sendstr1[] = "Linux debian 3.16.0-4-amd64 #1 SMP Debian 3.16.36-1+deb8u1 (2016-09-03) x86_64 GNU/Linux";
	int fd = 0;
	
	port = 1080;
	strncpy (seraddr,"127.0.0.1",sizeof (seraddr));
	
	if ( (fd = connect_server (seraddr,port)) == -1) {
		printf ("connect to server failed\n");
		exit (1);
	} else {
		send_data (fd,sendstr1);
	}

	return 0;
}

int connect_server (const char *ipstr,const short port)
{
	struct sockaddr_in seraddr;
	int sockfd;

	if (ipstr == NULL) {
		printf ("invalid server address");
		return -1;
	}

	if (port == 0) {
		printf ("invalid port\n");
		return -1;
	}
	
	bzero (&seraddr,sizeof (seraddr));
	
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons (port);
	if ( (inet_pton (AF_INET,ipstr,&seraddr.sin_addr.s_addr)) == -1) {
		printf ("invalid ip address\n");
		return -1;
	}

	if ( (sockfd = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1) {
		perror ("socket");
		return -1;
	}

	if ( (connect (sockfd,(const struct sockaddr *) &seraddr,sizeof (seraddr))) == -1) {
		printf ("connect failed,please mail to server supporter\n");
		return -1;
	}

	return sockfd;
}
	
int send_data (int sockfd,char *message)
{
	size_t len;
	int nwrite;

	if (message == NULL) {
		printf ("invalid data,send nothing\n");
		return -1;
	}
	
	len = strlen (message);
	if (len == 0) {
		printf ("invalid data,send nothing\n");
		return -1;
	}

	nwrite = write (sockfd,message,strlen (message));
	if (nwrite < 0) {
		perror ("write");
		return -1;
	}
		
	return nwrite;
}

