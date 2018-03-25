serv: serv1.c keyvalue.h serv2.c serv3.c serv4.c client.c
	gcc serv1.c keyvalue.h -o serv1 -lpthread -lrt
	gcc serv2.c keyvalue.h -o serv2 -lpthread -lrt
	gcc serv3.c keyvalue.h -o serv3 -lpthread -lrt
	gcc serv4.c keyvalue.h -o serv4 -lpthread -lrt
	gcc client.c -o client

