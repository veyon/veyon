#!/usr/bin/env bash
#
# check-openssl-version-guards.sh - enforce the VeyonCrypto rules of 06-pki.md §2 (for CI)
#
# Usage: .ci/common/check-openssl-version-guards.sh [<base-ref>]
#
#   1. OpenSSL version and feature checks (OPENSSL_VERSION_NUMBER/_MAJOR/_MINOR/_PREREQ,
#      OpenSSL_version_num, LIBRESSL_VERSION_NUMBER, OPENSSL_NO_*, __has_include(<openssl/...>))
#      only in core/src/crypto/OpenSslCompat*
#   2. all core/src/crypto/OpenSslCompat* files together stay within the budget of 150 lines
#   3. no QCA usage in core/src/crypto
#   4. no OpenSSL headers in the public VeyonCrypto headers (pimpl; only the internal headers
#      OpenSslUtils.h and CryptoPrivate.h may include them)
#   5. with <base-ref>: no QCA usage in lines added since <base-ref> (DoD 6, new crypto goes
#      through VeyonCrypto); legacy QCA code may still be changed in place
#
# Exit status 1 if any rule is violated.

set -e

cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

COMPAT_FILES='^core/src/crypto/OpenSslCompat[^/]*$'
COMPAT_BUDGET=150
INTERNAL_HEADERS='^core/src/crypto/(OpenSslUtils|CryptoPrivate)\.h$'
VERSION_PATTERN='OPENSSL_VERSION_NUMBER|OPENSSL_VERSION_MAJOR|OPENSSL_VERSION_MINOR|OPENSSL_VERSION_PREREQ|OpenSSL_version_num|LIBRESSL_VERSION_NUMBER|OPENSSL_NO_[A-Z0-9_]+|__has_include[[:space:]]*\([[:space:]]*<openssl/'
QCA_PATTERN='QtCrypto|QCA::|qca-qt|CryptoCore'

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
check_pattern "$QCA_PATTERN"
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
compatLines=$(git ls-files -- 'core/src/crypto/OpenSslCompat*' | xargs -r -d '\n' cat | wc -l)
if [ "$compatLines" -gt "$COMPAT_BUDGET" ] ; then
	fail "core/src/crypto/OpenSslCompat* has $compatLines lines (budget: $COMPAT_BUDGET)"
fi

# 3. QCA in the crypto layer
while IFS= read -r match ; do
	fail "QCA usage in core/src/crypto: $match"
done < <(git ls-files -- 'core/src/crypto' | xargs -r -d '\n' grep -n -H -E "$QCA_PATTERN" || true)

# 4. OpenSSL in public headers
while IFS= read -r match ; do
	fail "OpenSSL header in public VeyonCrypto header: $match"
done < <(git ls-files -- 'core/src/crypto/*.h' | grep -v -E "$INTERNAL_HEADERS|$COMPAT_FILES" |
			 xargs -r -d '\n' grep -n -H -E '#[[:space:]]*include[[:space:]]*<openssl/' || true)

# 5. new QCA usage
if [ -n "$1" ] ; then
	while IFS= read -r match ; do
		fail "new QCA usage (use VeyonCrypto): $match"
	done < <(git diff --unified=0 "$1" -- '*.c' '*.cpp' '*.h' '*.hpp' 'CMakeLists.txt' '*.cmake' |
				 grep -E '^\+[^+]' | grep -E "$QCA_PATTERN" || true)
fi

if [ $status -eq 0 ] ; then
	echo "VeyonCrypto checks passed (OpenSslCompat: $compatLines/$COMPAT_BUDGET lines)"
fi

exit $status
