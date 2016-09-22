#include "mqueue.h"
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <memory.h>

ssize_t mq_receive (mqd_t mqd,char *ptr,size_t maxlen,unsigned int *priop)
{
	int n;
	long index ;
	int8_t *mptr;
	ssize_t len;
	struct mq_hdr *mqhdr;
	struct mq_attr *attr;
	struct msg_hdr *msghdr;
	struct mq_info *mqinfo;

	mqinfo = mqd;
	if (mqinfo->mqi_magic != MQI_MAGIC) {
		errno = EBADF;
		return -1;
	}

	mqhdr = mqinfo->mqi_hdr;
	mptr = (int8_t *) mqhdr;
	attr = &mqhdr->mqh_attr;

	if ((n = pthread_mutex_lock (&mqhdr->mqh_lock)) != 0) {
		errno = n;
		return -1;
	}

	if (maxlen < attr->mq_maxmsg) {
		errno = EMSGSIZE;
		goto err;
	}

	if (attr->mq_curmsgs == 0) {
		if (mqinfo->mqi_flags & O_NONBLOCK) {
			errno = EAGAIN;
			goto err;
		} 

		mqhdr->mqh_nwait ++;
		while (attr->mq_curmsgs == 0) {
			pthread_cond_wait (&mqhdr->mqh_wait,&mqhdr->mqh_lock);
		}

		mqhdr->mqh_nwait --;
	}

	if ((index = mqhdr->mqh_head) == 0) {
		printf ("mq_open:wrong state\n");
		return -1;
	}

	msghdr = (struct msg_hdr *) &mptr[index];
	mqhdr->mqh_head = msghdr->msg_next;

	len = msghdr->msg_len;
	memcpy (ptr,msghdr + 1,len);

	if (priop != NULL)
		*priop = msghdr->msg_prio;

	msghdr->msg_next = mqhdr->mqh_free;
	mqhdr->mqh_free = index;

	if (attr->mq_curmsgs == attr->mq_maxmsg)
		pthread_cond_signal (&mqhdr->mqh_wait);

	attr->mq_curmsgs --;
	
	pthread_mutex_unlock (&mqhdr->mqh_lock);
	return len;

err:
	pthread_mutex_unlock (&mqhdr->mqh_lock);
	return -1;
	
}
