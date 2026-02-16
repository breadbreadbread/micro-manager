#!/bin/bash
#
# Comprehensive test script for Spinnaker SDK and adapter build
# Run this to verify your environment is ready for building Micro-Manager with Spinnaker support
#

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=========================================================================${NC}"
echo -e "${BLUE}Spinnaker SDK Environment Test${NC}"
echo -e "${BLUE}=========================================================================${NC}"
echo ""

# Configuration
SPINNAKER_DIR="${SPINNAKER_DIR:-/opt/spinnaker}"
SWIG_DIR="${SWIG_DIR:-/tmp/swig-3.0.12-install}"

# Test counters
PASSED=0
FAILED=0
WARNINGS=0

# Test function
test_item() {
    local name="$1"
    local test_cmd="$2"
    local expected="$3"

    echo -n "Testing $name... "

    if eval "$test_cmd"; then
        echo -e "${GREEN}✓ PASS${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  Expected: $expected"
        ((FAILED++))
        return 1
    fi
}

# Warning function
test_warning() {
    local name="$1"
    local test_cmd="$2"
    local message="$3"

    echo -n "Testing $name... "

    if eval "$test_cmd"; then
        echo -e "${YELLOW}⚠ WARNING${NC}"
        echo "  $message"
        ((WARNINGS++))
        return 0
    else
        echo -e "${GREEN}✓ PASS${NC}"
        ((PASSED++))
        return 1
    fi
}

echo -e "${BLUE}Step 1: System Architecture${NC}"
echo ""

# Test ARM64 architecture
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    echo -e "${GREEN}✓ System is ARM64 (Apple Silicon)${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠ System is $ARCH (not ARM64)${NC}"
    echo "  Spinnaker SDK 4.x.x is optimized for ARM64"
    ((WARNINGS++))
fi

echo ""
echo -e "${BLUE}Step 2: Spinnaker SDK${NC}"
echo ""

# Test SDK directory
test_item "Spinnaker SDK directory exists" \
    "[ -d '$SPINNAKER_DIR' ]" \
    "Directory at $SPINNAKER_DIR"

# Test headers
test_item "Spinnaker.h header exists" \
    "[ -f '$SPINNAKER_DIR/include/Spinnaker.h' ]" \
    "$SPINNAKER_DIR/include/Spinnaker.h"

# Test library
test_item "libSpinnaker.dylib exists" \
    "[ -f '$SPINNAKER_DIR/lib/libSpinnaker.dylib' ]" \
    "$SPINNAKER_DIR/lib/libSpinnaker.dylib"

# Test library architecture
echo -n "Testing Spinnaker library architecture... "
LIB_ARCH=$(file "$SPINNAKER_DIR/lib/libSpinnaker.dylib" 2>/dev/null | grep -o 'arm64\|x86_64' || echo "unknown")
if [ "$LIB_ARCH" = "arm64" ]; then
    echo -e "${GREEN}✓ PASS${NC} (ARM64)"
    ((PASSED++))
elif [ "$LIB_ARCH" = "x86_64" ]; then
    echo -e "${YELLOW}⚠ WARNING${NC} (x86_64, should be ARM64)"
    echo "  SDK may not work correctly on Apple Silicon"
    ((WARNINGS++))
else
    echo -e "${RED}✗ FAIL${NC} ($LIB_ARCH)"
    ((FAILED++))
fi

# Test camera detection
echo -n "Testing camera detection (SpinListCameras)... "
if [ -x "$SPINNAKER_DIR/bin/SpinListCameras" ]; then
    CAMERA_COUNT=$("$SPINNAKER_DIR/bin/SpinListCameras" 2>&1 | grep -c "Camera" || echo "0")
    if [ "$CAMERA_COUNT" -gt 0 ]; then
        echo -e "${GREEN}✓ PASS${NC} (found $CAMERA_COUNT camera(s))"
        ((PASSED++))
    else
        echo -e "${YELLOW}⚠ WARNING${NC} (no cameras detected)"
        echo "  Camera may not be connected or recognized"
        ((WARNINGS++))
    fi
else
    echo -e "${YELLOW}⚠ SKIPPED${NC} (tool not found)"
    ((WARNINGS++))
fi

echo ""
echo -e "${BLUE}Step 3: Build Tools${NC}"
echo ""

# Test SWIG
echo -n "Testing SWIG 3.0.12... "
if [ -x "$SWIG_DIR/bin/swig" ]; then
    SWIG_VER=$("$SWIG_DIR/bin/swig" -version 2>&1 | grep -o 'SWIG Version [0-9]\.[0-9]\.[0-9]')
    echo "found $SWIG_VER"

    # Check version
    if [[ "$SWIG_VER" =~ SWIG\ Version\ 3\.0\.[0-9] ]]; then
        echo -e "${GREEN}✓ PASS${NC} (version 3.0.x)"
        ((PASSED++))
    else
        echo -e "${YELLOW}⚠ WARNING${NC} (version not 3.0.x)"
        echo "  Micro-Manager requires SWIG 3.0.x"
        ((WARNINGS++))
    fi
else
    echo -e "${RED}✗ FAIL${NC} (not found at $SWIG_DIR)"
    ((FAILED++))
fi

# Test Java
test_item "Java found" \
    "command -v java" \
    "Java runtime"

test_item "Java version 11" \
    "java -version 2>&1 | grep -q 'version \"11'" \
    "Java 11.x"

# Get Java version
if command -v java &>/dev/null; then
    JAVA_VER=$(java -version 2>&1 | head -n 1 | cut -d'"' -f2 | cut -d'.' -f1-2)
    echo -e "  Java version: $JAVA_VER"
fi

# Test other build tools
test_item "autoconf found" \
    "command -v autoconf" \
    "autoconf"

test_item "automake found" \
    "command -v automake" \
    "automake"

test_item "libtool found" \
    "command -v libtool" \
    "libtool"

test_item "ant found" \
    "command -v ant" \
    "ant"

test_item "g++ found" \
    "command -v g++" \
    "g++ compiler"

echo ""
echo -e "${BLUE}Step 4: Micro-Manager Source${NC}"
echo ""

# Check if we're in Micro-Manager directory
MM_DIR="${1:-$(pwd)}"
echo -n "Testing Micro-Manager source directory... "
if [ -f "$MM_DIR/configure.ac" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
    echo "  Directory: $MM_DIR"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL${NC}"
    echo "  configure.ac not found in: $MM_DIR"
    echo "  Please run from Micro-Manager source directory"
    ((FAILED++))
fi

# Check for submodules
echo -n "Testing mmCoreAndDevices submodule... "
if [ -d "$MM_DIR/mmCoreAndDevices" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL${NC}"
    echo "  Submodule not initialized"
    echo "  Run: git submodule update --init --recursive"
    ((FAILED++))
fi

# Check for Spinnaker adapter template
echo -n "Testing Spinnaker4 adapter template... "
if [ -d "$MM_DIR/SPINNAKER_ADAPTER_TEMPLATE" ] || \
   [ -d "$MM_DIR/mmCoreAndDevices/DeviceAdapters/Spinnaker4" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (not found)"
    echo "  Copy SPINNAKER_ADAPTER_TEMPLATE/ to mmCoreAndDevices/DeviceAdapters/Spinnaker4/"
    ((WARNINGS++))
fi

echo ""
echo -e "${BLUE}Step 5: Environment Variables${NC}"
echo ""

# Check SWIG environment
echo -n "Testing SWIG environment variable... "
if [ -n "$SWIG" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
    echo "  SWIG=$SWIG"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (not set)"
    echo "  Set with: export SWIG=/path/to/swig"
    ((WARNINGS++))
fi

# Check JAVA_HOME
echo -n "Testing JAVA_HOME environment variable... "
if [ -n "$JAVA_HOME" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
    echo "  JAVA_HOME=$JAVA_HOME"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠ WARNING${NC} (not set)"
    echo "  Set with: export JAVA_HOME=/path/to/java"
    ((WARNINGS++))
fi

echo ""
echo -e "${BLUE}=========================================================================${NC}"
echo -e "${BLUE}Test Results Summary${NC}"
echo -e "${BLUE}=========================================================================${NC}"
echo ""

echo -e "${GREEN}Passed:  $PASSED${NC}"
echo -e "${YELLOW}Warnings: $WARNINGS${NC}"
echo -e "${RED}Failed:  $FAILED${NC}"
echo ""

# Overall assessment
if [ $FAILED -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed! Environment is ready.${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Apply patches:"
    echo "   cd $MM_DIR"
    echo "   patch -p1 < patches/disable_blueboxoptics_niji.patch"
    echo "   patch -p1 < patches/add_spinnaker4_support.patch"
    echo ""
    echo "2. Build Micro-Manager:"
    echo "   ./autogen.sh"
    echo "   ./configure --prefix=/usr/local --with-java=\$JAVA_HOME --with-spinnaker=$SPINNAKER_DIR"
    echo "   make -j\$(sysctl -n hw.ncpu)"
    echo ""
    exit 0
elif [ $FAILED -eq 0 ]; then
    echo -e "${YELLOW}⚠ Environment is ready with some warnings.${NC}"
    echo ""
    echo "Review warnings above and decide if you want to proceed."
    echo ""
    echo "Next steps (if you want to proceed):"
    echo "1. Apply patches"
    echo "2. Build Micro-Manager"
    echo ""
    exit 0
else
    echo -e "${RED}✗ Some tests failed. Environment needs fixing.${NC}"
    echo ""
    echo "Fix failed items above before building."
    echo ""
    exit 1
fi
