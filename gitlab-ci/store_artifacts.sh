#!/bin/bash

#-------------------------------------------------------------------------------
# Publish artifacts from CERN Gitlab CI.
# The script will only upload artifacts from builds found in the buildmap.
#
# To add a new build type, register it in the buildmap together
# with the repo name at the storage endpoint.
#
# E.g: cc7 --> el-7
#      storage endpoint: /eos/project/s/storage-ci/www/eos/citrine/commit/el-7/
#-------------------------------------------------------------------------------
set -ex

# Define a mapping between builds and repos
declare -A BUILDMAP

BUILDMAP[cc7_no_sse]=el-7

BRANCH=$1 #BRANCH="citrine-no_sse"
BUILD_TYPE=$2
PATH_PREFIX=$3

for artifacts_dir in *_artifacts; do
  build=${artifacts_dir%_*} # build=cc7_no_sse
  repo=${BUILDMAP[${build}]} # repo=el-7

  # Handle only builds registered in the build map
  [ -z ${repo} ] && continue

  path=${PATH_PREFIX}/${BRANCH}/${BUILD_TYPE}/${repo}

  # Upload RPMS
  mkdir -p ${path}/x86_64/
  cp ${build}_artifacts/RPMS/* ${path}/x86_64/
  createrepo --update -q ${path}/x86_64/

  # Upload SRPMS
  mkdir -p ${path}/SRPMS/
  cp ${build}_artifacts/SRPMS/* ${path}/SRPMS/
  createrepo --update -q ${path}/SRPMS/
done

exit 0
