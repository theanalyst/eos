//------------------------------------------------------------------------------
// File: BMThreadMacros.hh
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

#pragma once

namespace traits {

template <typename T>
struct has_thread_index {
  template <typename U>
  static auto check_attr(U*) -> decltype(std::declval<U>().thread_index,
                                         std::true_type());

  template <typename V>
  static auto check_fn(V*) -> decltype(std::declval<V>().thread_index(),
                                       std::true_type());

  template <typename>
  static std::false_type check_attr(...);

  template <typename>
  static std::false_type check_fn(...);

  static constexpr bool attr_value = decltype(check_attr<T>(nullptr))::value;
  static constexpr bool fn_value = decltype(check_fn<T>(nullptr))::value;
};

// Helper for the last branch of a constexpr_if, deny everything!
template<class>
inline constexpr bool dependent_false_v = false;

}

/*
 * Helper fn to help determine thread starting. Since the API has changed
 * between versions 1.55 shipped in centos7 and alma8/9, determine which variant
 * to call using the compiler.
 */
template <typename BM_State>
bool BM_THREAD_INDEX(BM_State& state) {
  if constexpr (traits::has_thread_index<BM_State>::attr_value) {
    return state.thread_index == 0;
  } else if constexpr(traits::has_thread_index<BM_State>::fn_value) {
    return state.thread_index() == 0;
  } else {
    // The discarded statement can't be ill formed, so just a generic false!
    static_assert(traits::dependent_false_v<BM_State>,
                  "This Benchmark library has no multithreading support!");
  }

}
