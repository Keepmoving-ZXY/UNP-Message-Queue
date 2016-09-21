#include <stdio.h>
#include <stdlib.h>
#include "mqueue.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

int main ()
{
	char filename [] = "testfile";
	mqd_t mqd;
	mqd = mq_open (filename,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (mqd == (struct mq_info *)-1) {
		perror ("mq_open");
		exit (1);
	}


	return 0;
}
