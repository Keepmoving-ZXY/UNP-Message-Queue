#include "mqueue.h"
#include <errno.h>
#include <sys/mman.h>

int mq_close (mqd_t mqd)
{
	long msgsize;
	long filesize;
	struct mq_hdr *mqhdr;
	struct mq_info *mqinfo;
	struct mq_attr *attr;

	mqinfo = mqd;
	if (mqinfo->mqi_magic != MQI_MAGIC) {
		errno = EBADF;
		return -1;
	}

	mqhdr = mqinfo->mqi_hdr;
	attr = &mqhdr->mqh_attr;

	if (mq_notify (mqd,NULL) != 0) {
		return -1;
	}

	msgsize = attr->mq_msgsize;
	filesize = sizeof (struct mq_hdr) + (attr->mq_maxmsg * (sizeof (struct msg_hdr) + msgsize));
	
	if (munmap (mqinfo->mqi_hdr,filesize) == -1)
		return -1;

	mqinfo->mqi_magic = 0;
	free (mqinfo);
	return 0;
}
