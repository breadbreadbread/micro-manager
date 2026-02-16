#!/bin/bash
#
# Build Micro-Manager with Spinnaker SDK support on macOS Apple Silicon
# This script handles common build issues and provides a streamlined build process
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${1:-$(pwd)}"
SPINNAKER_DIR="${SPINNAKER_DIR:-/opt/spinnaker}"
SWIG_DIR="${SWIG_DIR:-/tmp/swig-3.0.12-install}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

echo -e "${BLUE}=========================================================================${NC}"
echo -e "${BLUE}Micro-Manager Build Script for macOS Apple Silicon${NC}"
echo -e "${BLUE}=========================================================================${NC}"
echo ""
echo "Build directory: $BUILD_DIR"
echo "Spinnaker SDK:   $SPINNAKER_DIR"
echo "SWIG 3.0.12:     $SWIG_DIR"
echo "Install prefix:  $INSTALL_PREFIX"
echo ""

# Check if we're in the right directory
if [ ! -f "$BUILD_DIR/configure.ac" ]; then
    echo -e "${RED}Error: Not in Micro-Manager source directory${NC}"
    echo "Expected to find configure.ac in: $BUILD_DIR"
    exit 1
fi

# Step 1: Check prerequisites
echo -e "${GREEN}Step 1: Checking prerequisites...${NC}"
echo ""

# Check SWIG
if [ ! -f "$SWIG_DIR/bin/swig" ]; then
    echo -e "${RED}Error: SWIG 3.0.12 not found at $SWIG_DIR${NC}"
    echo "Please build SWIG 3.0.12 first:"
    echo ""
    echo "  cd /tmp"
    echo "  curl -LO https://prdownloads.sourceforge.net/swig/swig-3.0.12.tar.gz"
    echo "  tar xzf swig-3.0.12.tar.gz"
    echo "  cd swig-3.0.12"
    echo "  ./configure --prefix=$SWIG_DIR"
    echo "  make -j\$(sysctl -n hw.ncpu)"
    echo "  sudo make install"
    echo ""
    echo "Then re-run this script with:"
    echo "  SWIG_DIR=$SWIG_DIR $0 $BUILD_DIR"
    exit 1
fi
echo -e "${GREEN}✓ SWIG 3.0.12 found${NC}"

# Check Spinnaker SDK
if [ ! -f "$SPINNAKER_DIR/include/Spinnaker.h" ]; then
    echo -e "${YELLOW}Warning: Spinnaker SDK not found at $SPINNAKER_DIR${NC}"
    echo "Spinnaker support will not be built."
    SPINNAKER_AVAILABLE=0
else
    echo -e "${GREEN}✓ Spinnaker SDK found${NC}"
    SPINNAKER_AVAILABLE=1
fi

# Check Java
if ! command -v java &> /dev/null; then
    echo -e "${RED}Error: Java not found${NC}"
    echo "Please install Java JDK 11:"
    echo "  brew install --cask temurin@11"
    exit 1
fi
JAVA_VERSION=$(java -version 2>&1 | head -n 1 | cut -d'"' -f2 | cut -d'.' -f1-2)
echo -e "${GREEN}✓ Java found (version $JAVA_VERSION)${NC}"

# Check required tools
REQUIRED_TOOLS=(autoconf automake libtool ant g++)
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}Error: $tool not found${NC}"
        echo "Please install: brew install $tool"
        exit 1
    fi
    echo -e "${GREEN}✓ $tool found${NC}"
done

echo ""
echo -e "${GREEN}All prerequisites checked${NC}"
echo ""

# Step 2: Disable problematic adapters
echo -e "${GREEN}Step 2: Disabling problematic adapters...${NC}"
echo ""

PROBLEMATIC_ADAPTERS=(
    "BlueboxOptics_niji"
)

for adapter in "${PROBLEMATIC_ADAPTERS[@]}"; do
    echo "Checking $adapter..."
    MAKEFILE="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/$adapter/Makefile.am"

    if [ -f "$MAKEFILE" ]; then
        # Add DISABLED marker
        if ! grep -q "^# DISABLED:" "$MAKEFILE"; then
            sed -i.bak "1s/^/# DISABLED: Known build issue on macOS ARM64\n/" "$MAKEFILE"
            rm -f "${MAKEFILE}.bak"
            echo -e "${GREEN}  Disabled $adapter${NC}"
        else
            echo "  Already disabled"
        fi
    fi

    # Remove from SUBDIRS
    MAIN_MAKEFILE="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/Makefile.am"
    if [ -f "$MAIN_MAKEFILE" ] && grep -q "$adapter" "$MAIN_MAKEFILE"; then
        sed -i.bak "s/\($adapter\)/#\1 # DISABLED/" "$MAIN_MAKEFILE"
        rm -f "${MAIN_MAKEFILE}.bak"
        echo -e "${GREEN}  Removed $adapter from SUBDIRS${NC}"
    fi
done

echo ""
echo -e "${GREEN}Problematic adapters disabled${NC}"
echo ""

# Step 3: Add Spinnaker adapter if template exists
echo -e "${GREEN}Step 3: Checking for Spinnaker adapter template...${NC}"
echo ""

SPINNAKER_TEMPLATE="$BUILD_DIR/SPINNAKER_ADAPTER_TEMPLATE"

if [ -d "$SPINNAKER_TEMPLATE" ]; then
    SPINNAKER_DEST="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/Spinnaker4"

    if [ ! -d "$SPINNAKER_DEST" ]; then
        echo "Copying Spinnaker adapter template..."
        mkdir -p "$SPINNAKER_DEST"
        cp -r "$SPINNAKER_TEMPLATE"/* "$SPINNAKER_DEST/"
        echo -e "${GREEN}✓ Spinnaker adapter copied${NC}"
    else
        echo "Spinnaker adapter already exists"
    fi
else
    echo "Spinnaker adapter template not found (optional for now)"
fi

echo ""

# Step 4: Set environment
echo -e "${GREEN}Step 4: Setting build environment...${NC}"
echo ""

export SWIG="$SWIG_DIR/bin/swig"
export JAVA_HOME=$(/usr/libexec/java_home -v 11)

echo "SWIG=$SWIG"
echo "JAVA_HOME=$JAVA_HOME"
echo ""

# Step 5: Run autogen
echo -e "${GREEN}Step 5: Generating configure script...${NC}"
echo ""

cd "$BUILD_DIR"
if [ ! -f "configure" ] || [ "configure.ac" -nt "configure" ]; then
    ./autogen.sh
    echo -e "${GREEN}✓ Configure script generated${NC}"
else
    echo "Configure script already up to date"
fi

echo ""

# Step 6: Configure
echo -e "${GREEN}Step 6: Configuring build...${NC}"
echo ""

CONFIGURE_CMD="./configure \
    --prefix=$INSTALL_PREFIX \
    --with-java=$JAVA_HOME"

if [ $SPINNAKER_AVAILABLE -eq 1 ]; then
    CONFIGURE_CMD="$CONFIGURE_CMD --with-spinnaker=$SPINNAKER_DIR"
    echo "Building with Spinnaker SDK support"
else
    echo "Building without Spinnaker SDK"
fi

echo "Running: $CONFIGURE_CMD"
echo ""

$CONFIGURE_CMD

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Configure successful${NC}"
else
    echo -e "${RED}✗ Configure failed${NC}"
    exit 1
fi

echo ""

# Step 7: Build dependencies
echo -e "${GREEN}Step 7: Fetching dependencies...${NC}"
echo ""

make fetchdeps

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Dependencies fetched${NC}"
else
    echo -e "${YELLOW}Warning: Some dependencies failed (may be non-critical)${NC}"
fi

echo ""

# Step 8: Build
echo -e "${GREEN}Step 8: Building Micro-Manager...${NC}"
echo ""

NUM_CPUS=$(sysctl -n hw.ncpu)
echo "Building with $NUM_CPUS parallel jobs"
echo ""

make -j$NUM_CPUS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    echo ""
    echo "Common issues and solutions:"
    echo ""
    echo "1. SWIG version issues:"
    echo "   Verify: \$SWIG/bin/swig -version"
    echo "   Should be 3.0.x"
    echo ""
    echo "2. Java path issues:"
    echo "   Verify: java -version"
    echo "   Should be JDK 11"
    echo ""
    echo "3. Missing headers:"
    echo "   Check the error message for missing files"
    echo "   Install missing dependencies with brew"
    echo ""
    echo "4. Adapter-specific errors:"
    echo "   Check which adapter is failing"
    echo "   Disable it following the instructions in QUICK_DISABLE_BLUEBOX.md"
    echo ""
    exit 1
fi

echo ""

# Step 9: Install
echo -e "${GREEN}Step 9: Installing Micro-Manager...${NC}"
echo ""

sudo make install

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Install successful${NC}"
else
    echo -e "${RED}✗ Install failed${NC}"
    exit 1
fi

echo ""

# Step 10: Fix library paths (if Spinnaker built)
if [ $SPINNAKER_AVAILABLE -eq 1 ]; then
    echo -e "${GREEN}Step 10: Fixing Spinnaker library paths...${NC}"
    echo ""

    SPINNAKER_DYLIB="$INSTALL_PREFIX/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib"

    if [ -f "$SPINNAKER_DYLIB" ]; then
        echo "Updating rpath for Spinnaker adapter..."
        sudo install_name_tool -add_rpath "$SPINNAKER_DIR/lib" "$SPINNAKER_DYLIB"
        echo -e "${GREEN}✓ Library paths updated${NC}"
    else
        echo "Spinnaker adapter not built (may have failed or not configured)"
    fi
else
    echo -e "${YELLOW}Step 10: Skipped (Spinnaker not available)${NC}"
fi

echo ""
echo -e "${BLUE}=========================================================================${NC}"
echo -e "${GREEN}✓ Build and install complete!${NC}"
echo -e "${BLUE}=========================================================================${NC}"
echo ""
echo "To run Micro-Manager:"
echo "  $INSTALL_PREFIX/bin/micromanager"
echo ""
echo "To uninstall:"
echo "  cd $BUILD_DIR"
echo "  sudo make uninstall"
echo ""
echo "Build location: $BUILD_DIR"
echo "Install location: $INSTALL_PREFIX"
echo ""
