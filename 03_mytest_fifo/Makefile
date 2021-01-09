target:libevent_rd_fifo.c  wr_fifo.c
	gcc libevent_rd_fifo.c -o rd -levent
	gcc wr_fifo.c -o wr -levent

.PHONY:clean
clean:
	-rm *.o wr rd fifo.tmp -f