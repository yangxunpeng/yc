#pragma once
#include <atomic>

namespace yc
{
	class spin_lock  //зда§Ыј
	{
		std::atomic_flag flag = ATOMIC_FLAG_INIT;
	public:
		spin_lock() = default;
		spin_lock(const spin_lock&) = delete;
		spin_lock& operator= (const spin_lock&) = delete;
		void lock()
		{
			while (flag.test_and_set(std::memory_order_acquire))
				;
		}
		void unlock()
		{
			flag.clear(std::memory_order_release);
		}
	};

	class unique_spinlock
	{
	private:
		spin_lock *_plck;
	public:
		unique_spinlock(const unique_spinlock&) = delete;
		unique_spinlock& operator = (const unique_spinlock&) = delete;

		unique_spinlock(spin_lock *plck) : _plck(plck)
		{
			if (_plck)
				_plck->lock();
		}
		~unique_spinlock()
		{
			if (_plck)
				_plck->unlock();
		}
	};
}