SRCS:=$(wildcard *.cpp)

FtpClient:$(SRCS)
	g++ $^ -o $@ -pthread -lcrypt -lcrypto

clean:
	rm -rf FtpClient
