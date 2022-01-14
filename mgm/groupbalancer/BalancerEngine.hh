#pragma once
//------------------------------------------------------------------------------
// File: BalancerEngine.hh
// Author: Abhishek Lekshmanan - CERN
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2022 CERN/Switzerland                                  *
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

#include <map>
#include <numeric>
#include <cstdint>
#include <string>

namespace eos::mgm::group_balancer {

  //------------------------------------------------------------------------------
  //! @brief Class representing a group's size
  //! It holds the capacity and the current used space of a group.
  //------------------------------------------------------------------------------
  class GroupSize
  {
  public:
    //------------------------------------------------------------------------------
    //! Constructor
    //------------------------------------------------------------------------------
    GroupSize(uint64_t usedBytes, uint64_t capacity) : mSize(usedBytes),
                                                       mCapacity(capacity)
    {}

    //------------------------------------------------------------------------------
    //! Subtracts the given size from this group and adds it to the given toGroup
    //!
    //! @param toGroup the group where to add the size
    //! @param size the file size that should be swapped
    //------------------------------------------------------------------------------
    void swapFile(GroupSize* toGroup, uint64_t size)
    {
      toGroup->mSize += size;
      mSize -= size;
    }

    uint64_t
    usedBytes() const
    {
      return mSize;
    }

    uint64_t
    capacity() const
    {
      return mCapacity;
    }

    double
    filled() const
    {
      return (double) mSize / (double) mCapacity;
    }

  private:
    uint64_t mSize;
    uint64_t mCapacity;
  };

// Allow std::string_view -> std::string lookups on keys
using group_size_map = std::map<std::string,GroupSize, std::less<>>;
using groups_picked_t = std::pair<std::string, std::string>;

// A simple interface to populate the group_size map per group. This is useful
// for DI scenarios where we can alternatively fill in the group_size structures
struct IBalancerInfoFetcher {
  virtual group_size_map fetch() = 0;
};

inline double calculateAvg(const group_size_map& m)
{
  if (!m.size())
    return 0.0;

  return std::accumulate(m.begin(),m.end(),0ULL,
                         [](uint64_t s,const auto& kv) { return s + kv.second.filled(); })/m.size();
}

struct IBalancerEngine
{
  virtual void populateGroupsInfo(IBalancerInfoFetcher* f) = 0;
  virtual void recalculate() = 0;
  virtual void clear() = 0;
  virtual void updateGroupAvg(const std::string& group_name) = 0;
  virtual void updateGroupsAvg() = 0;
  virtual groups_picked_t pickGroupsforTransfer() const = 0;

  // TODO: make this configurable
  virtual void set_threshold(double) = 0;
  virtual ~IBalancerEngine() {};
};

} // namespace eos::mgm::group_balancer
