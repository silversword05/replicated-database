#include <concurrentqueue.h>
#include <utils.h>

int main() {
  moodycamel::ConcurrentQueue<uint> q;

  constexpr uint PER_THREAD = 10000;
  constexpr uint THREAD_COUNT = 10;

  std::atomic<uint> totalEnque;

  std::vector<std::jthread> vec;
  for (uint i = 0; i < THREAD_COUNT; i++) {
    vec.push_back(std::jthread([&](std::stop_token s) {
      for (uint i = 0; i < PER_THREAD; i++) {
        q.enqueue(i);
        if (s.stop_requested()) {
          PRINT("Stopping ");
          totalEnque.fetch_add(i+1);
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }));
  }
  PRINT("Threads created");

  std::this_thread::sleep_for(std::chrono::seconds(2));
  PRINT("Clearing all threads");
  vec.clear();
  PRINT("Total enqueued", totalEnque.load());

  std::jthread deqThread([&]{
    uint itemCount = 0;
    uint item;
    while(q.try_dequeue(item)) itemCount++;
    PRINT("Total items dequed", itemCount);
  });

  deqThread.join();
}