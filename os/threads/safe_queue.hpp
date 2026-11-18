#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

template<typename T>
class SafeQueue
{
public:
  explicit SafeQueue(std::size_t depth)
  : depth_(depth)
  {
    if (depth_ == 0) {
      throw std::invalid_argument("SafeQueue depth must be at least 1");
    }
  }

  void push(T item)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (items_.size() == depth_) {
        items_.pop_front();
        ++dropped_;
      }
      items_.push_back(std::move(item));
    }
    cv_.notify_one();
  }

  std::optional<T> pop()
  {
    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this] {
        return !items_.empty() || closed_;
      });

    if (closed_ && items_.empty()) {
      return std::nullopt;
    }

    T item = std::move(items_.front());
    items_.pop_front();

    return item;  // a returned local is moved automatically
  }

  void close()
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }

  // Provided: snapshots for printing.
  std::size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_.size();
  }

  std::size_t dropped() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_;
  }

private:
  const std::size_t depth_;
  std::deque<T> items_;
  std::size_t dropped_ = 0;
  bool closed_ = false;
  mutable std::mutex mutex_;     // mutable: size() and dropped() are const but still lock
  std::condition_variable cv_;
};
