test_timer: test_timer.cpp
	g++ $^ -o $@ -levent -std=gnu++11 -lpthread
	@echo "---------------------app start------------------------"
	./$@
clean:
	rm -rf test_timer
	rm -rf *.o
