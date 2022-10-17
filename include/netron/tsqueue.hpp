#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>

namespace netron
{

  template<typename T>
  class tsqueue
  {
  public:
    tsqueue() = default;
    tsqueue(const tsqueue<T>&) = delete;
    virtual ~tsqueue() { clear(); }

    // Returns and maintains item at front of queue
    const T& front()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_queue.front();
    }

    // Returns and maintains item at back of queue
    const T& back()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_queue.back();
    }

    // Adds an item to the back of the queue
    void push_back(const T& item)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_queue.emplace_back(std::move(item));
    }

    // Adds an item to the front of the queue
    void push_front(const T& item)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_queue.emplace_front(std::move(item));
    }

    // Returns true if queue has no items
    bool empty()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_queue.empty();
    }

    // Returns number of items in queue
    size_t count()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_queue.size();
    }

    // Clears the queue
    void clear()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_queue.clear();
    }

    // Removes and returns item from front of queue
    T pop_front()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto t = std::move(m_queue.front());
      m_queue.pop_front();
      return t;
    }

    // Removes and returns item from back of queue
    T pop_back()
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto t = std::move(m_queue.back());
      m_queue.pop_back();
      return t;
    }

  protected:
    std::deque<T> m_queue;
    std::mutex m_mutex;
  };

}