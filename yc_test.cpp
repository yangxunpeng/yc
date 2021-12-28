// yc_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "yc_memory.h"

int main()
{

	yc::spin_lock locker;

	yc::memory mem(1024 * 32, 16, 1024 * 128, 16, 1024 * 512, 8,&locker);

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
	getchar();
}


