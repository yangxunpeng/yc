// yc_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "yc_memory.h"
#include "yc_vector.h"

class MyClass
{
public:
	MyClass();
	~MyClass();

	virtual void add()  = 0;
private:
	void *p;
};

MyClass::MyClass()
{
}

MyClass::~MyClass()
{
}

int main()
{
	LARGE_INTEGER cpuFreq;
	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	double runtime = 0.0;
	QueryPerformanceFrequency(&cpuFreq);
	QueryPerformanceCounter(&startTime);


	yc::spin_lock locker;

	yc::memory mem(1024 * 32, 16, 1024 * 128, 16, 1024 * 512, 8,&locker);

	/*
	char buf[16] = "1234567890";
	while (1) {
		char *p = nullptr;
		p = (char*)mem.malloc(16);
		memcpy(p, buf, 16);

		size_t outlen = 0;
		char *p1 = (char*)mem.realloc(p, 4, outlen);
		printf("%s\n",p);
		printf("%zd\n",outlen);

		mem.free(p);
	}
	*/

	byte buf[1024];
	memset(buf, 1, 1024);
	yc::vector<byte> vc(&mem);
	for (int i = 0; i < 500; i++) {
		vc.append(buf,1024);
	}


	QueryPerformanceCounter(&endTime);
	runtime = (((endTime.QuadPart - startTime.QuadPart) * 1000.0f) / cpuFreq.QuadPart);
	printf("%.15lf\n", runtime);
	getchar();
}


