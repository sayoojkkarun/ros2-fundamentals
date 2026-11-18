#include <iostream>
#include <thread>
#include <atomic>

std::atomic<long> counter = 0;

void work()
{
  {
    for (int i = 0; i < 1'000'000; ++i) {
      ++counter;
    }
  }
}

int main()
{
  std::thread a(work);
  std::thread b(work);
  a.join();
  b.join();
  std::cout << "counter = " << counter << "  (expected 2000000)" << std::endl;
}
