#-------------------------------------------------------------------------------
# File: CMakeLists.txt
# Author: Abhishek Lekshmanan <abhishek.l@cern.ch> CERN
#-------------------------------------------------------------------------------

# ************************************************************************
# * EOS - the CERN Disk Storage System                                   *
# * Copyright (C) 2017 CERN/Switzerland                                  *
# *                                                                      *
# * This program is free software: you can redistribute it and/or modify *
# * it under the terms of the GNU General Public License as published by *
# * the Free Software Foundation, either version 3 of the License, or    *
# * (at your option) any later version.                                  *
# *                                                                      *
# * This program is distributed in the hope that it will be useful,      *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
# * GNU General Public License for more details.                         *
# *                                                                      *
# * You should have received a copy of the GNU General Public License    *
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
# ************************************************************************

# We use Benchmark as an external project, instead of add_subdirectory - we
# still use submodules, but this allows for customizations in build args, this
# is necessary as we define RELEASE to be a macro pointing to an eos release
# whereas benchmark libary has its internal RELEASE definition which means
# something else, also allows, if we didn't need to customize build options, we
# could just use FetchProject which directly exports the necessary
# benchmark::benchmark cmake target, however with ExternalProject this work is
# largely manual
include(ExternalProject)
set(BENCHMARK_CMAKE_CXX_FLAGS "-std=c++17")
set(BENCHMARK_CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_CXX_FLAGS=${BENCHMARK_CMAKE_CXX_FLAGS}"
        "-DBENCHMARK_ENABLE_TESTING=OFF")

ExternalProject_Add(googlebenchmark
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/test/microbenchmarks/benchmark
  CMAKE_ARGS "${BENCHMARK_CMAKE_ARGS}"
  INSTALL_COMMAND ""
  LOG_CONFIGURE ON
  LOG_BUILD ON)

ExternalProject_Get_Property(googlebenchmark source_dir)
find_path(BENCHMARK_INCLUDE_DIRS
  NAMES benchmark/benchmark.h
  PATHS ${source_dir}/include)

ExternalProject_Get_Property(googlebenchmark binary_dir)
message(STATUS "Setting benchmark binary_dir ${binary_dir}")
set(BENCHMARK_LIBRARY_PATH ${binary_dir}/src/libbenchmark.a)
set(BENCHMARK_LIBRARY benchmark::benchmark)
add_library(${BENCHMARK_LIBRARY} STATIC IMPORTED)
set_target_properties(${BENCHMARK_LIBRARY} PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${BENCHMARK_INCLUDE_DIRS}"
  IMPORTED_LOCATION ${BENCHMARK_LIBRARY_PATH})
add_dependencies(${BENCHMARK_LIBRARY} googlebenchmark)

set(BM_COMMON_SRCS
  common/BM_StringUtils.cc)

set(EOS_BM_SRCS
  ${BM_COMMON_SRCS})

add_executable(eos-microbenchmarks ${EOS_BM_SRCS})
add_dependencies(eos-microbenchmarks ${BENCHMARK_LIBRARY})
target_link_libraries(eos-microbenchmarks PRIVATE
 benchmark::benchmark)
