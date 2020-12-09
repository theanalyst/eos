//------------------------------------------------------------------------------
//! @file RainBlock.cc
//! @author Elvin-Alin Sindrilaru - CERN
//------------------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2020 CERN/Switzerland                                  *
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

#include "fst/layout/rain/RainBlock.hh"
#include "fst/io/FileIo.hh"

namespace
{
eos::common::BufferManager gRainBuffMgr;
}

EOSFSTNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
RainBlock::RainBlock(FileIo* file, uint64_t offset, uint32_t capacity):
  mFile(file), mOffset(offset), mLastOffset(offset),
  mCapacity(capacity), mLength(0ull), mIsComplete(false)
{
  mBuffer = gRainBuffMgr.GetBuffer(mCapacity);
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
RainBlock::~RainBlock()
{
  gRainBuffMgr.Recycle(mBuffer);
}

//----------------------------------------------------------------------------
// Save data in the current block
//----------------------------------------------------------------------------
bool
RainBlock::PutData(const char* buffer, uint64_t offset, uint32_t length)
{
  if ((mOffset > offset) ||
      (mOffset > offset + length) ||
      ((mOffset + mCapacity) < offset) ||
      ((mOffset + mCapacity) < (offset + length))) {
    eos_static_err("msg=\"data does not belong to this block\" block_off=%llu "
                   "block_len=%llu data_off=%llu data_len=%llu", mOffset,
                   mCapacity, offset, length);
    return false;
  }

  // Check if this data covers any of the existing holes
  if (!mHoles.empty()) {
    FillHoles(offset, length);
  }

  // Add hole interval to the map
  if (offset > mLastOffset) {
    mHoles.emplace_back(mLastOffset, offset - 1);
  }

  if (offset + length > mLastOffset) {
    mLastOffset = offset + length;
    mLength = mLastOffset - mOffset;
  }

  if (mHoles.empty() && (mLastOffset == (mOffset + mCapacity))) {
    mIsComplete = true;
  }

  uint64_t block_off = offset - mOffset;
  char* ptr = mBuffer->GetDataPtr();
  ptr += block_off;
  (void) memcpy(ptr, buffer, length);
  return true;
}

//----------------------------------------------------------------------------
// Fill any existing holes given the current request
//----------------------------------------------------------------------------
void
RainBlock::FillHoles(uint64_t offset, uint32_t length)
{
  uint64_t end_offset = offset + length;
  std::list<std::pair<uint64_t, uint64_t>> new_holes;

  for (auto it = mHoles.begin(); it != mHoles.end(); /* empty */) {
    // Hole completely covered
    if ((it->first >= offset) && (it->second <= end_offset)) {
      it = mHoles.erase(it);
      continue;
    }

    // Filled in the middle -> creats two new holes
    if ((it->first < offset) && (it->second > end_offset)) {
      it = mHoles.erase(it);
      new_holes.emplace_back(it->first, offset - 1);
      new_holes.emplace_back(end_offset, it->second);
      continue;
    }

    // Partially filled in the beginning
    if ((it->first < offset) && (it->second > offset)) {
      it = mHoles.erase(it);
      new_holes.emplace_back(it->first, offset - 1);
      continue;
    }

    // Partially filled at the end
    if ((it->first < end_offset) && (it->second > end_offset)) {
      it = mHoles.erase(it);
      new_holes.emplace_back(end_offset, it->second);
      continue;
    }

    ++it;
  }

  mHoles.insert(mHoles.end(), new_holes.begin(), new_holes.end());
}

//----------------------------------------------------------------------------
// Fill the remaining part of the buffer with zeros and mark it as complete
//----------------------------------------------------------------------------
bool
RainBlock::CompleteWithZeros()
{
  if (!mHoles.empty()) {
    return false;
  }

  if (mLastOffset < mOffset + mCapacity) {
    uint64_t len = mOffset + mCapacity - mLastOffset;
    char* ptr = mBuffer->GetDataPtr();
    ptr += mLastOffset;
    (void) memset(ptr, '\0', len);
  }

  mIsComplete = true;
  return true;
}

//------------------------------------------------------------------------------
// Reset the block information
//------------------------------------------------------------------------------
void
RainBlock::Reset(FileIo* file, uint64_t offset)
{
  mHoles.clear();
  mIsComplete = false;
  mOffset = offset;
  mLastOffset = mOffset;
  mFile = file;
}

EOSFSTNAMESPACE_END
