test_file: test_file.cpp
	g++ $^ -o $@ -levent -std=c++11 -lpthread
	@echo "---------------------app start------------------------"
	./$@
clean:
	rm -rf test_file
	rm -rf *.o
