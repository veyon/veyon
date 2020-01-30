#!/usr/bin/env bash

source scl_source enable devtoolset-7

set -e

/veyon/.ci/common/linux-build.sh /veyon /build
/veyon/.ci/common/finalize-rpm.sh "centos-74"
