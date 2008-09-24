# p2net make.

all: p2netd

H_p2net=p2net.h
H_settings=settings.h

OBJS=main.o

p2netd: $(OBJS)
	gcc -o p2netd $(OBJS)


main.o: main.c $(H_p2net) $(H_settings)
	gcc -c -o main.o main.c

