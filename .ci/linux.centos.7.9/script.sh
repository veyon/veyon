#!/usr/bin/env bash

source scl_source enable devtoolset-7

set -e

export CMAKE_FLAGS="$CMAKE_FLAGS -DCPACK_DIST=centos.7.9"

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-rpm.sh $1 $2
