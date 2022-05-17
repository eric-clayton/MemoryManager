all: MemoryManager.o libMemoryManager.a
MemoryManager.o:
	g++ -g -c MemoryManager.cpp
libMemoryManager.a:
	ar cr libMemoryManager.a MemoryManager.o
clean:
	rm *.o *.a
