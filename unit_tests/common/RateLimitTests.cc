//------------------------------------------------------------------------------
//! @file RateLimitTests.cc
//! @author Elvin-Alin Sindrilaru <esindril at cern dot ch>
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2019 CERN/Switzerland                                  *
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

#include "gtest/gtest.h"
#define IN_TEST_HARNESS
#include "common/RateLimit.hh"
#undef IN_TEST_HARNESS
#include <thread>
#include <vector>
#include <list>

using namespace std::chrono;

//------------------------------------------------------------------------------
// Test rate limiter basic functionality
//------------------------------------------------------------------------------
TEST(RequestRateLimit, BasicFunctionality)
{
  using namespace eos::common;
  int initial_delay = 5;
  RequestRateLimit rlimit(true);
  rlimit.SetRatePerSecond(1000001);
  ASSERT_EQ(0ull, rlimit.GetRatePerSecond());
  ASSERT_NO_THROW(rlimit.SetRatePerSecond(1));
  auto& clock = rlimit.GetClock();
  clock.advance(seconds(initial_delay)); // fake clock starts at 0

  // With time passing we should be able to submit one request per second
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(0, rlimit.Allow());
    clock.advance(std::chrono::seconds(1));
  }

  // Without time passing allow should return non-zero values
  ASSERT_EQ(0, rlimit.Allow());

  for (int i = 0; i < 10; ++i) {
    auto delay = rlimit.Allow();
    ASSERT_NE(0, delay);
    clock.advance(std::chrono::microseconds(delay));
  }
}

//------------------------------------------------------------------------------
// Test rate limiter in multithreaded mode
//------------------------------------------------------------------------------
TEST(RequestRateLimit, MultiThread)
{
  using namespace std::chrono;
  using namespace eos::common;
  std::vector<std::thread> threads;
  std::list<int> rates {5, 10, 100};

  for (auto& rate : rates) {
    std::unique_ptr<RequestRateLimit> rlimit(new RequestRateLimit(true));
    auto& clock = rlimit->GetClock();
    clock.advance(std::chrono::seconds(5));  // fake clock starts at 0
    ASSERT_NO_THROW(rlimit->SetRatePerSecond(rate));
    uint64_t start_us = duration_cast<microseconds>
                        (clock.getTime().time_since_epoch()).count();
    auto func = [&](int indx) noexcept {
      for (int i = 0; i < rate; ++i) {
        (void)rlimit->Allow();
      }
    };

    // With time passing we should be able to submit one request per second per
    // thread ---> "rate" seconds to submit everything from everyone
    for (int i = 0; i < rate; ++i) {
      threads.push_back(std::thread(func, i));
    }

    for (auto& t : threads) {
      if (t.joinable()) {
        t.join();
      }
    }

    uint64_t dur_ms = (rlimit->mLastTimestampUs - start_us) / 1000;
    // With x slots per second and x threads submitting x requests it should take
    // around x seconds to run this. Add +/-5% seconds tolerance.
    ASSERT_GE(dur_ms, (int)(rate * 1000 * 0.95));
    ASSERT_LE(dur_ms, (int)(rate * 1000 * 1.05));
    std::cout << "Run took: " << dur_ms << " (fake)ms" << std::endl;
  }
}

TEST(InvokeWithBackOff, Simple)
{
  using namespace eos::common;
  uint16_t counter = 0;
  uint16_t limit = 1;
  uint64_t invoke_counter = 0;
  uint64_t total_invocations =0;

  while(invoke_counter < 5) {
    invoke_counter += InvokeWithBackOff(counter, limit, []() {});
    ++total_invocations;
  }
  EXPECT_EQ(counter, 16);
  EXPECT_EQ(limit, 32);
  EXPECT_EQ(total_invocations, 16);

  while(invoke_counter < 15) {
    invoke_counter += InvokeWithBackOff(counter, limit, []() {});
    ++total_invocations;
  }
  EXPECT_EQ(counter, 16384);
  EXPECT_EQ(limit, 32768);
  EXPECT_EQ(total_invocations, 16384);
  while(invoke_counter < 16) {
    invoke_counter += InvokeWithBackOff(counter, limit, []() {});
    ++total_invocations;
  }
  EXPECT_EQ(counter,32768);
  EXPECT_EQ(limit, 0);
  EXPECT_EQ(total_invocations, 32768);

  // counter goes to 32769 gets reset to 0, limit goes to 1;
  // you get one more invocation when the reset happens
  while(invoke_counter < 17) {
    invoke_counter += InvokeWithBackOff(counter, limit, []() {});
    ++total_invocations;
  }
  EXPECT_EQ(counter, 1);
  EXPECT_EQ(limit, 2);
  EXPECT_EQ(total_invocations, 32770);

  while(invoke_counter < 18) {
    invoke_counter += InvokeWithBackOff(counter, limit, []() {});
    ++total_invocations;
  }
  EXPECT_EQ(counter, 2);
  EXPECT_EQ(limit, 4);
  EXPECT_EQ(total_invocations, 32771);
 }
