#!/usr/bin/env bash

set -e

export CMAKE_FLAGS="$CMAKE_FLAGS -DCPACK_DIST=ubuntu.20.04"

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-deb.sh $1 $2
