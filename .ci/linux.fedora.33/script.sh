#!/usr/bin/env bash

set -e

export CMAKE_FLAGS="-DWITH_PCH=OFF"

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-rpm.sh $1 $2 "fc31"
