app:test.o mq_open.o mq_close.o mq_getattr.o mq_setattr.o mq_unlink.o mq_send.o mq_receive.o mq_notify.o
	gcc -g test.o mq_open.o mq_close.o mq_getattr.o mq_setattr.o mq_unlink.o mq_send.o mq_receive.o mq_notify.o -o app -Wall -lpthread

test.o:test.c
	gcc -c -g test.c
mq_open.o:mq_open.c
	gcc -c -g mq_open.c 

mq_close.o:mq_close.c
	gcc -c -g mq_close.c

mq_getattr.o:mq_getattr.c
	gcc -c -g mq_getattr.c

mq_setattr.o:mq_setattr.c
	gcc -c -g mq_setattr.c

mq_unlink.o:mq_unlink.c
	gcc -c -g mq_unlink.c

mq_notify.o:mq_notify.c
	gcc -c -g mq_notify.c

mq_send.o:mq_send.c
	gcc -c -g mq_send.c

mq_receive.o:mq_receive.c
	gcc -c -g mq_receive.c

clean:
	rm -rf *.o
	rm app 
