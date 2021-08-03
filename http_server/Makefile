FLAG=-std=c++11 -I ~/third_part/include  -lpthread

http_server:http_server.cc libsearcher.so
	g++ $^ -o $@ $(FLAG)

.PHONY:clean
clean:
	rm  http_server
