/*!
\
\author	yangxunpeng
\email  313831106@qq.com
\update
*/

#pragma once

#include <cstdint>
#include <assert.h>
#include "yc_mutex.h"


const int  YC_MEM_MAX_STKS = 16;  //内存池最大块数
const int  YC_MEM_ALIGN_SIZE = sizeof(size_t) * 2;  //对齐
const int  YC_MEM_UP = 4;   //尝试最大分配次数

namespace yc
{
	class memory
	{
	public:
		class stk
		{
		public:
			stk(size_t blksize, size_t blknum) : _blksize(blksize), _pos(0)
			{
				if (_blksize % YC_MEM_ALIGN_SIZE)
					_blksize += YC_MEM_ALIGN_SIZE - _blksize % YC_MEM_ALIGN_SIZE;   //对齐填充
				_pbuf = (unsigned char*)::malloc(_blksize * blknum);
				if (!_pbuf)
					return;
				_pstk = (void**)::malloc(blknum * sizeof(void*));
				if (_pstk)
					_blknum = blknum;
				else {
					_blknum = 0;
					::free(_pbuf);
					_pbuf = nullptr;
					return;
				}
				for (auto i = _blknum; i > 0; i--)   //倒序绑定，方便pop
					_pstk[_pos++] = _pbuf + (i - 1)* _blksize;
			}

			~stk()
			{
				if (_pstk) {
					::free(_pstk);
					_pstk = nullptr;
				}
				if (_pbuf) {
					::free(_pbuf);
					_pbuf = nullptr;
				}
				_pos = 0;
				_blknum = 0;
			}

			void* pop()
			{
				if (_pstk && _pos) {
					--_pos;
					return _pstk[_pos];
				}
				return nullptr;
			}

			inline size_t blksize() const
			{
				return _blksize;
			}
			inline bool empty() const
			{
				return !_pos;
			}
			bool free(void* p) //
			{
				if (_pbuf && reinterpret_cast<size_t>(p) >= (size_t)_pbuf
				    && reinterpret_cast<size_t>(p) < (size_t)_pbuf + _blksize*_blknum) {
					assert(_pos < _blknum);
					_pstk[_pos++] = p;
					return true;
				}
				return false;
			}

			inline bool in(void *p) const  //判断是否在此内存块中
			{
				return (_pbuf && reinterpret_cast<size_t>(p) >= (size_t)_pbuf
				        && reinterpret_cast<size_t>(p) < (size_t)_pbuf + _blksize * _blknum);
			}
		private:
			size_t _blksize, _blknum, _pos;
			unsigned char* _pbuf;
			void ** _pstk;
		};



	public:
		memory(const memory&) = delete;   //防止类拷贝
		memory& operator = (const memory&) = delete;

		memory(spin_lock* plock = nullptr) : _plock(plock), _stknum(0)
		{
		}

		memory(size_t sblksize, size_t sblknum,
		       size_t mblksize = 0, size_t mblknum = 0,
		       size_t lblksize = 0, size_t lblknum = 0,
		       spin_lock* plock = nullptr) : memory(plock)
		{
			add_blk(sblksize, sblknum);
			add_blk(mblksize, mblknum);
			add_blk(lblksize, lblknum);
		}
		memory(size_t sblksize, size_t sblknum,
		       size_t mblksize, size_t mblknum,
		       size_t m2blksize, size_t m2blknum,
		       size_t lblksize, size_t lblknum,
		       spin_lock* plock = nullptr
		      ) :memory(plock)
		{
			add_blk(sblksize, sblknum);
			add_blk(mblksize, mblknum);
			add_blk(m2blksize, m2blknum);
			add_blk(lblksize, lblknum);
		}

		~memory()
		{
			for (auto i = 0u; i < _stknum; i++) {
				delete _stks[i];
				_stks[i] = nullptr;
			}
			_stknum = 0;
		}


		void add_blk(size_t blksize, size_t blknum)
		{
			if (!blksize || !blknum)
				return;

			unique_spinlock locker(_plock);

			if (YC_MEM_MAX_STKS == _stknum)
				return;

			stk* p = new stk(blksize, blknum);
			if (!p)
				return;
			if (p->empty()) {
				delete p;
				return;
			}
			_stks[_stknum++] = p;
			if (_stknum < 2)
				return;
			qsort(_stks, _stknum, sizeof(stk*), compare);
		}

		void* malloc(size_t size)
		{
			size_t len = 0;
			void* p = stkmalloc(size, len);
			if (p)
				return p;
			return ::malloc(size);
		}

		void *malloc(size_t size, size_t &outsize, bool bext = false)
		{
			void *pret = stkmalloc(size, outsize);
			if (pret)
				return pret;
			if (bext) {
				if (size % 16)
					size += 16 - size % 16;
				size += size / 2;
			}
			outsize = size;
			return ::malloc(size);
		}

		void* realloc(void* ptr, size_t size, size_t &outsize)
		{
			if (!ptr)
				return malloc(size, outsize);
			if (!size) {
				free(ptr);
				outsize = 0;
				return nullptr;
			}
			size_t ptrsize = stkblksize(ptr);
			if (!ptrsize) {
				outsize = size;
				return ::realloc(ptr, size); //该指针指向内存是系统分配的
			}

			if (ptrsize >= size && ptrsize < size * 2) {
				outsize = ptrsize;
				return ptr;  //无需重新分配
			}

			void *p = malloc(size, outsize);
			if (p) {
				memcpy(p, ptr, ptrsize <= size ? ptrsize : size);
				printf((const char*)p);
				free(ptr);
			}
			else
				outsize = 0;

			return p;
		}

		void free(void *p)
		{
			if (!stkfree(p))
				::free(p);
		}

	private:
		spin_lock* _plock;
		unsigned int        _stknum;
		stk*       _stks[YC_MEM_MAX_STKS];

	private:
		static int compare(const void* p1, const void* p2)  //根据所属内存块数量排序,从小到大
		{
			const stk* ps1 = (const stk*)p1;
			const stk* ps2 = (const stk*)p2;

			if (ps1->blksize() < ps2->blksize())
				return -1;
			else if (ps1->blksize() == ps2->blksize())
				return 0;
			return 1;
		}

		void* stkmalloc(size_t size, size_t &outlen)
		{
			void* pret = nullptr;
			if (_plock)
				_plock->lock();
			int nup = 0;
			for (auto i = 0u; i < _stknum; i++) {
				if (_stks[i]->blksize() < size)
					continue;
				if (_stks[i]->empty()) {
					++nup;
					if (nup >= YC_MEM_UP)
						break;
				}
				else {
					outlen = _stks[i]->blksize();
					pret = _stks[i]->pop();
					break;
				}
			}
			if (_plock)
				_plock->unlock();

			return pret;
		}

		bool stkfree(void *p)
		{
			if (!p)
				return true;
			bool bret = false;
			if (_plock)
				_plock->lock();
			for (auto i = 0u; i < _stknum; i++) {
				if (_stks[i]->free(p)) {
					bret = true;
					break;
				}
			}
			if (_plock)
				_plock->unlock();


			return bret;
		}

		size_t stkblksize(void *p)
		{
			size_t z = 0;
			if (_plock)
				_plock->lock();
			for (auto i = 0u; i < _stknum; i++) {
				if (_stks[i]->in(p)) {
					z = _stks[i]->blksize();
					break;
				}
			}
			if (_plock)
				_plock->unlock();
			return z;
		}
	};
}