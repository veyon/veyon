#!/bin/bash
# verify-bundle-paths.sh - Verify that no hardcoded paths remain in the bundle

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [[ $# -lt 1 ]]; then
	echo "Usage: $0 <path-to-app-bundle>"
	exit 1
fi

APP_BUNDLE="$1"

if [[ ! -d "$APP_BUNDLE" ]]; then
	echo -e "${RED}Error: $APP_BUNDLE is not a directory${NC}"
	exit 1
fi

echo -e "${GREEN}=== Verifying bundle: $APP_BUNDLE ===${NC}"
echo ""

found_issues=0

# Check all Mach-O files for hardcoded paths
while IFS= read -r -d '' file; do
	if file "$file" 2>/dev/null | grep -q "Mach-O"; then
		# Check for /usr/local paths (excluding /usr/local/lib/libz.1.dylib which is system)
		if otool -L "$file" 2>/dev/null | grep -E "/(usr/local|opt/local|Cellar)" | grep -v "compatibility version" > /dev/null; then
			if [[ $found_issues -eq 0 ]]; then
				echo -e "${YELLOW}Found hardcoded paths:${NC}"
				echo ""
			fi
			echo -e "${RED}⚠️  $(basename "$file"):${NC}"
			otool -L "$file" 2>/dev/null | grep -E "/(usr/local|opt/local|Cellar)" | sed 's/^/    /'
			echo ""
			found_issues=$((found_issues + 1))
		fi
	fi
done < <(find "$APP_BUNDLE/Contents" -type f -print0)

if [[ $found_issues -eq 0 ]]; then
	echo -e "${GREEN}✓ No hardcoded paths found!${NC}"
	echo -e "${GREEN}✓ Bundle is portable${NC}"
	exit 0
else
	echo -e "${RED}✗ Found $found_issues file(s) with hardcoded paths${NC}"
	echo -e "${YELLOW}Run the packaging script again to fix these issues${NC}"
	exit 1
fi
