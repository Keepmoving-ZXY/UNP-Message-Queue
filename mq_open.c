#include "mqueue.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#define va_mode_t mode_t
#define MAX_TRIES 10

struct mq_attr defattr = {0,128,1024,0};

mqd_t mq_open (const char *pathname,int oflag,...)
{
	int i;
	int fd;
	int nonblock;
	int created;
	int save_errno;
	long msgsize;
	long filesize;
	long index;
	va_list ap;
	mode_t mode;
	int8_t *mptr;
	struct stat statbuff;
	struct mq_hdr *mqhdr;
	struct msg_hdr *msghdr;
	struct mq_attr *attr;
	struct mq_info *mqinfo;
	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;
	
	created = 0;
	nonblock = oflag & O_NONBLOCK;
	oflag &= ~O_NONBLOCK;

	mptr = (int8_t *) MAP_FAILED;
	mqinfo = NULL;

again:
	if (oflag & O_CREAT) {
		va_start (ap,oflag);
		mode = va_arg (ap,va_mode_t) & ~S_IXUSR;
		attr = va_arg (ap,struct mq_attr *);
		va_end (ap);

		fd = open (pathname,oflag | O_EXCL | O_RDWR,mode | S_IXUSR);
		if (fd < 0) {
			if (errno == EEXIST && (oflag & O_EXCL) == 0)
				goto exist;
			else 
				return (mqd_t ) -1;
		}

		created = 1;

		if (attr == NULL)
			attr = &defattr;
		else {
			if (attr->mq_maxmsg <= 0 || attr->mq_msgsize <= 0) {
				errno = EINVAL;
				goto err;
			}
		}

		msgsize = MSGSIZE (attr->mq_msgsize);
		filesize = sizeof (struct mq_hdr) + (attr->mq_maxmsg * (sizeof (struct msg_hdr) + msgsize));
		
		if (lseek (fd,filesize - 1,SEEK_SET) == -1)
			goto err;
		if (write (fd,"",1) == -1)
			goto err;

		mptr = mmap (NULL,filesize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		if (mptr == MAP_FAILED) 
			goto err;
		
		if ((mqinfo = (struct mq_info *) malloc (sizeof (struct mq_info))) == NULL)
			goto err;
		
		mqinfo->mqi_hdr = mqhdr = (struct mq_hdr *) mptr;
		mqinfo->mqi_magic = MQI_MAGIC;
		mqinfo->mqi_flags = O_NONBLOCK;

		mqhdr->mqh_attr.mq_flags = 0;
		mqhdr->mqh_attr.mq_maxmsg = attr->mq_maxmsg;
		mqhdr->mqh_attr.mq_msgsize = attr->mq_msgsize;
		mqhdr->mqh_attr.mq_curmsgs = 0;
		mqhdr->mqh_nwait = 0;
		mqhdr->mqh_pid = 0;
		mqhdr->mqh_head = 0;
		
		index = sizeof (struct mq_hdr);
		mqhdr->mqh_free = index;
		
		for (i = 0;i < attr->mq_maxmsg - 1;i ++) {
			msghdr = (struct msg_hdr *) &mptr[index];
			index += sizeof (struct msg_hdr) + msgsize;
			msghdr->msg_next = index;
		}

		msghdr = (struct msg_hdr *) &mptr[index];
		msghdr->msg_next = 0;

		if ((i = pthread_mutexattr_init (&mattr)) != 0)
			goto pthreaderr;
		pthread_mutexattr_setpshared (&mattr,PTHREAD_PROCESS_SHARED);
		i = pthread_mutex_init (&mqhdr->mqh_lock,&mattr);
		pthread_mutexattr_destroy (&mattr);
		if (i != 0)
			goto pthreaderr;

		if ((i = pthread_condattr_init (&cattr)) != 0)
			goto pthreaderr;
		pthread_condattr_setpshared (&cattr,PTHREAD_PROCESS_SHARED);
		i = pthread_cond_init (&mqhdr->mqh_wait,&cattr);
		pthread_condattr_destroy (&cattr);
		if (i != 0)
			goto pthreaderr;

		if (fchmod (fd,mode) == -1)
			goto err;
		close (fd);
		return (mqd_t) mqinfo;
	}	

exist:
	if ((fd = open (pathname,O_RDWR)) < 0) {
		if (errno == ENOENT && (oflag & O_CREAT))
			goto again;
		goto err;
	}

	for (i = 0;i < MAX_TRIES; i++) {
		if (stat (pathname,&statbuff) == -1) {
			
			if (errno == ENOENT && (oflag & O_CREAT))
			{
				close (fd);
				goto again;
			}

			goto err;
		}

		if ((statbuff.st_mode & S_IXUSR) == 0)
			break;

		sleep (1);
	}

	if (i == MAX_TRIES) {
		errno = ETIMEDOUT;
		goto err;
	}

	filesize = statbuff.st_size;
	mptr = mmap (NULL,filesize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if (mptr == MAP_FAILED) {
		goto err;
	}

	close (fd);

	if ((mqinfo = (struct mq_info *) malloc (sizeof (struct mq_info))) == NULL) 
		goto err;

	mqinfo->mqi_hdr = (struct mq_hdr *) mptr;
	mqinfo->mqi_magic = MQI_MAGIC;
	mqinfo->mqi_flags = nonblock;

	return ((mqd_t )(mqinfo));

pthreaderr:
	errno = i;

err:
	save_errno = errno;
	if (created) 
		unlink (pathname);
	if (mptr != MAP_FAILED)
		munmap (mptr,filesize);
	if (mqinfo != NULL)
		free (mqinfo);

	close (fd);
	errno = save_errno;
	return ((mqd_t) -1);
}
