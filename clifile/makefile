all :ser
GDB=-g

ser: ser.o socket.o work_pthread.o
	gcc -o ser ser.o socket.o work_pthread.o -lpthread -lcrypto
ser.o :ser.c
	gcc -c ser.c ${GDB}
socket.o :socket.c
	gcc -c socket.c ${GDB}
work_pthread.o :work_pthread.c
	gcc -c work_pthread.c ${GDB}

clean:
	rm -rf *.o ser

