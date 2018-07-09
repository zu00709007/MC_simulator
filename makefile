simulator: Request.o LRU_Cache.o TTL_Cache.o
	g++ main.cpp Request.o LRU_Cache.o TTL_Cache.o -o simulator

Request.o: Request.cpp Request.h
	g++ Request.cpp Request.h -c

LRU_Cache.o: LRU_Cache.cpp LRU_Cache.h
	g++ LRU_Cache.cpp LRU_Cache.h -c
	
TTL_Cache.o: TTL_Cache.cpp TTL_Cache.h
	g++ TTL_Cache.cpp TTL_Cache.h -c

c:
	rm -rf *.o *.gch *~ simulator
