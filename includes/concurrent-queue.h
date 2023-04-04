#pragma once

#include <bits/stdc++.h>

template <typename T> class ConcurrentQueue {
public:
  T pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    auto val = queue_.front();
    queue_.pop();
    mlock.unlock();
    cond_.notify_one();
    return val;
  }

  void push(const T &item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }

  ConcurrentQueue() = default;
  ConcurrentQueue(const ConcurrentQueue &) = delete; // disable copying
  ConcurrentQueue &
  operator=(const ConcurrentQueue &) = delete; // disable assignment

private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};