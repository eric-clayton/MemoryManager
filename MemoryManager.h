#include <functional>
#include <unordered_map>
#include <list>
#include <string>
#include <stack>
int bestFit(int sizeInWords, void* list);
int worstFit(int sizeInWords, void* list);
void* listToArray(std::list<uint16_t> freeList, size_t sizeInwords);
int count_digit(int number);
std::stack<int> intStack(int num);
int getSize(void* freeArray);
char* arrToCharArr(void* freeArray, int size);
class MemoryManager
{
	unsigned wordSize;
	size_t sizeInWords;
	unsigned memoryLimit;
	std::function<int(int, void*)> allocator;
	char* memory;
	std::list<uint16_t> freeList;
	unsigned char* bitMap;
	std::unordered_map<int, int> allocatedSize;
public:
	MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator);
	~MemoryManager();
	void initialize(size_t sizeInWords);
	void shutdown();
	void* allocate(size_t sizeInBytes);
	void free(void* address);
	void setAllocator(std::function<int(int, void*)> allocator);
	int dumpMemoryMap(char* filename);
	void* getList();
	void* getBitmap();
	unsigned getWordSize();
	void* getMemoryStart();
	unsigned getMemoryLimit();
	void insertList(int offset, int numWords);
	void removeList(int offset, int numWords);
	void setBitmap(int offset, int numWords);
	void freeBitmap(int offset, int numWords);
	
	std::list<uint16_t>::iterator findIt(int offset);
};
