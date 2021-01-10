test_ftp_server:test_ftp_server.cpp XThread.cpp XThreadPool.cpp XFtpServerCMD.cpp XFtpFactory.cpp XFtpTask.cpp XFtpUSER.cpp XFtpLIST.cpp XFtpPORT.cpp XFtpRETR.cpp XFtpSTOR.cpp
	g++ $^ -g -o $@ -levent -lpthread -std=c++11
	#./$@
clean:
	rm -rf test_ftp_server
	rm -rf *.o
