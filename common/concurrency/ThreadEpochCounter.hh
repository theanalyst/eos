//------------------------------------------------------------------------------
// File: ThreadEpochCounter.hh
// Author: Abhishek Lekshmanan - CERN
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2023 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/


#pragma once
#include <atomic>
#include <array>
#include <thread>
#include <iostream>
#include <cassert>

namespace eos::common {


#ifdef __cpp_lib_hardware_interference_size
  using std::hardware_constructive_interference_size;
  using std::hardware_destructive_interference_size;
#else
  // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
  constexpr std::size_t hardware_constructive_interference_size = 64;
  constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

/**
* @brief a simple epoch counter per thread that can be used to implement
* RCU-like algorithms. Basically we store a bitfield of
 * 16 bit counter and a 48 bit epoch. If we have no hash collisions, this is fairly
 * simple to implement, you'd only need a simple increment and a memory_order_release
 * store. However, if we have hash collisions, we need to store the oldest epoch
 * as we're tracking the oldest epoch.
 */
template <size_t kMaxThreads=4096>
class SimpleEpochCounter {
public:
  size_t increment(uint64_t epoch, uint16_t count=1) noexcept {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % kMaxThreads;
    // This is 2 instructions, instead of a single CAS. Given that threads
    // will not hash to the same number, we can guarantee that we'd only have one
    // epoch per thread

    auto old = mCounter[tid].load(std::memory_order_relaxed);
    assert(old && 0xFFFF == 0 || (old >> 16) == epoch);
    auto new_val = (epoch << 16) | (old & 0xFFFF) + count;
    mCounter[tid].store(new_val, std::memory_order_release);
    return tid;
  }

  inline void decrement(size_t tid, uint64_t epoch) {
    auto old = mCounter[tid].load(std::memory_order_relaxed);
    mCounter[tid].store(old - ((old << 16) == epoch),
        std::memory_order_release);

  }

  inline void decrement(uint64_t epoch=0) {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % kMaxThreads;
    auto old = mCounter[tid].load(std::memory_order_relaxed);
    mCounter[tid].store(old - 1, std::memory_order_release);
  }

  size_t getReaders(size_t tid) noexcept {
    return mCounter[tid].load(std::memory_order_relaxed) & 0xFFFF;
  }


  bool epochHasReaders(uint64_t epoch) noexcept {
    for (int i=0; i < kMaxThreads; ++i) {
      auto val = mCounter[i].load(std::memory_order_acquire);
      if ((val >> 16) == epoch && (val & 0xFFFF) > 0) {
        return true;
      }
    }
    return false;
  }

private:
  alignas(hardware_destructive_interference_size) std::array<std::atomic<uint64_t>, kMaxThreads> mCounter{0};
};

template<size_t kMaxThreads=4096>
class ThreadEpochCounter {
public:
  size_t increment(uint64_t epoch, uint16_t count=1) {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % kMaxThreads;
    auto old = mCounter[tid].load(std::memory_order_relaxed);
    auto new_val = (epoch << 16) | (old & 0xFFFF) + count;

    // extremely unlikely that we'd have to spin here, but off the rare chance, find out the oldest epoch
    // and store that if some other thread beats us here.
    while (!mCounter[tid].compare_exchange_strong(old,new_val, std::memory_order_acq_rel)) {
      // HASH COLLISION!!! -> store the oldest epoch!
      if ((old >> 16) < epoch) {
        //std::cout << "Old epoch: " << (old >> 16) << " new epoch: " << epoch << std::endl;
        new_val = old;
      } else if ((old >> 16) == epoch) {
        new_val = (epoch << 16) | ((old & 0xFFFF) + count);
      } // we are the oldest epoch, so we don't need to do anything
    }
    return tid;
  }

  inline void decrement(size_t tid) {
    mCounter[tid].fetch_sub(1, std::memory_order_release);
  }

  inline void decrement() {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % kMaxThreads;
    decrement(tid);
  }

  size_t getReaders(size_t tid) noexcept {
    return mCounter[tid].load(std::memory_order_relaxed) & 0xFFFF;
  }

  bool epochHasReaders(uint64_t epoch)
  {
    for (int i = 0; i < kMaxThreads; ++i) {
       auto val = mCounter[i].load(std::memory_order_acquire);
      if ((val >> 16) == epoch && (val & 0xFFFF) > 0) {
         return true;
       }
     }
     return false;
   }
private:
  alignas(hardware_destructive_interference_size) std::array<std::atomic<uint64_t>, kMaxThreads> mCounter{0};
};


} // eos::common
