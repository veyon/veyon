#!/usr/bin/env bash

set -e

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-deb.sh $1 $2 "debian-bullseye"
