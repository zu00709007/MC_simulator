simulator: Request.o LRU_Cache.o TTL_Cache.o Huristic.o
	g++ main.cpp Request.o LRU_Cache.o TTL_Cache.o Huristic.o -o simulator

Request.o: Request.cpp Request.h
	g++ Request.cpp Request.h -c

LRU_Cache.o: LRU_Cache.cpp LRU_Cache.h
	g++ LRU_Cache.cpp LRU_Cache.h -c
	
TTL_Cache.o: TTL_Cache.cpp TTL_Cache.h
	g++ TTL_Cache.cpp TTL_Cache.h -c

Huristic.o: Huristic.cpp Huristic.h
	g++ Huristic.cpp Huristic.h -c

c:
	rm -rf *.o *.gch *~ simulator
