#!/usr/bin/env bash

set -e

/veyon/.ci/common/linux-build.sh /veyon /build
/veyon/.ci/common/finalize-rpm.sh "opensuse-15.2"
