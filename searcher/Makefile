FLAG=-std=c++11 -I ~/third_part/include  -ljsoncpp

.PHONY:all
all:index_test searcher_test libsearcher.so

index_test:searcher.cc index_test.cc
	g++ $^ -o $@ $(FLAG)

searcher_test:searcher_test.cc searcher.cc
	g++ $^ -o $@ $(FLAG) 

libsearcher.so:searcher.cc searcher.h
	g++ searcher.cc -o libsearcher.so -shared -fPIC $(FLAG)

.PHONY:clean
clean:
	rm index_test searcher_test libsearcher.so
