SRCS:=$(wildcard *.cpp)

FtpServer:$(SRCS)
	g++ $^ -o $@ -pthread -lmysqlclient -lcrypto

clean:
	rm -rf FtpServer
