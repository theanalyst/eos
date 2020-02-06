// ----------------------------------------------------------------------
// File: SmartSpaceConfig.cc
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

#include "common/Logging.hh"
#include "mgm/tgc/Constants.hh"
#include "mgm/tgc/SmartSpaceConfig.hh"

EOSTGCNAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
SmartSpaceConfig::SmartSpaceConfig(
  ITapeGcMgm &mgm,
  const std::string &spaceName,
  const std::time_t maxConfigCacheAgeSecs):
  m_config(std::bind(&ITapeGcMgm::getTapeGcSpaceConfig, &mgm, spaceName), maxConfigCacheAgeSecs)
{
}

//------------------------------------------------------------------------------
// Returns the configuration
//------------------------------------------------------------------------------
SpaceConfig
SmartSpaceConfig::get() const
{
  const auto config = m_config.get();

  if (config.prev.queryPeriodSecs != config.current.queryPeriodSecs) {
    std::ostringstream msg;
    msg << "msg=\"" << TGC_NAME_QRY_PERIOD_SECS << " has been changed from " <<
      config.prev.queryPeriodSecs  << " to " << config.current.queryPeriodSecs << "\"";
    eos_static_info(msg.str().c_str());
  }
  if (config.prev.minFreeBytes != config.current.minFreeBytes) {
    std::ostringstream msg;
    msg << "msg=\"" << TGC_NAME_MIN_FREE_BYTES << " has been changed from " << config.prev.minFreeBytes  << " to " <<
      config.current.minFreeBytes << "\"";
    eos_static_info(msg.str().c_str());
  }
  if (config.prev.minUsedBytes != config.current.minUsedBytes) {
    std::ostringstream msg;
    msg << "msg=\"" << TGC_NAME_MIN_USED_BYTES << " has been changed from " << config.prev.minUsedBytes  << " to " <<
      config.current.minUsedBytes << "\"";
    eos_static_info(msg.str().c_str());
  }

  return config.current;
}

EOSTGCNAMESPACE_END
