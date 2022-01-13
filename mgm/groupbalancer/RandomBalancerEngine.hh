#pragma once
#include <unordered_set>
//------------------------------------------------------------------------------
// File: RandomBalancerEngine.hh
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

#include "mgm/groupbalancer/BalancerEngine.hh"
#include <random>
namespace eos::mgm::group_balancer {

namespace {
std::random_device rd;
std::mt19937 generator(rd());
}

inline uint32_t getRandom(uint32_t max) {
  std::uniform_int_distribution<> distribution(0,max);
  return distribution(generator);
}


class RandomBalancerEngine final: public IBalancerEngine
{
public:
  void populate(IBalancerInfoFetcher* f) override;
  void recalculate() override;
  void clear() override;
  void updateGroupAvg(const std::string& group_name, double Threshold) override;
  void updateGroupsAvg(double Threshold) override;
  groups_picked_t pickGroupsforTransfer() const override;

private:
  /// groups whose size is over the average size of the groups
  std::unordered_set<std::string> mGroupsOverAvg;
  /// groups whose size is under the average size of the groups
  std::unordered_set<std::string> mGroupsUnderAvg;
  /// groups' sizes cache
  std::map<std::string, GroupSize> mGroupSizes;
  /// average filled percentage in groups
  double mAvgUsedSize;
};

}
