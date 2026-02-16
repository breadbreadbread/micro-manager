# Quick Fix: Disable BlueboxOptics_niji Adapter

## Problem
The BlueboxOptics_niji adapter is failing to compile with:
```
lexical_cast.hpp' file not found
```

This is because newer Boost versions (1.66+) removed the `lexical_cast` header.

## Solution

Disable the BlueboxOptics_niji adapter to allow the build to proceed.

### Method 1: Quick Edit (Recommended)

Navigate to your build directory and run these commands:

```bash
cd /tmp/micro-manager-1-nospace-1771252231

# 1. Remove BlueboxOptics_niji from DeviceAdapters SUBDIRS
cd mmCoreAndDevices/DeviceAdapters

# Edit Makefile.am to comment out BlueboxOptics_niji
sed -i.bak 's/BlueboxOptics_niji/# BlueboxOptics_niji # DISABLED/' Makefile.am

# Verify it's commented out
grep BlueboxOptics_niji Makefile.am

# 2. Disable in configure.ac
cd ..
# Comment out the m4_define for BlueboxOptics_niji
sed -i.bak '/m4_define(\[BlueboxOptics_niji\]/s/^/# DISABLED /' configure.ac

# Verify it's commented out
grep -A2 -B2 BlueboxOptics_niji configure.ac

# 3. Re-run autogen to regenerate configure scripts
cd ../..
./autogen.sh

# 4. Re-configure (use your previous options)
# Make sure to set SWIG path
export SWIG=/tmp/swig-3.0.12-install/bin/swig
export JAVA_HOME=$(/usr/libexec/java_home -v 11)

./configure \
    --prefix=/usr/local \
    --with-java=$JAVA_HOME \
    # Add any other options you used previously

# 5. Continue the build
make -j$(sysctl -n hw.ncpu)
```

### Method 2: Manual Edit (If sed fails)

If the sed commands don't work, edit the files manually:

**File 1: `mmCoreAndDevices/DeviceAdapters/Makefile.am`**

Find the SUBDIRS line (around line 5-10):
```makefile
# BEFORE:
SUBDIRS = DemoCamera BlueboxOptics_niji ...

# AFTER:
SUBDIRS = DemoCamera # BlueboxOptics_niji DISABLED ...
```

**File 2: `mmCoreAndDevices/DeviceAdapters/configure.ac`**

Find the m4_define for BlueboxOptics_niji (search for it):
```m4
# BEFORE:
m4_define([BlueboxOptics_niji], [deviceadapter/BlueboxOptics_niji/Makefile.am])

# AFTER:
# DISABLED: BlueboxOptics_niji - requires old Boost with lexical_cast
# m4_define([BlueboxOptics_niji], [deviceadapter/BlueboxOptics_niji/Makefile.am])
```

### Method 3: Delete the adapter directory

If you don't need BlueboxOptics_niji at all:

```bash
cd /tmp/micro-manager-1-nospace-1771252231/mmCoreAndDevices/DeviceAdapters
rm -rf BlueboxOptics_niji

# Then proceed with steps 3-5 from Method 1
```

## After Fixing

### If other adapters fail

The same issue might occur with other adapters using old Boost code. Look for errors like:
- `lexical_cast.hpp not found`
- `Boost 1.66+ removed lexical_cast`
- Other Boost-related errors

Apply the same fix (comment out in Makefile.am and configure.ac).

### Common adapters that might have issues:

1. **BlueboxOptics_niji** - Already fixing
2. **DemoCamera** - Usually works, can keep
3. **USB3Vision** - Might have Boost issues
4. **PCO** - Older adapter, might have issues

### Alternative: Disable all non-essential adapters

If you just want to get Micro-Manager working quickly, you can keep only essential adapters:

**Essential adapters to keep:**
- DemoCamera (for testing)
- Any adapters for cameras you actually have

**In `mmCoreAndDevices/DeviceAdapters/Makefile.am`:**

```makefile
# Minimal SUBDIRS - only keep what you need
SUBDIRS = DemoCamera
```

Then add your Spinnaker adapter later:
```makefile
SUBDIRS = DemoCamera Spinnaker4
```

## Continue Building

After disabling the problematic adapter, continue the build:

```bash
cd /tmp/micro-manager-1-nospace-1771252231

# Make sure environment is set
export SWIG=/tmp/swig-3.0.12-install/bin/swig
export JAVA_HOME=$(/usr/libexec/java_home -v 11)

# Continue build
make -j$(sysctl -n hw.ncpu)

# If successful, install
sudo make install
```

## Verification

To verify the adapter is disabled:

```bash
# Check if BlueboxOptics_niji appears in configure output
./configure --help 2>&1 | grep -i bluebox
# Should show nothing or a DISABLED marker

# Check Makefile
cat mmCoreAndDevices/DeviceAdapters/Makefile.am | grep -i bluebox
# Should be commented out or missing
```

## Troubleshooting

### Error: "autogen.sh fails"

Make sure you have proper permissions:
```bash
chmod +x autogen.sh
./autogen.sh
```

### Error: "configure script not found"

autogen.sh should generate configure. If it fails, check output for errors.

### Error: "make continues to try building BlueboxOptics_niji"

You might need to clean build artifacts first:
```bash
make clean
# Or for a complete clean:
make distclean
```

## Next Steps

Once you have a successful Micro-Manager build:

1. **Install Micro-Manager**
   ```bash
   sudo make install
   ```

2. **Add the Spinnaker adapter**
   - Copy the adapter files to `mmCoreAndDevices/DeviceAdapters/Spinnaker4/`
   - Add `Spinnaker4` to SUBDIRS in `Makefile.am`
   - Add m4_define for Spinnaker4 in `configure.ac`
   - Add SDK detection in `configure.ac`
   - Rebuild

3. **Test the adapter**
   - Launch Micro-Manager
   - Load the Spinnaker4 device adapter
   - Initialize and capture images

See the main plan document for detailed Spinnaker adapter integration steps.
