//------------------------------------------------------------------------------
// File: RCULite.hh
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
#include <thread>
#include "common/concurrency/ThreadEpochCounter.hh"

namespace eos::common {

constexpr size_t MAX_THREADS = 4096;

/*
  A Read Copy Update Like primitive that guarantees that is wait-free on the
  readers and guarantees that all memory is protected from deletion. This
  is similar to folly's RCU implementation, but a bit simpler to accomodate
  our use cases.

  Let's say you've a data type that is mostly a read workload with very rare
  updates, with classical RW Locks this is what you'd be doing

  void reader() {
     std::shared_lock lock(shared_mutex);
     process(myconfig);
  }

  A rather simple way to not pay the cost would be using something like
  atomic_unique_ptr

  void reader() {
     auto* config_data = myconfig.get()
     process(config_data);
  }

  void writer() {
    auto *old_config_data = myconfig.reset(new myconfig(config_data));
    // This works and is safe, however we don't know when is a good checkpoint
    // in the program to delete the old_config_data. Deleting when another reader
    // is still accessing the data is something we want to avoid

  }

  void reader() {
    std::shared_lock rlock(my_rcu_domain);
    process(myconfig.get());
  }

  void writer() {
    auto* old_config_data = myconfig.reset(new config(config_data));
    rcu_synchronize();
    delete (old_config_data);
  }


 */

template <typename ListT = SimpleEpochCounter<4096>>
class RCUDomain {
public:

  RCUDomain() = default;

  inline int rcu_read_lock() {
    return mReadersCounter.increment(mEpoch.load(std::memory_order_acquire));
  }

  inline void rcu_read_unlock() {
    mReadersCounter.decrement(mEpoch.load(std::memory_order_acquire));
  }

  inline void rcu_read_unlock(uint64_t tid) {
    mReadersCounter.decrement(mEpoch.load(std::memory_order_acquire), tid);
  }

  inline void rcu_read_unlock_index(uint64_t index) {
    mReadersCounter.decrement_index(index);
  }

  inline void rcu_synchronize() {
    auto curr_epoch = mEpoch.load(std::memory_order_acquire);

    while (!mEpoch.compare_exchange_strong(curr_epoch, curr_epoch + 1,
                                           std::memory_order_acq_rel)) ;

    int i=0;
    while(mReadersCounter.epochHasReaders(curr_epoch)) {
      if (i++ % 20 == 0) {
        std::this_thread::yield();
      }
    }
  }


  inline void lock_shared() noexcept {
    rcu_read_lock();
  }

  inline bool try_lock_shared() noexcept {
    rcu_read_lock();
    return true;
  }

  inline void unlock_shared() noexcept {
    rcu_read_unlock();
  }

  inline void lock() noexcept {}

  inline void unlock() noexcept {
    rcu_synchronize();
  }

private:

  ListT mReadersCounter;
  alignas(hardware_destructive_interference_size) std::atomic<uint64_t> mEpoch{0};
};

using VersionedRCUDomain = RCUDomain<VersionEpochCounter<32768>>;

} // eos::common
