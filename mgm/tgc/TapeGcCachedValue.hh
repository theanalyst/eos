// ----------------------------------------------------------------------
// File: TapeGcCachedValue.hh
// Author: Steven Murray - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
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

#ifndef __EOSMGM_TAPEGCCACHEDVALUE_HH__
#define __EOSMGM_TAPEGCCACHEDVALUE_HH__

#include "mgm/Namespace.hh"

#include <functional>
#include <time.h>

/*----------------------------------------------------------------------------*/
/**
 * @file TapeAwareGcCachedValue.hh
 *
 * @brief Templated class for creating a time based cache for a single
 * variable.
 *
 */
/*----------------------------------------------------------------------------*/
EOSMGMNAMESPACE_BEGIN

//----------------------------------------------------------------------------
//! Templated class for creating a time based cache for a single variable.
//!
//! @tparam ValueType The type of the value to be cached.
//----------------------------------------------------------------------------
template <typename ValueType> class TapeGcCachedValue {
public:
  //--------------------------------------------------------------------------
  //! Constructor
  //! @param valueGetter callable responsible for getting a new value
  //! @param maxAgeSecs age at which a call to get() will renew the cache
  //--------------------------------------------------------------------------
  TapeGcCachedValue(std::function<ValueType()> valueGetter, const time_t maxAgeSecs):
    m_valueHasNeverBeenSet(true),
    m_valueGetter(valueGetter),
    m_maxAgeSecs(maxAgeSecs),
    m_timestamp(time(nullptr))
  {
  }

  //--------------------------------------------------------------------------
  //! @param valueChanged out parameter that is set to true if the cached
  //! value has changed
  //! @return the cached value.
  //--------------------------------------------------------------------------
  ValueType
  get(bool &valueChanged)
  {
    const time_t now = time(nullptr);
    const time_t age = now - m_timestamp;

    if(m_valueHasNeverBeenSet || age >= m_maxAgeSecs) {
      m_valueHasNeverBeenSet = false;
      m_timestamp = now;
      const ValueType newValue = m_valueGetter();
      valueChanged = newValue != m_value;
      m_value = newValue;
    }

    return m_value;
  }

private:
  /// True if the cached value has never been set
  bool m_valueHasNeverBeenSet;

  /// The cached value
  ValueType m_value;

  /// Callable responsible for getting a new value
  std::function<ValueType()> m_valueGetter;

  /// Age at which a call to get() will renew the cache
  time_t m_maxAgeSecs;

  /// The timestamp of when the value was last updated
  time_t m_timestamp;
}; // class TapeGcCachedValue

EOSMGMNAMESPACE_END

#endif
