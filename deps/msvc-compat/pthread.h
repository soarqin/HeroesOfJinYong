#pragma once

#include <mutex>

using pthread_mutex_t = std::mutex;

inline int pthread_mutex_init(pthread_mutex_t *, const void *) noexcept {
    return 0;
}

inline int pthread_mutex_destroy(pthread_mutex_t *) noexcept {
    return 0;
}

inline int pthread_mutex_lock(pthread_mutex_t *mutex) noexcept {
    mutex->lock();
    return 0;
}

inline int pthread_mutex_unlock(pthread_mutex_t *mutex) noexcept {
    mutex->unlock();
    return 0;
}
