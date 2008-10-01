# p2net make.

all: p2netd

H_p2net=p2net.h
H_settings=settings.h
H_info=info.h
H_daemon=daemon.h
H_nodes=nodes.h
H_connections=connections.h

OBJS=main.o settings.o info.o daemon.o nodes.o connections.o
ARGS=

p2netd: $(OBJS)
	gcc -o p2netd $(OBJS)


main.o: main.c $(H_p2net) $(H_settings)
	gcc -c -o main.o main.c $(ARGS)

settings.o: settings.c $(H_p2net) $(H_settings) 
	gcc -c -o settings.o settings.c $(ARGS)

info.o: info.c $(H_info)
	gcc -c -o info.o info.c $(ARGS)

daemon.o: daemon.c $(H_daemon)
	gcc -c -o daemon.o daemon.c $(ARGS)

nodes.o: nodes.c $(H_nodes)
	gcc -c -o nodes.o nodes.c $(ARGS)

connections.o: connections.c $(H_connections)
	gcc -c -o connections.o connections.c $(ARGS)



