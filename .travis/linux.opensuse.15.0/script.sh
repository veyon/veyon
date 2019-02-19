#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build
/veyon/.travis/common/finalize-rpm.sh "opensuse-15.0"
