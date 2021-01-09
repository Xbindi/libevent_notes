echo_server: echo_server.cpp
	g++ $^ -o $@ -levent -std=c++11 -lpthread
	@echo "---------------------app start------------------------"
	./$@
clean:
	rm -rf echo_server
	rm -rf *.o
