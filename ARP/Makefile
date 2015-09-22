CC=gcc
#UNP_DIR=/home/raghu/np/unpv13e
UNP_DIR=/home/users/cse533/Stevens/unpv13e
#UNP_DIR=/home/subin/Desktop/np/stevens/unpv13e
INCLUDE=${UNP_DIR}/lib
CFLAGS=-I${INCLUDE} -L${UNP_DIR} -g -O2 -D_REENTRANT -Wall
LIBS=-lpthread -lunp

all: tour arp

tour: get_hw_addrs.o ip_hdr.o tour_msg.o icmp_msg.o tour.o
	${CC} ${CFLAGS} get_hw_addrs.o ip_hdr.o tour_msg.o icmp_msg.o tour.o -o ragkumar_tour ${LIBS}

arp: get_hw_addrs.o arp.o
	${CC} ${CFLAGS} get_hw_addrs.o arp.o -o ragkumar_arp ${LIBS}

tour.o: tour.c
	${CC} ${CFLAGS} -c tour.c

arp.o: arp.h arp.c
	${CC} ${CFLAGS} -c arp.c

get_hw_addrs.o: hw_addrs.h get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

ip_hdr.o: ip_hdr.h ip_hdr.c
	${CC} ${CFLAGS} -c ip_hdr.c

tour_msg.o: tour_msg.h tour_msg.c
	${CC} ${CFLAGS} -c tour_msg.c

icmp_msg.o: icmp_msg.h icmp_msg.c
	${CC} ${CFLAGS} -c icmp_msg.c

clean:
	rm -f *.o ragkumar_tour ragkumar_arp
