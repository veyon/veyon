#!/usr/bin/env bash
#
# run-clang-format.sh - format (or check) all first-party C/C++ sources
#
# Usage: .ci/common/run-clang-format.sh [--check] [--list]
#
#   (no option)  reformat all first-party sources in place
#   --check      do not modify files, exit with status 1 if any file is not
#                formatted according to .clang-format (for CI)
#   --list       only print the list of files that are subject to formatting
#
# The clang-format binary can be overridden via the CLANG_FORMAT environment
# variable (default: clang-format-20 if available, otherwise clang-format).

set -e

cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

MODE=format
case "$1" in
	--check) MODE=check ;;
	--list) MODE=list ;;
	"") ;;
	*) echo "Usage: $0 [--check|--list]" >&2 ; exit 2 ;;
esac

if [ -z "$CLANG_FORMAT" ] ; then
	if command -v clang-format-20 > /dev/null ; then
		CLANG_FORMAT=clang-format-20
	else
		CLANG_FORMAT=clang-format
	fi
fi

# First-party C/C++ sources tracked by git. Submodules (3rdparty/*) are not
# listed by git ls-files, the remaining exclusions are third-party or
# generated files living inside first-party directories.
list_files()
{
	git ls-files -- '*.c' '*.cpp' '*.h' | grep -v -E \
		-e '^3rdparty/' \
		-e '^core/src/d3des\.(c|h)$' \
		-e '^core/src/Toast\.(cpp|h)$' \
		-e '^master/src/KExtraColumnsProxyModel\.(cpp|h)$' \
		-e '^plugins/platform/windows/XEventLog\.(cpp|h)$' \
		-e '^plugins/platform/windows/netfw\.h$' \
		-e '^plugins/vncserver/x11vnc-builtin/x11vnc-veyon\.c$'
}

if [ "$MODE" = "list" ] ; then
	list_files
	exit 0
fi

if [ "$MODE" = "check" ] ; then
	list_files | xargs -r -d '\n' "$CLANG_FORMAT" --dry-run --Werror
else
	list_files | xargs -r -d '\n' "$CLANG_FORMAT" -i
fi
