target:bufferevent_server.c bufferevent_client.c
	gcc bufferevent_server.c -o server -levent
	gcc bufferevent_client.c -o client -levent

.PHONY:clean
clean:
	-rm *.o server client -f