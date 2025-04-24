#pragma once
#include <chrono>

struct Timer
{
  using time_point = std::chrono::high_resolution_clock::time_point;
  time_point startTime;
  Timer(): startTime(std::chrono::high_resolution_clock::now()) {}
  void reset() { startTime = std::chrono::high_resolution_clock::now(); }
  float elapsed() const
  {
    time_point curTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> d = curTime - startTime;
    return d.count();
  }
  float elapsed_ms() const
  {
    time_point curTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> d = curTime - startTime;
    return d.count();
  }
  float elapsed_us() const
  {
    time_point curTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::micro> d = curTime - startTime;
    return d.count();
  }
};
