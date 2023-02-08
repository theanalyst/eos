#include "common/concurrency/ThreadEpochCounter.hh"
#include <gtest/gtest.h>

TEST(SimpleEpochCounter, Basic)
{
  eos::common::experimental::SimpleEpochCounter counter;
  ASSERT_FALSE(counter.epochHasReaders(0));
  int epoch=1;
  auto tid = counter.increment(epoch, 1);
  ASSERT_TRUE(counter.epochHasReaders(epoch));
  ASSERT_EQ(counter.getReaders(tid), 1);
  counter.decrement();
  ASSERT_FALSE(counter.epochHasReaders(epoch));
}

TEST(SimpleEpochCounter, HashCollision)
{
  eos::common::experimental::SimpleEpochCounter<2> counter;
  ASSERT_FALSE(counter.epochHasReaders(0));
  std::array<std::atomic<int>, 2> epoch_counter = {0,0};
  std::vector<std::thread> threads;
  for (int i=0; i < 100; ++i) {
    threads.emplace_back([&counter, &epoch_counter, &i](){
      int epoch = (i & 1);
      auto tid = counter.increment(epoch, 1);
      epoch_counter[tid]++;
    });
  }

  for (auto& t: threads) {
    t.join();
  }

  ASSERT_EQ(epoch_counter[0], counter.getReaders(0));
  ASSERT_EQ(epoch_counter[1], counter.getReaders(1));
}

TEST(ThreadEpochCounter, HashCollision)
{
  eos::common::experimental::ThreadEpochCounter<2> counter;
  ASSERT_FALSE(counter.epochHasReaders(0));
  std::array<std::atomic<int>, 2> epoch_counter = {0,0};

  std::vector<std::thread> threads;
  for (int i=0; i < 100; ++i) {
    threads.emplace_back([&counter, &epoch_counter, &i](){
      int epoch = (i & 1); // FIXME vary epochs to break this!
      auto tid = counter.increment(epoch, 1);
      epoch_counter[tid]++;
    });
  }

  for (auto& t: threads) {
    t.join();
  }

  ASSERT_EQ(epoch_counter[0], counter.getReaders(0));
  ASSERT_EQ(epoch_counter[1], counter.getReaders(1));

}

TEST(VersionEpochCounter, Basic)
{
  eos::common::VersionEpochCounter counter;
  ASSERT_FALSE(counter.epochHasReaders(0));
  int epoch=1;
  auto tid = counter.increment(epoch, 1);
  ASSERT_TRUE(counter.epochHasReaders(epoch));
  ASSERT_EQ(counter.getReaders(tid), 1);
  counter.decrement(epoch);
  ASSERT_FALSE(counter.epochHasReaders(epoch));
}

TEST(VersionEpochCounter, MultiThreaded)
{
  eos::common::VersionEpochCounter<2> counter;
  ASSERT_FALSE(counter.epochHasReaders(0));
  std::array<std::atomic<int>, 2> epoch_counter = {0,0};
  std::vector<std::thread> threads;
  for (int i=0; i < 100; ++i) {
    threads.emplace_back([&counter, &epoch_counter, &i](){
      int epoch = (i & 1);
      auto tid = counter.increment(epoch, 1);
      epoch_counter[tid]++;
    });
  }

  for (auto& t: threads) {
    t.join();
  }

  ASSERT_EQ(epoch_counter[0], counter.getReaders(0));
  ASSERT_EQ(epoch_counter[1], counter.getReaders(1));
}