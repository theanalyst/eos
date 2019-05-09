// ----------------------------------------------------------------------
// File: AutoRepair.cc
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

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


/*----------------------------------------------------------------------------*/
#include "common/AutoRepair.hh"
#include "common/StringConversion.hh"
#include <map>
/*----------------------------------------------------------------------------*/

EOSCOMMONNAMESPACE_BEGIN

int
/*----------------------------------------------------------------------------*/
AutoRepair::Parse(const std::string& repair)
{
  std::map<std::string, std::string> arconfig;
  eos::common::StringConversion::GetKeyValueMap(
						repair.c_str(),
						arconfig);
  Reset();

  if (arconfig.size() != 4) {
    return EINVAL;
  }

  if (arconfig["posc"] == "1") set_posc();
  if (arconfig["dropall"] == "1") set_dropall();
  if (arconfig["drop"] == "1") set_drop();
  if (arconfig["scan"] == "1") set_scan();
  
  for (auto it = arconfig.begin(); it != arconfig.end(); ++it) {
    if ( (it->second != "0") && 
	 (it->second != "1")  )
      return EINVAL;

    if ( (it->first != "posc") && 
	 (it->first != "dropall") &&
	 (it->first != "drop") &&
	 (it->first != "scan") ) {
      return EINVAL;
    }
  }
  return 0;
}

/*----------------------------------------------------------------------------*/
const char*
AutoRepair::Usage()
{
  return "posc:0|1,dropall:0|1,drop:0|1,scan:0|1";
}

/*----------------------------------------------------------------------------*/

EOSCOMMONNAMESPACE_END

