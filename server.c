#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <strings.h>
#include <mqueue.h>
#include <string.h>
#include <fcntl.h>

int begin_listen (const short port);
int open_mq (mqd_t *mqd,char *pathname);
void *write_data (void *arg);
void *deal_data (void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int connfd;

int main ()
{
	int sockfd;
	int n = 0;
	short port = 1080;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	char pathname[] = "testfile";
	mqd_t mqd;
	pthread_t thread[2];

	if ( (n = open_mq (&mqd,pathname)) != 1) {
		printf ("error in open mqueue\n");
		exit (1);
	}
	
	sleep (5);

	if ( (sockfd = begin_listen (port)) == -1) {
		printf ("listen port %d failed\n",port);
		exit (1);
	}
	
	if ( (connfd = accept (sockfd,(struct sockaddr *) &cliaddr,&clilen)) == -1) {
		perror ("accept");
		exit (1);
	}
	
	if ( (pthread_create (&thread[0],NULL,write_data,(void *)&mqd)) != 0) {
		perror ("pthread_create");
		exit (1);
	}
	
	if ( (pthread_create (&thread[1],NULL,deal_data,(void *)&mqd)) != 0) {
		perror ("pthread_create");
		exit (1);
	}
	
	sleep (5);
	mq_unlink (pathname);

	return 0;
}

int begin_listen (const short port)
{
	int sockfd;
	struct sockaddr_in seraddr;
	
	bzero (&seraddr,sizeof (seraddr));

	if ( (sockfd = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP)) == -1) {
		perror ("socket");
		return -1;
	}

	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons (port);
	seraddr.sin_addr.s_addr = htonl (INADDR_ANY);

	if (bind (sockfd,(struct sockaddr *) &seraddr,sizeof (seraddr)) == -1) {
		perror ("bind");
		return -1;
	}
	
	if (listen (sockfd,10) == -1) {
		perror ("listen");
		return -1;
	}

	return sockfd;
}

int open_mq (mqd_t *mqd,char *pathname)
{
	mqd_t temp;

	if ( (temp = mq_open (pathname,O_RDWR|O_CREAT,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,NULL)) == -1) {
		perror ("mq_open");
		return -1;
	}

	*mqd = temp;

	return 1;
}

void *write_data (void *arg)
{
	int n = 0;
	int proi = 10;
	char buff[200];
	mqd_t mqd = *((mqd_t*) arg);
	pthread_t thread;
	
	thread = pthread_self ();
	pthread_detach (thread);

	while (1) {
		n = read (connfd,buff,200);
		if (n < 0) {
			perror ("read");
			return (void *) -1;
		}
		
		if (n == 0)
			break;

		buff[n - 1] = '\0';
		mq_send (mqd,buff,n,proi ++);
	}

	return (void *) 1;
}

void *deal_data (void *arg)
{
	mqd_t mqd = *((mqd_t *) arg);
	char buff[200];
	pthread_t thread;

	thread = pthread_self ();
	pthread_detach (thread);

	sleep (3);

	if ((mq_receive (mqd,buff,200,NULL)) == -1) {
		perror ("mq_receive");
		exit (1);
	}
	printf ("data:%s\n",buff);
	
	return (void *) 1;
}
