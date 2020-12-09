//------------------------------------------------------------------------------
//! @file RainBlock.hh
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

#pragma once
#include "fst/Namespace.hh"
#include "common/Logging.hh"
#include "common/BufferManager.hh"
#include <vector>
#include <list>
#include <cstdint>

EOSFSTNAMESPACE_BEGIN

//! Forward declarations
class FileIo;

//------------------------------------------------------------------------------
//! Class RainBlock
//------------------------------------------------------------------------------
class RainBlock: public eos::common::LogId
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //!
  //! @param file attached file object where this block is written
  //! @param offset actual offset in the attached file
  //! @param capacity maximum size of the current block
  //----------------------------------------------------------------------------
  RainBlock(FileIo* file, uint64_t offset, uint32_t capacity);

  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  ~RainBlock() final;

  //----------------------------------------------------------------------------
  //! Put data in the current block
  //!
  //! @param buffer buffer containing the data
  //! @param offset offset in the attached file
  //! @param length lenght of the data
  //!
  //! @return true if successful, otherwise false
  //----------------------------------------------------------------------------
  bool PutData(const char* buffer, uint64_t offset, uint32_t lenght);

  //----------------------------------------------------------------------------
  //! Fill any existing holes in the current block given the new request
  //!
  //! @param offset offset of the new request
  //! @param length length of the new request
  //----------------------------------------------------------------------------
  void FillHoles(uint64_t offset, uint32_t length);

  //----------------------------------------------------------------------------
  //! Fill the remaining (unused) part of the buffer with zeros and mark it
  //! as complete
  //!
  //! @retrun true if successful, otherwise false
  //----------------------------------------------------------------------------
  bool CompleteWithZeros();

  //----------------------------------------------------------------------------
  //! Reset the block information
  //!
  //! @param file new file object to which current block is attached
  //! @param offset new offset of the block withing the new file
  //----------------------------------------------------------------------------
  void Reset(FileIo* file, uint64_t offset);

  //----------------------------------------------------------------------------
  //! Get pointer to the undelying data
  //----------------------------------------------------------------------------
  inline const char* GetDataPtr() const
  {
    return mBuffer->GetDataPtr();
  }

  //----------------------------------------------------------------------------
  //! Get length of relevant data in the current block
  //----------------------------------------------------------------------------
  inline uint32_t GetLength() const
  {
    return mLength;
  }

  //----------------------------------------------------------------------------
  //! Get offset of the current block in relation to the file it belongs to
  //----------------------------------------------------------------------------
  inline uint64_t GetOffset() const
  {
    return mOffset;
  }

  //----------------------------------------------------------------------------
  //! Check if block is complete
  //----------------------------------------------------------------------------
  inline bool IsComplete() const
  {
    return mIsComplete;
  }

  //----------------------------------------------------------------------------
  //! Get list of "holes" in the current block
  //----------------------------------------------------------------------------
  inline std::list<std::pair<uint64_t, uint64_t>> GetListHoles() const
  {
    return mHoles;
  }

private:
#ifdef IN_TEST_HARNESS
public:
#endif
  eos::fst::FileIo* mFile; ///< File to which this block corresponds
  uint64_t mOffset; ///< Actual offset in the file where this blocks begins
  uint64_t mLastOffset; ///< Last written offset
  uint32_t mCapacity; ///< Max size of the current block
  uint32_t mLength; ///< Length of useful data, relevant if no holes
  bool mIsComplete; ///< Flag to mark that block is full
  //! Map of holes in the block if writing was not in streaming mode
  std::list<std::pair<uint64_t, uint64_t>> mHoles;
  std::shared_ptr<eos::common::Buffer> mBuffer; ///< Actual data buffer
};

EOSFSTNAMESPACE_END
