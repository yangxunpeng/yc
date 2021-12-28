#pragma once
#include <stdint.h>

#include "yc_memory.h"

namespace yc
{
	struct null_alloctor {
		yc::memory* operator()()
		{
			return nullptr;
		}
	};

	template<typename _Tp, class _Alloctor = null_alloctor>
	class vector
	{
	public:
		using value_type = typename std::enable_if<std::is_trivially_copyable<_Tp>::value, _Tp>::type;   //校验类型
		using size_type = size_t;

		vector(yc::memory* pmem = nullptr) : _pbuf(nullptr)
			, _pos(0)
			, _usize(0)
			, _ubufsize(0)
			, _pmem(pmem)
		{
			if (!pmem)
				_pmem = _Alloctor()();
		}

		vector(const value_type* s, size_type size, yc::memory* pmem = nullptr) : vector(pmem)
		{
			//append(s,size);
		}

	private:
		yc::memory* _pmem;
		value_type* _pbuf;
		size_t      _pos;
		size_type   _usize;    //使用的大小
		size_type   _ubufsize; //_pbuf总大小，容积

	private:
		void grown(size_type size = 1)
		{
			if (_usize + size <= _ubufsize || !size)
				return;
			size_type usizet;

			usizet = _ubufsize;
			if (!usizet)
				usizet = 4;
			while (usizet < _usize + size)
				usizet *= 2;

			if(usizet > max_size())
				throw std::range_error("range error");

			value_type *pt = nullptr;
			size_t sizeout = 0;
			pt = mem_malloc(size * sizeof(value_type),sizeout);
			if (!pt)
				throw std::bad_alloc();
			if (_pbuf) {
				if (_usize)
					memcpy(pt, _pbuf, _usize * sizeof(value_type));
			}
			_ubufsize = sizeout / sizeof(value_type);
			_pbuf = pt;
		}

		void* mem_malloc(size_t size, size_t &sizeout)
		{
			if (_pmem)
				return _pmem->malloc(size, sizeout);
			sizeout = size;
			return ::malloc(size);

		}

	public:
		void append(const value_type* pdata, size_type n)
		{
			if (!n || !pdata) {
				return *this;
			}

		}


	public: // Capacity
		inline size_type max_size() const noexcept
		{
			return SIZE_MAX / sizeof(value_type);
		}
	};


}