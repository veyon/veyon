#!/usr/bin/env bash

set -e

/veyon/.ci/common/linux-build.sh /veyon /build
/veyon/.ci/common/finalize-rpm.sh "centos-8.2"
