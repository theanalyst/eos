//------------------------------------------------------------------------------
// File: RandomBalancerEngine.cc
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

#include "mgm/groupbalancer/RandomBalancerEngine.hh"
#include "common/Logging.hh"

namespace eos::mgm::group_balancer {

void RandomBalancerEngine::populateGroupsInfo(IBalancerInfoFetcher* f)
{
  mGroupSizes = f->fetch();
  recalculate();
  updateGroupsAvg();
}

void RandomBalancerEngine::recalculate()
{
  mAvgUsedSize = calculateAvg(mGroupSizes);
}

void RandomBalancerEngine::clear()
{
  mGroupSizes.clear();
  mGroupsOverAvg.clear();
  mGroupsUnderAvg.clear();
}

void RandomBalancerEngine::updateGroupAvg(const std::string& group_name)
{
  auto kv = mGroupSizes.find(group_name);
  if (kv == mGroupSizes.end()) {
    return;
  }

  const GroupSize& groupSize = kv->second;
  double diffWithAvg = ((double) groupSize.filled()
                        - ((double) mAvgUsedSize));

  // set erase only erases if found, so this is safe without key checking
  mGroupsOverAvg.erase(group_name);
  mGroupsUnderAvg.erase(group_name);

  eos_static_debug("diff=%.02f threshold=%.02f", diffWithAvg, mThreshold);

  // Group is mThreshold over or under the average used size
  if (abs(diffWithAvg) > mThreshold) {
    if (diffWithAvg > 0) {
      mGroupsOverAvg.emplace(group_name);
    } else {
      mGroupsUnderAvg.emplace(group_name);
    }
  }
}

void RandomBalancerEngine::updateGroupsAvg()
{
  mGroupsOverAvg.clear();
  mGroupsUnderAvg.clear();
  if (mGroupSizes.size() == 0) {
    return;
  }

  for(const auto& kv: mGroupSizes) {
    updateGroupAvg(kv.first);
  }

}

groups_picked_t
RandomBalancerEngine::pickGroupsforTransfer()
{
  if (mGroupsUnderAvg.size() == 0 || mGroupsOverAvg.size() == 0) {
    if (mGroupsOverAvg.size() == 0) {
      eos_static_debug("No groups over the average!");
    }

    if (mGroupsUnderAvg.size() == 0) {
      eos_static_debug("No groups under the average!");
    }

    recalculate();
    return {};
  }


  auto over_it = mGroupsOverAvg.begin();
  auto under_it = mGroupsUnderAvg.begin();
  int rndIndex = getRandom(mGroupsOverAvg.size() - 1);
  std::advance(over_it, rndIndex);
  rndIndex = getRandom(mGroupsUnderAvg.size() - 1);
  std::advance(under_it, rndIndex);
  return {*over_it, *under_it};
}

int RandomBalancerEngine::record_transfer(std::string_view source_group,
                                          std::string_view target_group,
                                          uint64_t filesize)
{
  auto source_grp = mGroupSizes.find(source_group);
  auto target_grp = mGroupSizes.find(target_group);

  if (source_grp == mGroupSizes.end() || target_grp == mGroupSizes.end()) {
    eos_static_err("Invalid source/target groups given!");
    return ENOENT;
  }

  source_grp->second.swapFile(&target_grp->second, filesize);
  return 0;
}

}
