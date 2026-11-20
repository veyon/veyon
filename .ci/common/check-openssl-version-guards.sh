#!/usr/bin/env bash
#
# check-openssl-version-guards.sh - enforce the VeyonCrypto rules of 06-pki.md §2 (for CI)
#
# Usage: .ci/common/check-openssl-version-guards.sh
#
#   1. OpenSSL version and feature checks (OPENSSL_VERSION_NUMBER/_MAJOR/_MINOR/_PREREQ,
#      OpenSSL_version_num, LIBRESSL_VERSION_NUMBER, OPENSSL_NO_*, __has_include(<openssl/...>))
#      only in crypto/src/OpenSslCompat*
#   2. all crypto/src/OpenSslCompat* files together stay within the budget of 150 lines
#   3. no OpenSSL headers in the public VeyonCrypto headers (pimpl; only the internal headers
#      OpenSslUtils.h and CryptoPrivate.h may include them)
#   4. no Veyon core headers (VeyonCore.h, Logger.h, ...) in crypto/: veyon-crypto does not depend
#      on veyon-core, so every "..." include there must name a header in crypto/src or next to the
#      including file
#
# Exit status 1 if any rule is violated.

set -e

cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

COMPAT_FILES='^crypto/src/OpenSslCompat[^/]*$'
COMPAT_BUDGET=150
INTERNAL_HEADERS='^crypto/src/(OpenSslUtils|CryptoPrivate)\.h$'
VERSION_PATTERN='OPENSSL_VERSION_NUMBER|OPENSSL_VERSION_MAJOR|OPENSSL_VERSION_MINOR|OPENSSL_VERSION_PREREQ|OpenSSL_version_num|LIBRESSL_VERSION_NUMBER|OPENSSL_NO_[A-Z0-9_]+|__has_include[[:space:]]*\([[:space:]]*<openssl/'

status=0

fail()
{
	echo "ERROR: $*" >&2
	status=1
}

# the greps below run in process substitutions whose exit status is lost, so make sure that the
# patterns are valid instead of letting a broken pattern pass silently
check_pattern()
{
	local result=0
	grep -E -q -e "$1" /dev/null || result=$?
	if [ $result -gt 1 ] ; then
		echo "ERROR: invalid pattern: $1" >&2
		exit 2
	fi
}

check_pattern "$VERSION_PATTERN"
check_pattern "$COMPAT_FILES"
check_pattern "$INTERNAL_HEADERS"

# first-party sources (submodules are not listed by git ls-files)
sources()
{
	git ls-files -- '*.c' '*.cpp' '*.h' '*.hpp' | grep -v -E '^3rdparty/'
}

# 1. version checks
while IFS= read -r match ; do
	fail "OpenSSL version check outside OpenSslCompat: $match"
done < <(sources | grep -v -E "$COMPAT_FILES" | xargs -r -d '\n' grep -n -H -E "$VERSION_PATTERN" || true)

# 2. compat budget
compatLines=$(git ls-files -- 'crypto/src/OpenSslCompat*' | xargs -r -d '\n' cat | wc -l)
if [ "$compatLines" -gt "$COMPAT_BUDGET" ] ; then
	fail "crypto/src/OpenSslCompat* has $compatLines lines (budget: $COMPAT_BUDGET)"
fi

# 3. OpenSSL in public headers
while IFS= read -r match ; do
	fail "OpenSSL header in public VeyonCrypto header: $match"
done < <(git ls-files -- 'crypto/src/*.h' | grep -v -E "$INTERNAL_HEADERS|$COMPAT_FILES" |
			 xargs -r -d '\n' grep -n -H -E '#[[:space:]]*include[[:space:]]*<openssl/' || true)

# 4. Veyon core headers in crypto/
while IFS= read -r source ; do
	directory=$(dirname "$source")
	while IFS= read -r header ; do
		if [[ "$header" == *..* ]] || { [ ! -f "crypto/src/$header" ] && [ ! -f "$directory/$header" ] ; } ; then
			fail "non-crypto header $header in $source (veyon-crypto must not depend on veyon-core)"
		fi
	done < <(sed -n -E 's/^[[:space:]]*#[[:space:]]*include[[:space:]]*"([^"]+)".*/\1/p' "$source")
done < <(git ls-files -- 'crypto/*.c' 'crypto/*.cpp' 'crypto/*.h' 'crypto/*.hpp')

if [ $status -eq 0 ] ; then
	echo "VeyonCrypto checks passed (OpenSslCompat: $compatLines/$COMPAT_BUDGET lines)"
fi

exit $status
