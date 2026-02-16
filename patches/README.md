# Patches for Micro-Manager Build and Spinnaker Support

## Overview

This directory contains patch files to modify Micro-Manager's build system to:

1. Disable problematic adapters (BlueboxOptics_niji, SerialManager)
2. Add Spinnaker4 adapter support with SDK detection

## Applying Patches

### Prerequisites

Ensure you're in Micro-Manager source directory:
```bash
cd /tmp/micro-manager-1-nospace-1771252231
```

Check that the git repository is clean:
```bash
git status
```

### Patch 1: Disable BlueboxOptics_niji

This patch disables the BlueboxOptics_niji adapter which fails to compile on modern macOS due to Boost `lexical_cast` being removed in Boost 1.66+.

**Apply:**
```bash
# Navigate to project root
cd /tmp/micro-manager-1-nospace-1771252231

# Apply the patch
patch -p1 < /home/engine/project/patches/disable_blueboxoptics_niji.patch

# Verify patch was applied
git status
```

**Expected changes:**
- `mmCoreAndDevices/DeviceAdapters/Makefile.am` - BlueboxOptics_niji commented out
- `mmCoreAndDevices/DeviceAdapters/configure.ac` - m4_define for BlueboxOptics_niji commented out

### Patch 2: Disable SerialManager

This patch disables the SerialManager adapter which fails to compile on modern macOS due to Boost Asio API changes (`io_service` replaced by `io_context` in newer Boost versions).

**Why this patch is needed:**
Simply commenting out `m4_define` in configure.ac is NOT sufficient. The Autoconf/Automake build system processes Makefile.am first, so even if configure.ac disables it, SerialManager will still be built because it's in SUBDIRS.

This patch properly removes SerialManager from SUBDIRS to prevent it from being compiled.

**Apply:**
```bash
cd /tmp/micro-manager-1-nospace-1771252231

# Apply the patch
patch -p1 < /home/engine/project/patches/disable_serialmanager_fix.patch

# Verify patch was applied
git status
```

**Expected changes:**
- `mmCoreAndDevices/DeviceAdapters/Makefile.am` - SerialManager **removed** from SUBDIRS (not just commented)
- `mmCoreAndDevices/DeviceAdapters/configure.ac` - m4_define for SerialManager commented out

### Patch 3: Add Spinnaker4 Support

This patch adds complete Spinnaker SDK 4.x.x support to Micro-Manager's build system.

**Apply:**
```bash
cd /tmp/micro-manager-1-nospace-1771252231

# Copy adapter files first (if not already done)
mkdir -p mmCoreAndDevices/DeviceAdapters/Spinnaker4
cp -r /home/engine/project/SPINNAKER_ADAPTER_TEMPLATE/* \
      mmCoreAndDevices/DeviceAdapters/Spinnaker4/

# Apply the patch
patch -p1 < /home/engine/project/patches/add_spinnaker4_support.patch

# Verify patch was applied
git status
```

**Expected changes:**
- `mmCoreAndDevices/DeviceAdapters/Makefile.am` - Spinnaker4 added to SUBDIRS
- `mmCoreAndDevices/DeviceAdapters/configure.ac` - Spinnaker SDK detection and configuration added

**What the patch does:**
1. Adds `--with-spinnaker` configure option
2. Detects Spinnaker SDK 4.x.x headers and libraries
3. Sets up compiler and linker flags
4. Detects macOS Apple Silicon (ARM64) architecture
5. Creates conditional build for Spinnaker4 adapter
6. Adds SDK installation path checking

### Applying All Patches

To apply all patches at once:

```bash
cd /tmp/micro-manager-1-nospace-1771252231

# Copy adapter files
mkdir -p mmCoreAndDevices/DeviceAdapters/Spinnaker4
cp -r /home/engine/project/SPINNAKER_ADAPTER_TEMPLATE/* \
      mmCoreAndDevices/DeviceAdapters/Spinnaker4/

# Apply all patches (disable problematic adapters first, then add Spinnaker support)
patch -p1 < /home/engine/project/patches/disable_blueboxoptics_niji.patch
patch -p1 < /home/engine/project/patches/disable_serialmanager_fix.patch
patch -p1 < /home/engine/project/patches/add_spinnaker4_support.patch

# Verify
git status
```

## Build After Patches

### Step 1: Re-generate configure script

```bash
cd /tmp/micro-manager-1-nospace-1771252231
./autogen.sh
```

### Step 2: Configure

```bash
# Set environment
export SWIG=/tmp/swig-3.0.12-install/bin/swig
export JAVA_HOME=$(/usr/libexec/java_home -v 11)

# Configure with Spinnaker support
./configure \
    --prefix=/usr/local \
    --with-java=$JAVA_HOME \
    --with-spinnaker=/opt/spinnaker
```

**Verify configure detected Spinnaker:**

Look for output like:
```
checking for Spinnaker SDK 4.x.x headers... found in /opt/spinnaker/include
checking for Spinnaker SDK 4.x.x libraries... found in /opt/spinnaker/lib
Building Spinnaker4 adapter for macOS Apple Silicon (ARM64)
```

### Step 3: Build

```bash
# Fetch dependencies
make fetchdeps

# Build with all CPU cores
make -j$(sysctl -n hw.ncpu)
```

### Step 4: Install

```bash
sudo make install
```

### Step 5: Fix library paths

```bash
# Update rpath for Spinnaker adapter
sudo install_name_tool -add_rpath /opt/spinnaker/lib \
    /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib

# Verify
otool -L /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
```

## Verifying Patches

### Check Patches Applied

```bash
# Check BlueboxOptics_niji is disabled
grep "BlueboxOptics_niji" mmCoreAndDevices/DeviceAdapters/Makefile.am
# Should show commented out line

grep "BlueboxOptics_niji" mmCoreAndDevices/DeviceAdapters/configure.ac
# Should show commented out m4_define
```

```bash
# Check SerialManager is disabled
grep "SerialManager" mmCoreAndDevices/DeviceAdapters/Makefile.am
# Should NOT show SerialManager (removed from SUBDIRS)

grep "SerialManager" mmCoreAndDevices/DeviceAdapters/configure.ac
# Should show commented out m4_define with DISABLED marker
```

```bash
# Check Spinnaker4 is added
grep "Spinnaker4" mmCoreAndDevices/DeviceAdapters/Makefile.am
# Should show Spinnaker4 in SUBDIRS

grep -A5 "Spinnaker SDK 4.x.x detection" mmCoreAndDevices/DeviceAdapters/configure.ac
# Should show SDK detection code
```

### Test Build

After applying patches and building:

```bash
# Check adapter was built
ls -la /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib

# Should exist and show reasonable size (~200-500KB)
```

## Reverting Patches

If you need to revert a patch:

```bash
# Revert specific patch
patch -p1 -R < /home/engine/project/patches/disable_blueboxoptics_niji.patch

# Or revert everything
git checkout -- mmCoreAndDevices/DeviceAdapters/Makefile.am
git checkout -- mmCoreAndDevices/DeviceAdapters/configure.ac
```

## Troubleshooting

### Patch Fails to Apply

**Error**: "patch: **** Only garbage was found in the patch input"

**Solution**: Ensure you're applying from correct directory
```bash
# Must be in Micro-Manager root
cd /tmp/micro-manager-1-nospace-1771252231
pwd  # Should show /tmp/micro-manager-1-nospace-1771252231
```

**Error**: "patch: patch does not apply"

**Solution**: File might already be modified
```bash
# Check current state
git diff mmCoreAndDevices/DeviceAdapters/Makefile.am

# Revert if needed
git checkout -- mmCoreAndDevices/DeviceAdapters/Makefile.am

# Then apply patch again
patch -p1 < /home/engine/project/patches/disable_blueboxoptics_niji.patch
```

### Configure Can't Find Spinnaker

**Error**: "Spinnaker SDK 4.x.x headers not found"

**Solution**:
```bash
# Verify SDK is installed
ls -la /opt/spinnaker/include/Spinnaker.h
ls -la /opt/spinnaker/lib/libSpinnaker.dylib

# If not installed, install from FLIR website:
# https://www.flir.com/products/spinnaker-sdk/

# Or use different path if installed elsewhere
./configure --with-spinnaker=/path/to/spinnaker
```

### Library Path Issues

**Error**: "Library not loaded: @rpath/libSpinnaker.dylib"

**Solution**:
```bash
# Update rpath
sudo install_name_tool -add_rpath /opt/spinnaker/lib \
    /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib

# Verify
otool -L /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
# Should show: /opt/spinnaker/lib (current_version) (compatibility_version)
```

### Library Linking Errors

**Error**: Multiple "undefined reference to 'boost::...'" errors

**Solution**: This is likely caused by incorrect Boost include/library paths. Ensure:
```bash
# Homebrew is installed correctly
brew list boost

# Boost headers are accessible
ls -la /opt/homebrew/include/boost/

# Configure with explicit Boost path
./configure --with-boost=/opt/homebrew

# Or set environment variables
export CPPFLAGS="-I/opt/homebrew/include"
export LDFLAGS="-L/opt/homebrew/lib"
```

### SerialManager Still Being Built

**Problem**: Even after patching, SerialManager still compiles and fails

**Solution 1**: Verify patch was applied correctly
```bash
# Check Makefile.am
grep SerialManager mmCoreAndDevices/DeviceAdapters/Makefile.am
# Should NOT contain SerialManager in SUBDIRS

# Check configure.ac
grep -A2 -B2 "SerialManager" mmCoreAndDevices/DeviceAdapters/configure.ac
# Should show DISABLED m4_define, NOT an active one
```

**Solution 2**: Clean build artifacts
```bash
make clean
# Or for complete clean
make distclean

# Then re-apply patches and rebuild
```

**Solution 3**: Manual deletion
```bash
# Remove entire SerialManager directory
cd mmCoreAndDevices/DeviceAdapters
rm -rf SerialManager

# Rebuild
./autogen.sh
./configure --prefix=/usr/local --with-java=$JAVA_HOME
make -j$(sysctl -n hw.ncpu)
```

## Manual Patching (If patch command fails)

If `patch` command doesn't work, you can manually apply changes.

### For disable_blueboxoptics_niji.patch

**File 1: `mmCoreAndDevices/DeviceAdapters/Makefile.am`**

Find the SUBDIRS line and comment out BlueboxOptics_niji:

```makefile
# Find line (around line 5-10):
SUBDIRS = DemoCamera BlueboxOptics_niji $(DEMO_ADAPTERS) ...

# Comment it out:
SUBDIRS = DemoCamera # BlueboxOptics_niji DISABLED: Boost lexical_cast issue on macOS ARM64 $(DEMO_ADAPTERS) ...
```

**File 2: `mmCoreAndDevices/DeviceAdapters/configure.ac`**

Find the m4_define for BlueboxOptics_niji (search for it):
```m4
# BEFORE:
m4_define([BlueboxOptics_niji], [deviceadapter/BlueboxOptics_niji/Makefile.am])

# AFTER:
# DISABLED: BlueboxOptics_niji - Boost lexical_cast issue on macOS ARM64
# m4_define([BlueboxOptics_niji], [deviceadapter/BlueboxOptics_niji/Makefile.am])
```

### For disable_serialmanager_fix.patch

**File 1: `mmCoreAndDevices/DeviceAdapters/Makefile.am`**

Find SUBDIRS line and REMOVE SerialManager:

```makefile
# Find line (around line 5-10):
SUBDIRS = DemoCamera SerialManager BlueboxOptics_niji ...

# Remove it entirely:
SUBDIRS = DemoCamera BlueboxOptics_niji # SerialManager DISABLED: Boost Asio io_service incompatibility on macOS ARM64 $(DEMO_ADAPTERS) ...
```

**File 2: `mmCoreAndDevices/DeviceAdapters/configure.ac`**

Find and DISABLE the m4_define for SerialManager:

```m4
# BEFORE:
m4_define([SerialManager], [deviceadapter/SerialManager/Makefile.am])

# AFTER:
# DISABLED: SerialManager - Boost Asio io_service incompatibility on macOS ARM64
# m4_define([SerialManager], [deviceadapter/SerialManager/Makefile.am])
```

### For add_spinnaker4_support.patch

See detailed changes in the patch file. The main additions are:

1. **Add Spinnaker4 to SUBDIRS** in Makefile.am
2. **Add SDK detection** to configure.ac (see patch for exact code)

## Summary

These patches provide:

1. **Quick fix** for BlueboxOptics_niji build blocker
2. **Complete fix** for SerialManager build blocker (removes from SUBDIRS)
3. **Complete integration** of Spinnaker4 adapter into build system
4. **SDK detection** with proper error checking
5. **Apple Silicon awareness** for ARM64 architecture
6. **Conditional build** support (only builds if SDK is found)
7. **Documentation** explaining reasons and alternatives

After applying patches and building, you'll have a fully functional Micro-Manager with Spinnaker SDK 4.x.x support for your Blackfly S camera on macOS Apple Silicon.

### SerialManager Note

SerialManager is non-essential for most Micro-Manager installations (it's primarily for serial port communication with specific hardware like some stages, filter wheels, and shutters). Disabling it:

- Allows build to proceed immediately
- Does not affect camera adapters (including Spinnaker4)
- Maintains all core Micro-Manager functionality

If you specifically need serial communication functionality, see `SERIALMANAGER_BOOST_ASIO_FIX.md` for information about the issue and alternative solutions.
