#!/bin/bash
#
# Script to disable problematic device adapters during build
# This allows building Micro-Manager on systems where some adapters have build issues
#

BUILD_DIR="$1"

if [ -z "$BUILD_DIR" ]; then
    echo "Usage: $0 <build_directory>"
    echo "Example: $0 /tmp/micro-manager-1-nospace-1771252231"
    exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory does not exist: $BUILD_DIR"
    exit 1
fi

echo "========================================================================="
echo "Disabling problematic device adapters in Micro-Manager"
echo "Build directory: $BUILD_DIR"
echo "========================================================================="
echo ""

# List of adapters known to have build issues
# Format: "AdapterName:Reason"
PROBLEMATIC_ADAPTERS=(
    "BlueboxOptics_niji:Missing Boost lexical_cast.hpp header"
    "DemoCamera:Not needed for production builds"
)

# Function to disable an adapter in Makefile.am
disable_adapter() {
    local adapter_dir="$1"
    local reason="$2"

    echo "Processing adapter: $adapter_dir"
    echo "  Reason: $reason"

    local makefile_am="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/$adapter_dir/Makefile.am"

    if [ ! -f "$makefile_am" ]; then
        echo "  Skipped: Makefile.am not found"
        echo ""
        return
    fi

    # Check if already disabled
    if grep -q "^# DISABLED:" "$makefile_am"; then
        echo "  Already disabled"
        echo ""
        return
    fi

    # Disable the adapter by commenting it out
    # We'll add DISABLED marker to the first line
    sed -i '.bak' "1s/^/# DISABLED: $reason\n/" "$makefile_am"
    rm -f "${makefile_am}.bak"

    echo "  Disabled: Added DISABLED marker to $makefile_am"
    echo ""
}

# Function to remove adapter from SUBDIRS in main Makefile.am
remove_from_subdirs() {
    local adapter="$1"
    local makefile="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/Makefile.am"

    echo "Removing $adapter from SUBDIRS list..."

    if [ ! -f "$makefile" ]; then
        echo "  Error: Main Makefile.am not found: $makefile"
        echo ""
        return
    fi

    # Check if adapter is in SUBDIRS
    if grep -q "SUBDIRS.*$adapter" "$makefile"; then
        # Comment out the adapter line
        sed -i '.bak' "s/\($adapter\)/#\1 # DISABLED/" "$makefile"
        rm -f "${makefile}.bak"
        echo "  Removed from SUBDIRS"
    else
        echo "  Not in SUBDIRS (may already be removed)"
    fi
    echo ""
}

# Function to disable adapter in configure.ac
disable_in_configure() {
    local adapter="$1"
    local configure_ac="$BUILD_DIR/mmCoreAndDevices/DeviceAdapters/configure.ac"

    echo "Removing $adapter from configure.ac..."

    if [ ! -f "$configure_ac" ]; then
        echo "  Error: configure.ac not found: $configure_ac"
        echo ""
        return
    fi

    # Look for m4_define for the adapter and comment it out
    sed -i '.bak' "/m4_define(\[$adapter\]/s/^/# DISABLED /" "$configure_ac"
    rm -f "${configure_ac}.bak"

    echo "  Disabled in configure.ac"
    echo ""
}

# Main execution
echo "Step 1: Disabling individual adapters..."
echo ""

for adapter_info in "${PROBLEMATIC_ADAPTERS[@]}"; do
    IFS=':' read -ra PARTS <<< "$adapter_info"
    adapter="${PARTS[0]}"
    reason="${PARTS[1]}"

    disable_adapter "$adapter" "$reason"
done

echo ""
echo "Step 2: Removing adapters from main DeviceAdapters/Makefile.am SUBDIRS..."
echo ""

for adapter_info in "${PROBLEMATIC_ADAPTERS[@]}"; do
    IFS=':' read -ra PARTS <<< "$adapter_info"
    adapter="${PARTS[0]}"

    remove_from_subdirs "$adapter"
done

echo ""
echo "Step 3: Disabling adapters in configure.ac..."
echo ""

for adapter_info in "${PROBLEMATIC_ADAPTERS[@]}"; do
    IFS=':' read -ra PARTS <<< "$adapter_info"
    adapter="${PARTS[0]}"

    disable_in_configure "$adapter"
done

echo "========================================================================="
echo "Summary: Disabled ${#PROBLEMATIC_ADAPTERS[@]} problematic adapter(s)"
echo "========================================================================="
echo ""
echo "Next steps:"
echo "1. Run: cd '$BUILD_DIR'"
echo "2. Re-generate configure script: ./autogen.sh"
echo "3. Re-configure with your options"
echo "4. Continue build: make -j\$(sysctl -n hw.ncpu)"
echo ""
echo "To re-enable an adapter later, remove the DISABLED markers from:"
echo "  - mmCoreAndDevices/DeviceAdapters/<adapter>/Makefile.am"
echo "  - mmCoreAndDevices/DeviceAdapters/Makefile.am"
echo "  - mmCoreAndDevices/DeviceAdapters/configure.ac"
echo ""
