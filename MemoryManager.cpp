#include "MemoryManager.h"
#include <math.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator)
{
	this->wordSize = wordSize;
	this->allocator = allocator;
	bitMap = nullptr;
	memory = nullptr;
}
MemoryManager::~MemoryManager()
{
	this->shutdown();
}
void MemoryManager::initialize(size_t sizeInWords)
{
	this->shutdown();
	this->sizeInWords = sizeInWords;
	memoryLimit = sizeInWords * wordSize;
	memory = new char[memoryLimit];
	int bitmapSize = (sizeInWords / 8 + (sizeInWords % 8 != 0)) + 2;
	bitMap = new unsigned char[bitmapSize]{0};
	bitMap[0] = (bitmapSize - 2) & 0xFF;
	bitMap[1] = (bitmapSize - 2) & 0xFF00;
	freeList.push_back(1);
	freeList.push_back(0);
	freeList.push_back(sizeInWords);
}
void MemoryManager::shutdown()
{
	if (memory != nullptr)
	{
		delete[] memory;
		memory = nullptr;
	}
	if (bitMap != nullptr)
	{
		delete[] bitMap;
		bitMap = nullptr;
	}
	freeList.clear();
	allocatedSize.clear();
}
void* MemoryManager::allocate(size_t sizeInBytes)
{
	if (freeList.size() == 1)
	{
		return nullptr;
	}
	int offset;
	int numWords = sizeInBytes / wordSize;
	uint16_t* temp = static_cast<uint16_t*>(listToArray(freeList, sizeInWords));
	if (temp == nullptr)
	{
		offset = 0;
	}
	else
	{
		offset = allocator(numWords, temp);
	}
	delete[] temp;
	if (offset == -1)
	{
		return nullptr;
	}
	void* nArr = memory + (wordSize * offset);
	allocatedSize[offset] = numWords;
	insertList(offset, numWords);
	setBitmap(offset, numWords);
	return nArr;
}
void MemoryManager::free(void* address)
{
	int index = (static_cast<char*>(address) - memory) / wordSize;
	int size = allocatedSize[index];
	removeList(index, size);
	freeBitmap(index, size);
	allocatedSize.erase(index);

}
void MemoryManager::setAllocator(std::function<int(int, void*)> allocator)
{
	this->allocator = allocator;
}
int MemoryManager::dumpMemoryMap(char* filename)
{
	uint16_t* temp = static_cast<uint16_t*>(listToArray(freeList, sizeInWords));
	int size = getSize(temp);
	char* buff = arrToCharArr(temp, size);
	int fd1;
	int w1;
	creat(filename, S_IRWXU);
	fd1 = open(filename, O_RDWR);
	if (fd1 == -1)
	{
		return -1;
	}
	w1 = write(fd1, buff, size);
	if (w1 == -1)
	{
		return -1;
	}
	delete[] buff;
	delete[] temp;
	return 0;
}
void* MemoryManager::getList()
{
	void* temp = listToArray(freeList, sizeInWords);
	return temp;
}
void* MemoryManager::getBitmap()
{
	int bitmapSize = (sizeInWords / 8 + (sizeInWords % 8 != 0)) + 2;
	unsigned char* temp = new unsigned char[bitmapSize];
	for (int i = 0; i < bitmapSize; i++)
	{
		temp[i] = bitMap[i];
	}
	return temp;
}
unsigned MemoryManager::getWordSize()
{
	return wordSize;
}
void* MemoryManager::getMemoryStart()
{
	return memory;
}
unsigned MemoryManager::getMemoryLimit()
{
	return memoryLimit;
}
void MemoryManager::insertList(int offset, int numWords)
{
	auto it1 = findIt(offset);
	if (it1 == freeList.end())
	{
		std::cout << "findIt failed\n"; 
	}
	auto it2 = it1;
	it2++;
	int newOffset = *it1 + numWords;
	int newSize = *it2 - numWords;
	if (newSize == 0)
	{
		freeList.erase(it1);
		freeList.erase(it2);
		(*freeList.begin())--;
	}
	else
	{
		*it1 = newOffset;
		*it2 = newSize;
	}
}
void MemoryManager::removeList(int offset, int numWords)
{
	int start = offset;
	int end = offset + numWords - 1;
	auto front = freeList.begin();
	front++;
	auto curr = freeList.end();
	for (auto it = front; it != freeList.end(); std::advance(it, 2))
	{
		if (*it > start)
		{
			curr = it;
			break;
		}
	}
	auto temp = curr;
	if (curr != front)
	{
		std::advance(temp, -1);
		int prevSize = *temp;
		std::advance(temp, -1);
		int prevOffset = *temp;
		int prevEnd = prevOffset + prevSize - 1;

		if (prevEnd == start - 1)
		{
			temp++;
			*temp += numWords;
			if (curr == freeList.end())
			{	
				return;
			}
			int newEnd = prevOffset + *temp - 1;
			if (newEnd == *curr - 1)
			{
				auto temp2 = curr;
				temp2++;
				*temp += *temp2;
				freeList.erase(curr);
				freeList.erase(temp2);
			}
		}
		else
		{
			freeList.insert(curr,offset);
			auto temp2 = freeList.insert(curr,numWords);
			(*freeList.begin())++;
			if (curr == freeList.end())
			{
				return;
			}
			if (end == *curr - 1)
			{
				temp = curr;
				temp++;
				*temp2 += *temp;
				freeList.erase(curr);
				freeList.erase(temp);
				(*freeList.begin())--;
			}
		}
	}
	else
	{
		if (*curr == end - 1)
		{
			temp++;
			freeList.insert(curr, offset);
			freeList.insert(curr, numWords + *temp);
			freeList.erase(curr);
			freeList.erase(temp);
		}
		else
		{
			freeList.insert(curr, offset);
			freeList.insert(curr, numWords);
			(*freeList.begin())++;
		}
	}
}
void MemoryManager::setBitmap(int offset, int numWords)
{
	int index = (offset / 8) + 2;
	int temp = index;
	int j = offset;
	int num = 0;
	int power = (j % 8);
	for (int i = 0; i < numWords; i++)
	{
		num += pow(2, power);
		j++;
		power = (j % 8);
		if (power == 0)
		{
			bitMap[index] += num;
			index =(j / 8) + 2;
			num = 0;
		}
	}
	bitMap[index] += num;

}
void MemoryManager::freeBitmap(int offset, int numWords)
{
	int index = (offset / 8) + 2;
	int temp = index;
	int j = offset;
	int num = 0;
	int power = (j % 8);
	for (int i = 0; i < numWords; i++)
	{
		num += pow(2, power);
		j++;
		power = (j % 8);
		if (power == 0)
		{
			bitMap[index] ^= num;
			index = (j / 8) + 2;
			num = 0;
		}
	}
	bitMap[index] ^= num;

}
std::list<uint16_t>::iterator MemoryManager::findIt(int offset)
{
	int holes = *freeList.begin();
	auto l = freeList.begin();
	l++;
	auto r = freeList.end();
	std::advance(r, -2);
	while (*l <= *r) 
	{
		auto mid = l;
		int midDist = std::distance(l, r) / 4;
		if (midDist % 2 != 0)
		{
			midDist--;
		}
		std::advance(mid, midDist);

		// Check if x is present at mid
		if (*mid == offset)
			return mid;

		// If x greater, ignore left half
		if (*mid < offset)
			std::advance(l, 2);

		// If x is smaller, ignore right half
		else
			std::advance(l, -2);
	}
	return freeList.end();
}
int bestFit(int sizeInWords, void* list)
{
	uint16_t* holeList = static_cast<uint16_t*>(list);
	uint16_t holeListlength = *holeList++;
	bool found = false;
	uint16_t ans;
	uint16_t ansSize;
	for (uint16_t i = 1; i < (holeListlength) * 2; i += 2) 
	{
		if (holeList[i] >= sizeInWords)
		{
			if (!found)
			{
				ans = holeList[i - 1];
				ansSize = holeList[i];
				found = true;
			}
			else if (holeList[i] <= ansSize)
			{
				ans = holeList[i - 1];
				ansSize = holeList[i];
			}
		}
	}
	if (!found)
	{
		return -1;
	}
	return ans;
}
int worstFit(int sizeInWords, void* list)
{
	uint16_t* holeList = static_cast<uint16_t*>(list);
	uint16_t holeListlength = *holeList++;
	uint16_t ans = -1;
	uint16_t ansSize = holeList[1];
	for (uint16_t i = 1; i < (holeListlength) * 2; i += 2)
	{
		if (holeList[i] >= sizeInWords && holeList[i] >= ansSize)
		{
			ans = holeList[i - 1];
			ansSize = holeList[i];
		}
	}
	if (ans == -1)
	{
		return -1;
	}
	return ans;
}
void* listToArray(std::list<uint16_t> freeList, size_t sizeInWords)
{
	size_t size = freeList.size();
	auto front = freeList.begin();
	front++;
	auto back = freeList.end();
	std::advance(back, -1);
	
	if (*front == 0 && *back == sizeInWords)
	{
		return nullptr;
	}
	uint16_t* a = new uint16_t[size];
	size_t j = 0;
	for (uint16_t i : freeList)
	{
		 a[j++] = i;
	}
	return a;
}
int count_digit(int number) 
{
	if(number == 0)
	{
		return 1;
	}
	int ans = int(log10(number) + 1);
	return ans;
}
std::stack<int> intStack(int num)
{
	std::stack<int> s;
	if (num == 0)
	{
		s.push(0);
		return s;
	}
	while (num > 0) {
		s.push(num % 10);
		num = num / 10;
	}
	return s;
}
int getSize(void* freeArray)
{
	uint16_t* arr = static_cast<uint16_t*>(freeArray);
	int size = arr[0];
	size = (7 * size) - 3;
	for (int i = 1; i <= arr[0] * 2; i++)
	{
		size += count_digit(arr[i]);
	}
	return size;
}
char* arrToCharArr(void* freeArray, int size)
{
	uint16_t* arr = static_cast<uint16_t*>(freeArray);
	if (arr == nullptr)
	{
		return nullptr;
	}
	char* buff = new char[size];
	int i = 0;
	int index = 1;
	while(i < size)
	{
		buff[i++] = '[';
		std::stack<int> s = intStack(arr[index++]);
		while (!s.empty()) 
		{
			buff[i++] = '0' + s.top();
			s.pop();
		}
		buff[i++] = ',';
		buff[i++] = ' ';
		s = intStack(arr[index++]);
		while (!s.empty())
		{
			buff[i++] = '0' + s.top();
			s.pop();
		}
		buff[i++] = ']';
		if (i == size)
		{
			break;
		}
		buff[i++] = ' ';
		buff[i++] = '-';
		buff[i++] = ' ';
	}
	return buff;
}
