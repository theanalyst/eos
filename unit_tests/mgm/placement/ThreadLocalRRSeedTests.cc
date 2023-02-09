//
// Created by abhi on 2/9/23.
//

#include "mgm/placement/ThreadLocalRRSeed.hh"
#include "gtest/gtest.h"

using eos::mgm::placement::ThreadLocalRRSeed;
TEST(ThreadLocalRRSeed, Initialization)
{
  ThreadLocalRRSeed::init(10, false);
  EXPECT_EQ(ThreadLocalRRSeed::gRRSeeds.size(), 10);
  for (auto i = 0; i < 10; i++) {
    EXPECT_EQ(ThreadLocalRRSeed::gRRSeeds[i], 0);
  }
}

TEST(ThreadLocalRRSeed, Resize)
{
  ThreadLocalRRSeed::init(10, false);
  ThreadLocalRRSeed::resize(20, false);
  EXPECT_EQ(ThreadLocalRRSeed::gRRSeeds.size(), 20);
  for (auto i = 0; i < 20; i++) {
    EXPECT_EQ(ThreadLocalRRSeed::gRRSeeds[i], 0);
  }
}

TEST(ThreadLocalRRSeed, get)
{
  ThreadLocalRRSeed::init(10, false);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 0), 0);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 1), 0);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 0), 1);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 1), 1);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 10), 2);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 0), 12);
}

TEST(ThreadLocalRRSeed, random)
{
  ThreadLocalRRSeed::init(10, true);
  EXPECT_EQ(ThreadLocalRRSeed::gRRSeeds.size(), 10);
  std::vector<uint64_t> seeds;
  for (auto i = 0; i < 10; i++) {
    std::cout << ThreadLocalRRSeed::gRRSeeds[i] << " ";
    seeds.push_back(ThreadLocalRRSeed::gRRSeeds[i]);
  }
  std::cout << "\n";

  EXPECT_EQ(ThreadLocalRRSeed::get(0, 0), seeds[0]);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 1), seeds[0]);
  EXPECT_EQ(ThreadLocalRRSeed::get(0, 0), seeds[0] + 1);
}