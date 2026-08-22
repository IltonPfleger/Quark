#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

void worker(int identifier) {
  while (true) {
    std::cout << identifier << std::endl;
  }
}

int main() {
  const int total = 4;
  std::vector<std::thread> pool;

  for (int i = 0; i < total; ++i) {
    pool.emplace_back(worker, i + 1);
  }

  for (auto &entry : pool) {
    entry.join();
  }

  return 0;
}
