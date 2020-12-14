//------------------------------------------------------------------------------
// File: RaidGroupTests.cc
// Author: Elvin Sindrilaru - CERN
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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#define IN_TEST_HARNESS
#include "fst/layout/rain/RainGroup.hh"
#undef IN_TEST_HARNESS

using namespace eos::common;

static std::unique_ptr<char[]> dummy_data(new char[1 * MB]);

TEST(RainBlock, FillLimits)
{
  eos::fst::RainBlock block(nullptr, 4 * MB, 1 * MB);
  std::list<std::pair<uint64_t, uint64_t>> pieces {
    {4 * MB,  256 * KB},
    {4 * MB + 256 * KB, 256 * KB},
    {4 * MB + 512 * KB, 256 * KB},
    {4 * MB + 768 * KB, 256 * KB}
  };
  // Piece outside the block range at the beginning
  ASSERT_FALSE(block.StoreData(dummy_data.get(), 3 * MB, 1 * KB));
  ASSERT_FALSE(block.StoreData(dummy_data.get(), 3 * MB + 900 * KB, 500 * KB));
  // Piece outside the block range at the end
  ASSERT_FALSE(block.StoreData(dummy_data.get(), 4 * MB + 900 * KB, 500 * KB));
  ASSERT_FALSE(block.StoreData(dummy_data.get(), 5 * MB, 2 * KB));
  ASSERT_FALSE(block.IsComplete());

  // Add all the pieces that fill the entire block
  for (const auto& elem : pieces) {
    ASSERT_TRUE(block.StoreData(dummy_data.get(), elem.first, elem.second));
  }

  ASSERT_TRUE(block.IsComplete());
}

TEST(RainBlock, HandleHoles)
{
  eos::fst::RainBlock block(nullptr, 4 * MB, 1 * MB);
  std::list<std::pair<uint64_t, uint64_t>> pieces {
    {4 * MB,  100 * KB},
    {4 * MB + 200 * KB, 300 * KB},
    {4 * MB + 600 * KB, 200 * KB},
    {4 * MB + 900 * KB, 124 * KB}
  };
  // Holes at the following start/end offsets
  std::list<std::pair<uint64_t, uint64_t>> expected_holes {
    {4 * MB + 100 * KB, 4 * MB + 200 * KB - 1},
    {4 * MB + 500 * KB, 4 * MB + 600 * KB - 1},
    {4 * MB + 800 * KB, 4 * MB + 900 * KB - 1}
  };

  for (const auto& elem : pieces) {
    ASSERT_TRUE(block.StoreData(dummy_data.get(), elem.first, elem.second));
  }

  ASSERT_FALSE(block.IsComplete());
  auto holes = block.GetListHoles();
  ASSERT_EQ(3, holes.size());

  for (const auto& hole : holes) {
    bool found = false;

    for (const auto& exp_hole : expected_holes) {
      if ((hole.first == exp_hole.first) &&
          (hole.second == exp_hole.second)) {
        found = true;
        break;
      }
    }

    ASSERT_TRUE(found);
  }

  // Fill the holes and we should get a complete block at the end
  for (const auto& elem : expected_holes) {
    ASSERT_TRUE(block.StoreData(dummy_data.get(), elem.first,
                                elem.second - elem.first));
  }

  ASSERT_TRUE(block.IsComplete());
}

TEST(RainBlock, CompleteWithZeros)
{
  eos::fst::RainBlock block(nullptr, 4 * MB, 1 * MB);
  // Note there is hole between the two pieces
  std::list<std::pair<uint64_t, uint64_t>> pieces {
    {4 * MB,  100 * KB},
    {4 * MB + 200 * KB, 300 * KB},
  };

  for (const auto& elem : pieces) {
    ASSERT_TRUE(block.StoreData(dummy_data.get(), elem.first, elem.second));
  }

  ASSERT_FALSE(block.IsComplete());
  ASSERT_FALSE(block.CompleteWithZeros());
  // Fill the hole and try again
  ASSERT_TRUE(block.StoreData(dummy_data.get(), 4 * MB + 100 * KB, 100 * KB));
  ASSERT_TRUE(block.CompleteWithZeros());
  ASSERT_TRUE(block.IsComplete());
}

TEST(RainBlock, Reset)
{
  eos::fst::RainBlock block(nullptr, 4 * MB, 1 * MB);
  // Note there is hole between the two pieces
  std::list<std::pair<uint64_t, uint64_t>> pieces {
    {4 * MB,  100 * KB},
    {4 * MB + 200 * KB, 300 * KB},
  };

  for (const auto& elem : pieces) {
    ASSERT_TRUE(block.StoreData(dummy_data.get(), elem.first, elem.second));
  }

  ASSERT_EQ(4 * MB, block.GetOffset());
  block.Reset(nullptr, 12 * MB);
  ASSERT_EQ(12 * MB, block.GetOffset());
  ASSERT_TRUE(block.GetListHoles().empty());
  ASSERT_FALSE(block.IsComplete());
}
