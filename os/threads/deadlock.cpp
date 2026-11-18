#include <iostream>
#include <mutex>
#include <thread>

std::mutex sensor_mutex;
std::mutex map_mutex;
long updates = 0;

void localization()
{
  for (int i = 0; i < 100'000; ++i) {
    std::scoped_lock lock(sensor_mutex, map_mutex);
    ++updates;
  }
}

void mapping()  // locks map, then sensor: the opposite order
{
  for (int i = 0; i < 100'000; ++i) {
    std::scoped_lock lock(map_mutex, sensor_mutex);
    ++updates;
  }
}

int main()
{
  std::thread a(localization);
  std::thread b(mapping);
  a.join();
  b.join();
  std::cout << "updates = " << updates << "  (expected 200000)" << std::endl;
}
