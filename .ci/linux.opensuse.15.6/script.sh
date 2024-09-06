#!/usr/bin/env bash

set -e

export CMAKE_FLAGS="$CMAKE_FLAGS -DWITH_QT6=OFF -DCPACK_DIST=opensuse.15.6"

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-rpm.sh $1 $2
