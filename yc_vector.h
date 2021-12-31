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

		using iterator = _Tp * ;
		using const_iterator = const _Tp * ;

		using reference = value_type & ;
		using const_reference = const value_type & ;

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
			pt = (value_type*)mem_malloc(size * sizeof(value_type),sizeout);
			if (!pt)
				throw std::bad_alloc();
			if (_pbuf) {
				if (_usize)
					memcpy(pt, _pbuf, _usize * sizeof(value_type));
			}
			_ubufsize = sizeout / sizeof(value_type);
			_pbuf = pt;
		}

		inline void* mem_malloc(size_t size, size_t &sizeout)
		{
			if (_pmem)
				return _pmem->malloc(size, sizeout);
			sizeout = size;
			return ::malloc(size);
		}

		inline void mem_free(void* p)
		{
			if (_pmem)
				_pmem->free(p);

			else
				::free(p);
		}

		inline void* mem_realloc(void* ptr, size_t size, size_t &sizeout)
		{
			if (_pmem)
				return _pmem->realloc(ptr, size, sizeout);
			sizeout = size;
			return ::realloc(ptr,size);
		}

	public:
		vector& append(const value_type* pdata, size_type n)
		{
			if (!n || !pdata) {
				return *this;
			}
			grown(n);
			memcpy(_pbuf + _usize, pdata, n * sizeof(value_type));
			_usize += n;
			return *this;
		}

		iterator insert(size_type pos, const value_type* pdata, size_type n)
		{
			try {
				if (!pdata || !n)
					return begin();
				if (pos >= _usize) {
					append(pdata,n);
					return _pbuf + pos;
				}
				grown(n);
				memmove(_pbuf + pos + n, _pbuf + pos, (_usize - pos) * sizeof(value_type));
				memcpy(_pbuf + pos, pdata, n * sizeof(value_type));
				_usize += n;
				return _pbuf + pos;
			}
			catch (...) {
				return false;
			}

			return true;
		}

		iterator myerase(size_type position)
		{

		}

	public: //容量相关
		inline size_type max_size() const noexcept
		{
			return SIZE_MAX / sizeof(value_type);
		}

		value_type* begin()
		{
			return _usize ? _pbuf : nullptr;
		}

		void shrink_to_fit()
		{
			if (!_usize) { //为空，释放所有空间
				if (_pbuf)
					mem_free(_pbuf);
				_pbuf = nullptr;
				_pos = 0;
				_ubufsize = 0;
				return;
			}
			if (_usize > _ubufsize / 2 || _ubufsize < 64)  //不需要缩小
				return;
			size_t sizeout = 0;
			size_t sizenew = _usize + ((_usize % 8) ? (8u - _usize % 8) : 0 );

			value_type* pnew = (value_type*)mem_realloc(_pbuf, sizenew*sizeof(value_type), sizeout);
			if (!pnew)
				return;
			_pbuf = pnew;
			_ubufsize = sizeout / sizeof(value_type);
		}

	public:// element
		reference operator[] (size_type n)
		{
			return _pbuf[n];
		}

		const_reference operator[] (size_type n) const
		{
			return _pbuf[n];
		}

		reference at(size_type n)
		{
			if(n >= _usize)
				throw std::range_error("range error");
			return _pbuf[n];
		}

		const_reference at(size_type n) const
		{
			if (n >= _usize)
				throw std::range_error("range error");
			return _pbuf[n];
		}

		reference front() noexcept
		{
			return *_pbuf;
		}

		const_reference front() const noexcept
		{
			return *_pbuf;
		}

		reference back() noexcept
		{
			return _pbuf[_usize - 1];
		}

		const_reference back() const noexcept
		{
			return _pbuf[_usize - 1];
		}

		value_type* data() noexcept
		{
			return _pbuf;
		}

		const value_type* data() const noexcept
		{
			return _pbuf;
		}

		inline iterator erase(iterator position)
		{
			return myerase((size_type));
		}

		inline void clear()
		{
			_usize = 0;
		}
	};




}