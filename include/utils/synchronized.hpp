#ifndef NINJACLOWN_UTILS_SYNCHRONIZED_HPP
#define NINJACLOWN_UTILS_SYNCHRONIZED_HPP

#include <mutex>

namespace utils {
template <typename StoredValue, typename Mutex = std::mutex>
class synchronized {

    template <typename Sync, typename UnderlyingType>
    struct acquired_impl {
        explicit acquired_impl(Sync &s) noexcept(noexcept(s.mutex.lock()))
          : sync{s}
        {
            sync.mutex.lock();
        }
        ~acquired_impl()
        {
            sync.mutex.unlock();
        }

        [[nodiscard]] UnderlyingType *operator->() noexcept
        {
            return &(sync.stored_value);
        }

        [[nodiscard]] UnderlyingType &operator*() noexcept
        {
            return sync.stored_value;
        }

        Sync &sync;
    };

public:
    using acquired_t = acquired_impl<synchronized, StoredValue>;
    using const_acquired_t = acquired_impl<const synchronized, const StoredValue>;

    template <typename... Args>
    explicit synchronized(Args &&... args) noexcept(noexcept(Mutex{}) && noexcept(StoredValue{std::forward<Args>(args)...}))
      : mutex{}
      , stored_value(std::forward<Args>(args)...)
    {
    }

    [[nodiscard]] acquired_t acquire() noexcept(noexcept(acquired_t{std::declval<synchronized&>()})) {
        return acquired_t{*this};
    }

    [[nodiscard]] const_acquired_t acquire() const noexcept(noexcept(const_acquired_t{std::declval<const synchronized&>()})) {
        return const_acquired_t{*this};
    }

private:
    mutable Mutex mutex;
    StoredValue stored_value;
};
}

#endif //NINJACLOWN_UTILS_SYNCHRONIZED_HPP
