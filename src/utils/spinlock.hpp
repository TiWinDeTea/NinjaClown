#ifndef NINJACLOWN_UTILS_SPINLOCK_HPP
#define NINJACLOWN_UTILS_SPINLOCK_HPP

#include <atomic>

namespace utils {
class spinlock {
public:
	void lock() noexcept {
		while (m_locked.test_and_set(std::memory_order_acquire))
			;
	}

	void unlock() noexcept {
		m_locked.clear(std::memory_order_release);
	}

private:
	std::atomic_flag m_locked;
};
} // namespace utils

#endif //NINJACLOWN_UTILS_SPINLOCK_HPP
