// /************************************************************************
//  * EOS - the CERN Disk Storage System                                   *
//  * Copyright (C) 2023 CERN/Switzerland                           *
//  *                                                                      *
//  * This program is free software: you can redistribute it and/or modify *
//  * it under the terms of the GNU General Public License as published by *
//  * the Free Software Foundation, either version 3 of the License, or    *
//  * (at your option) any later version.                                  *
//  *                                                                      *
//  * This program is distributed in the hope that it will be useful,      *
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
//  * GNU General Public License for more details.                         *
//  *                                                                      *
//  * You should have received a copy of the GNU General Public License    *
//  * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
//  ************************************************************************
//

//
// Created by Abhishek Lekshmanan on 02/02/2023.
//

#include "common/concurrency/RCULite.hh"
#include "common/concurrency/AtomicUniquePtr.h"
#include "gtest/gtest.h"
#include <shared_mutex>

TEST(RCUTests, Basic)
{
  using namespace eos::common;

  // Test that we can create an RCU object
  RCUDomain rcu_domain;
  atomic_unique_ptr<int> ptr(new int(0));
  int sum{0};
  int i{0};
  // Test that we can create an RCU read lock
  auto read_fn = [&rcu_domain,&i, &sum,&ptr]() {
    rcu_domain.rcu_read_lock();
    sum += *ptr;
    rcu_domain.rcu_read_unlock();
  };

  std::thread writer([&rcu_domain, &ptr, &i]() {
    for (int j=0; j < 1000; ++j) {
      auto old_ptr = ptr.reset(new int(i++));
      rcu_domain.rcu_synchronize();
      delete old_ptr;
    }
  });

  std::vector<std::thread> readers;
  for (int i=0; i<100; ++i) {
    readers.emplace_back(read_fn);
  }
  for (int i=0; i<100; ++i) {
    readers[i].join();
  }

} // namespace eos::common