//------------------------------------------------------------------------------
// File: AtomicUniquePtrTests.hh
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

#include "common/concurrency/AtomicUniquePtr.h"
#include <gtest/gtest.h>

TEST(AtomicUniquePtr, Basic)
{
  eos::common::atomic_unique_ptr<int> p(new int(1));
  EXPECT_EQ(*p.get(), 1);
  auto old_ptr = p.release();
  EXPECT_EQ(*old_ptr, 1);
  EXPECT_EQ(p.get(), nullptr);
  std::unique_ptr<int> old_ptr_guard(old_ptr);
}

TEST(AtomicUniquePtr, Reset)
{
  eos::common::atomic_unique_ptr<int> p(new int(1));
  EXPECT_EQ(*p.get(), 1);
  auto old_ptr = p.reset(new int(2));
  std::shared_ptr<int> old_ptr_guard(old_ptr);
  EXPECT_EQ(*p.get(), 2);
  EXPECT_EQ(*old_ptr_guard.get(), 1);
}

TEST(AtomicUniquePtr, MoveCtor)
{
  eos::common::atomic_unique_ptr<int> p1(new int(1));
  eos::common::atomic_unique_ptr<int> p2(std::move(p1));
  EXPECT_EQ(*p2.get(), 1);
  EXPECT_EQ(p1.get(), nullptr);
}

TEST(AtomicUniquePtr, reset_from_null)
{
  eos::common::atomic_unique_ptr<int> p;
  EXPECT_EQ(p.get(), nullptr);
  p.reset_from_null(new int(1));
  EXPECT_EQ(*p.get(), 1);
}

TEST(AtomicUniquePtr, MemberAccessOperator)
{
  struct A  {
    std::string data;
    explicit A(std::string d) : data(d) {}
  };
  eos::common::atomic_unique_ptr<A> p(new A("hello"));
  EXPECT_EQ(p->data, "hello");
}

TEST(AtomicUniquePtr, VectorOfAtomics)
{
  std::vector<eos::common::atomic_unique_ptr<int>> v;
  v.emplace_back(new int(1));
  v.emplace_back(new int(2));
  v.emplace_back(new int(3));
  EXPECT_EQ(*v[0].get(), 1);
  EXPECT_EQ(*v[1].get(), 2);
  EXPECT_EQ(*v[2].get(), 3);
}

TEST(AtomicUniquePtr, SimpleGC)
{
  std::vector<eos::common::atomic_unique_ptr<int>> v;
  eos::common::atomic_unique_ptr p(new int(1));
  int * p_ = p.reset(new int(2));   // p_ points to 1
  v.emplace_back(p_);
  EXPECT_EQ(*p.get(), 2);
  EXPECT_EQ(*v[0].get(), 1); // v[0] points to 1
}
