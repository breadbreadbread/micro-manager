# Plan: Add macOS Apple Silicon Support for FLIR Spinnaker SDK

## Executive Summary

This plan addresses two interconnected problems:
1. **Poor macOS Apple Silicon support** in Micro-Manager
2. **FLIR Blackfly S USB 3 camera compatibility** requiring Spinnaker SDK 4.x.x on macOS Apple Silicon

The current Spinnaker device adapter only works on Windows with Spinnaker SDK 2.3.x, which is incompatible with your setup (macOS Apple Silicon + Spinnaker SDK 4.x.x).

## Current State Analysis

### Micro-Manager Architecture

Based on examination of the codebase:

1. **Device Adapter Architecture**
   - Micro-Manager uses C++ device adapters that link against vendor SDKs
   - Each camera type has its own device adapter (`mmgr_dal_Spinnaker.dll` on Windows)
   - Device adapters are located in `mmCoreAndDevices/DeviceAdapters/` (open source) and `mmCoreAndDevices/SecretDeviceAdapters/` (proprietary)
   - The Spinnaker adapter would be in `SecretDeviceAdapters` since it requires FLIR's proprietary SDK

2. **Build System**
   - **Windows**: Apache Ant + Visual Studio
   - **Unix (macOS/Linux)**: GNU Autotools (configure/make) + Ant for Java components
   - Device adapters with external dependencies require SDK detection during configure

3. **JNI/SWIG Layer**
   - MMCore is wrapped with SWIG to create MMCoreJ (Java bindings)
   - Device adapters are loaded dynamically at runtime via dlopen/dlsym

### The Problem

1. **Current Spinnaker Adapter Limitations**
   - Platform: Windows only
   - SDK Version: 2.3.x only
   - Architecture: x86_64 only (Windows)
   - macOS support: None

2. **SDK Version Differences**
   - **Spinnaker 2.3.x**: Older API, primarily Windows support
   - **Spinnaker 4.x.x**: Current API with full macOS Apple Silicon (ARM64) support
   - API changes between versions require adapter code modifications

3. **Your Requirements**
   - Platform: macOS Apple Silicon (ARM64)
   - SDK: Spinnaker 4.x.x (installed and working)
   - Camera: FLIR Blackfly S USB 3

## Solution Plan

### Phase 1: Investigation and Setup

**Objective**: Understand the current Spinnaker adapter implementation and prepare the development environment.

#### Task 1.1: Locate and Examine Current Spinnaker Adapter
```bash
# Initialize the mmCoreAndDevices submodule
git submodule update --init --recursive mmCoreAndDevices

# The Spinnaker adapter is likely in:
mmCoreAndDevices/SecretDeviceAdapters/Spinnaker/  # or similar directory
```

**Deliverables**:
- Current Spinnaker adapter source code
- Build configuration files (Makefile.am, configure.ac modifications)
- SDK integration patterns

#### Task 1.2: Analyze Spinnaker SDK 4.x.x API
```bash
# Assuming SDK is installed at:
/opt/spinnaker/  # or similar
# Look for:
# - Include headers: /opt/spinnaker/include/
# - Libraries: /opt/spinnaker/lib/
# - Documentation: /opt/spinnaker/docs/
```

**Deliverables**:
- API documentation for Spinnaker 4.x.x
- List of API changes from 2.3.x to 4.x.x
- Example code from SDK demonstrating camera initialization and acquisition

#### Task 1.3: Examine Similar Camera Adapters
Look at adapters that already support macOS to understand the pattern:
```bash
# Find adapters that compile on macOS
mmCoreAndDevices/DeviceAdapters/PointGreyChameleon/  # May have similar patterns
mmCoreAndDevices/DeviceAdapters/DemoCamera/           # Reference implementation
mmCoreAndDevices/DeviceAdapters/Thorlabs/              # May have macOS support
```

**Deliverables**:
- macOS adapter build patterns
- Dynamic linking setup for vendor libraries
- Library path configuration

### Phase 2: Adapter Code Modernization

**Objective**: Update the Spinnaker adapter to work with SDK 4.x.x.

#### Task 2.1: Create New macOS ARM64-Specific Adapter
Since the SDK version changes are significant, create a new adapter module:

```
mmCoreAndDevices/DeviceAdapters/Spinnaker4/
├── Spinnaker4.h
├── Spinnaker4.cpp
├── Spinnaker4Camera.h       # Camera class implementation
├── Spinnaker4Camera.cpp
├── Makefile.am              # Build configuration
└── README.md                # Build instructions
```

**Key Changes from 2.3.x to 4.x.x**:
- Namespace changes (likely `Spinnaker::` vs older namespacing)
- Camera initialization sequence
- Image buffer handling
- Property/node map access
- GenICam API version (GenApi 3.x in Spinnaker 4.x.x)

#### Task 2.2: Implement Core Camera Functions

Implement the Micro-Manager camera device adapter interface:

```cpp
// Required MMCore camera API methods
class Spinnaker4Camera : public CCameraBase<Spinnaker4Camera> {
public:
    Spinnaker4Camera();
    ~Spinnaker4Camera();

    // Initialization
    int Initialize();
    int Shutdown();

    // Image acquisition
    int SnapImage();
    const unsigned char* GetImageBuffer();
    unsigned GetImageWidth() const;
    unsigned GetImageHeight() const;
    unsigned GetImageBytesPerPixel() const;

    // Camera controls
    int SetExposure(double exposureMs);
    double GetExposure();
    int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize);
    int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize);
    int ClearROI();

    // Properties
    int SetProperty(const char* name, const char* value);
    int GetProperty(const char* name, char* value);

    // Hardware trigger support (important for performance)
    int StartSequenceAcquisition(long numImages, double intervalMs);
    int StopSequenceAcquisition();
};
```

#### Task 2.3: SDK Integration

```cpp
// Spinnaker4.cpp - Integration with SDK 4.x.x

#include "Spinnaker.h"  // Main Spinnaker header

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class Spinnaker4Camera {
private:
    SystemPtr system_;
    CameraList_t cameraList_;
    CameraPtr camera_;
    ImagePtr latestImage_;

public:
    int Initialize() {
        // Initialize Spinnaker system
        system_ = System::GetInstance();

        // Get camera list
        cameraList_ = system_->GetCameras();
        if (cameraList_.GetSize() == 0) {
            return DEVICE_NOT_CONNECTED;
        }

        // Get first camera
        camera_ = cameraList_.GetByIndex(0);

        // Initialize camera
        camera_->Init();

        // Configure for streaming
        CEnumerationPtr ptrAcquisitionMode = camera_->GetNodeMap().GetNode("AcquisitionMode");
        if (IsAvailable(ptrAcquisitionMode) && IsWritable(ptrAcquisitionMode)) {
            CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
            if (IsAvailable(ptrAcquisitionModeContinuous) && IsReadable(ptrAcquisitionModeContinuous)) {
                ptrAcquisitionMode->SetIntValue(ptrAcquisitionModeContinuous->GetValue());
            }
        }

        return DEVICE_OK;
    }

    int SnapImage() {
        camera_->BeginAcquisition();
        ImagePtr pImage = camera_->GetNextImage(1000);  // 1 second timeout
        latestImage_ = pImage->Convert(PixelFormat_Mono8, HQ_LINEAR);
        camera_->EndAcquisition();
        return DEVICE_OK;
    }

    const unsigned char* GetImageBuffer() {
        return static_cast<const unsigned char*>(latestImage_->GetData());
    }
};
```

### Phase 3: Build System Integration

**Objective**: Configure the build system to compile the adapter on macOS ARM64.

#### Task 3.1: Update Build Configuration

**Modify `mmCoreAndDevices/DeviceAdapters/Makefile.am`**:
```makefile
# Add Spinnaker4 to SUBDIRS
SUBDIRS = ... Spinnaker4 ...
```

**Create `mmCoreAndDevices/DeviceAdapters/Spinnaker4/Makefile.am`**:
```makefile
AM_CPPFLAGS = -I$(top_srcdir)/MMDevice \
              -I$(SPINNAKER_INCLUDE_DIR) \
              $(MMDEVAPI_CFLAGS)

AM_LDFLAGS = -module -avoid-version -no-undefined

deviceadapter_LTLIBRARIES = libmmgr_dal_Spinnaker4.la

libmmgr_dal_Spinnaker4_la_SOURCES = \
    Spinnaker4.cpp \
    Spinnaker4.h

libmmgr_dal_Spinnaker4_la_LIBADD = $(MMDEVAPI_LIBS) \
    $(SPINNAKER_LIBS)

libmmgr_dal_Spinnaker4_la_LDFLAGS = $(AM_LDFLAGS) \
    -framework CoreFoundation \
    -framework IOKit
```

#### Task 3.2: Configure Script Updates

**Modify `mmCoreAndDevices/DeviceAdapters/configure.ac`**:
```m4
# Spinnaker SDK 4.x.x detection
AC_ARG_WITH([spinnaker],
    [AS_HELP_STRING([--with-spinnaker=DIR],
        [Spinnaker SDK 4.x.x installation directory])],
    [SPINNAKER_DIR="$withval"],
    [SPINNAKER_DIR=""])

if test -n "$SPINNAKER_DIR"; then
    SPINNAKER_INCLUDE_DIR="$SPINNAKER_DIR/include"
    SPINNAKER_LIB_DIR="$SPINNAKER_DIR/lib"

    AC_CHECK_FILE([$SPINNAKER_INCLUDE_DIR/Spinnaker.h],
        [AC_MSG_NOTICE([Found Spinnaker SDK headers])],
        [AC_MSG_ERROR([Spinnaker SDK headers not found in $SPINNAKER_INCLUDE_DIR])])

    AC_CHECK_FILE([$SPINNAKER_LIB_DIR/libSpinnaker.dylib],
        [SPINNAKER_LIBS="-L$SPINNAKER_LIB_DIR -lSpinnaker"],
        [AC_MSG_ERROR([Spinnaker SDK libraries not found in $SPINNAKER_LIB_DIR])])

    AC_SUBST([SPINNAKER_INCLUDE_DIR])
    AC_SUBST([SPINNAKER_LIBS])

    build_spinnaker4=yes
else
    build_spinnaker4=no
fi

AM_CONDITIONAL([BUILD_SPINNAKER4], [test "x$build_spinnaker4" = xyes])
```

#### Task 3.3: macOS ARM64 Specifics

**Handle Apple Silicon specifics in configure.ac**:
```m4
# Check for Apple Silicon
case $host_cpu in
    arm64|aarch64)
        MACOS_ARM64=yes
        ;;
    *)
        MACOS_ARM64=no
        ;;
esac

if test "x$MACOS_ARM64" = xyes; then
    AC_MSG_NOTICE([Building for macOS Apple Silicon])
    # Spinnaker 4.x.x is required for ARM64
    AS_IF([test "x$build_spinnaker4" != xyes],
        [AC_MSG_WARN([Spinnaker 4.x.x required for macOS ARM64])])
fi
```

### Phase 4: Dynamic Library Installation

**Objective**: Ensure the Spinnaker SDK libraries are properly linked and installed.

#### Task 4.1: Library Path Configuration

**Create `mmCoreAndDevices/DeviceAdapters/Spinnaker4/postinstall.sh`**:
```bash
#!/bin/bash
# Update rpath to include Spinnaker library directory
install_name_tool -change @rpath/libSpinnaker.dylib \
    @rpath/libSpinnaker.dylib \
    "$DESTDIR/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib"

# Add rpath to Spinnaker installation
install_name_tool -add_rpath /opt/spinnaker/lib \
    "$DESTDIR/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib"
```

#### Task 4.2: Packaging Strategy

For distribution, need to include or reference the Spinnaker libraries:

**Option 1: Bundle with Micro-Manager** (Recommended)
```bash
# Copy Spinnaker libraries into the distribution
cp /opt/spinnaker/lib/*.dylib build/install/lib/micro-manager/
```

**Option 2: Reference system installation**
```bash
# User must have Spinnaker SDK installed
# Document requirement in README
```

### Phase 5: Testing and Validation

**Objective**: Verify the adapter works correctly on macOS Apple Silicon.

#### Task 5.1: Unit Tests

```cpp
// tests/Spinnaker4Test.cpp

TEST(Spinnaker4Test, Initialization) {
    Spinnaker4Camera camera;
    ASSERT_EQ(DEVICE_OK, camera.Initialize());
}

TEST(Spinnaker4Test, SnapImage) {
    Spinnaker4Camera camera;
    camera.Initialize();
    ASSERT_EQ(DEVICE_OK, camera.SnapImage());
    const unsigned char* buffer = camera.GetImageBuffer();
    ASSERT_NE(nullptr, buffer);
}

TEST(Spinnaker4Test, GetImageProperties) {
    Spinnaker4Camera camera;
    camera.Initialize();
    camera.SnapImage();
    ASSERT_GT(camera.GetImageWidth(), 0);
    ASSERT_GT(camera.GetImageHeight(), 0);
}
```

#### Task 5.2: Integration Testing

**Create test configuration for Micro-Manager**:
```
# config.cfg for testing

Device,Spinnaker4,Spinnaker4,Spinnaker4,0
Property,Spinnaker4,Camera Name,Blackfly S BFS-U3-16S2M

# Initialize camera
Initialize,Spinnaker4

# Test snap
Snap,Spinnaker4
Save,Spinnaker4,/tmp/test_image.tif
```

#### Task 5.3: Performance Testing

Test various acquisition modes:
- Single image capture
- Continuous streaming
- Hardware triggering (if supported)
- ROI operations

### Phase 6: Documentation and Release

**Objective**: Document the changes and prepare for release.

#### Task 6.1: User Documentation

**Create `mmCoreAndDevices/DeviceAdapters/Spinnaker4/README.md`**:
```markdown
# FLIR Spinnaker Camera Adapter for Micro-Manager

## Overview
This adapter enables Micro-Manager to control FLIR Blackfly and other Spinnaker-based
cameras on macOS Apple Silicon (ARM64) using Spinnaker SDK 4.x.x.

## Requirements
- macOS 11.0 or later (Apple Silicon / ARM64)
- Spinnaker SDK 4.x.x installed
- FLIR camera (tested with Blackfly S BFS-U3-16S2M)

## Installation
1. Install Spinnaker SDK 4.x.x from FLIR website
2. Configure Micro-Manager with:
   ./configure --with-spinnaker=/opt/spinnaker
3. Build: make
4. Install: sudo make install

## Building from Source
[Detailed build instructions]

## Configuration
[Micro-Manager configuration examples]

## Troubleshooting
[Common issues and solutions]
```

#### Task 6.2: Developer Documentation

Document the adapter implementation:
- API changes from 2.3.x to 4.x.x
- Build system modifications
- macOS ARM64 specific considerations
- Testing procedures

## Implementation Timeline

### Week 1-2: Phase 1 (Investigation)
- Locate and examine current adapter
- Analyze SDK 4.x.x API
- Study existing adapters

### Week 3-5: Phase 2 (Adapter Modernization)
- Create new adapter structure
- Implement core camera functions
- Integrate Spinnaker SDK 4.x.x

### Week 6-7: Phase 3 (Build System)
- Update Makefile.am
- Modify configure.ac
- Handle ARM64 specifics

### Week 8: Phase 4 (Library Installation)
- Configure library paths
- Set up dynamic linking
- Test library loading

### Week 9-10: Phase 5 (Testing)
- Write unit tests
- Integration testing
- Performance testing

### Week 11-12: Phase 6 (Documentation and Release)
- Write user documentation
- Create developer docs
- Prepare for community release

## Technical Challenges and Solutions

### Challenge 1: API Changes Between SDK Versions
**Problem**: Spinnaker 2.3.x and 4.x.x have significant API differences.

**Solution**:
- Create a compatibility layer mapping old API calls to new API
- Document all API changes
- Use conditional compilation to support both versions (if needed)

### Challenge 2: Cross-Platform Compatibility
**Problem**: Maintaining Windows support while adding macOS ARM64.

**Solution**:
- Use preprocessor directives to separate platform-specific code
- Shared implementation in base class
- Platform-specific implementations derived from base

### Challenge 3: SDK Detection
**Problem**: Automatically finding Spinnaker SDK on macOS.

**Solution**:
- Search common installation locations:
  - `/opt/spinnaker/`
  - `/usr/local/spinnaker/`
  - `~/Spinnaker/`
- Provide explicit path via `--with-spinnaker` configure option
- Document SDK installation requirements

### Challenge 4: Library Distribution
**Problem**: Spinnaker libraries must be available at runtime.

**Solution**:
- Use `@rpath` and `install_name_tool` to set library paths
- Option to bundle libraries with Micro-Manager distribution
- Document requirements for users to install SDK

### Challenge 5: macOS Security and Code Signing
**Problem**: macOS may block loading unsigned third-party libraries.

**Solution**:
- Document code signing requirements
- Provide instructions for disabling Gatekeeper for development
- Consider obtaining proper code signatures for distribution

## Build Commands

### On macOS Apple Silicon

```bash
# 1. Install prerequisites
brew install autoconf automake libtool swig@3 boost ant

# 2. Set Java environment
export JAVA_HOME=$(/usr/libexec/java_home -v 11)
export SWIG=/usr/local/opt/swig@3/bin/swig

# 3. Clone and initialize submodules
git clone --recurse-submodules https://github.com/micro-manager/micro-manager.git
cd micro-manager

# 4. Generate configure script
./autogen.sh

# 5. Configure with Spinnaker SDK
./configure \
    --prefix=/usr/local \
    --with-java=$JAVA_HOME \
    --with-spinnaker=/opt/spinnaker

# 6. Build
make fetchdeps
make -j$(sysctl -n hw.ncpu)

# 7. Install
sudo make install

# 8. Run Micro-Manager
/usr/local/bin/micromanager
```

## Testing Checklist

- [ ] Adapter compiles without errors on macOS ARM64
- [ ] Adapter loads successfully in Micro-Manager
- [ ] Camera is detected and initialized
- [ ] Single image capture works
- [ ] Continuous streaming works
- [ ] ROI operations work correctly
- [ ] Exposure control works
- [ ] Gain control works
- [ ] Properties can be read and written
- [ ] Hardware triggering works (if supported)
- [ ] Memory leak testing
- [ ] Stress testing (extended acquisition)
- [ ] Multiple camera testing (if applicable)

## Success Criteria

1. **Functional**: Blackfly S camera successfully captures images on macOS ARM64
2. **Performance**: Frame rates meet camera specifications
3. **Stability**: No crashes during extended use
4. **Compatibility**: Works with Micro-Manager GUI and scripting interfaces
5. **Documented**: Clear installation and usage instructions
6. **Maintainable**: Code follows Micro-Manager conventions
7. **Tested**: Comprehensive test suite with good coverage

## Future Improvements

1. **Multi-camera support**: Add support for multiple Spinnaker cameras simultaneously
2. **Advanced features**: Implement SDK-specific features (pixel format conversion, etc.)
3. **Performance optimization**: Use zero-copy buffers where possible
4. **Cross-platform**: Extend support to Linux ARM64
5. **Python bindings**: Expose adapter through PyMMCore

## Resources

### Documentation
- FLIR Spinnaker SDK 4.x.x Documentation: https://softwareservices.flir.com/spinnaker/latest/index.html
- Micro-Manager Device Adapter Guide: https://micro-manager.org/wiki/Micro-Manager_Device_Adapters
- Micro-Manager Building Guide: https://micro-manager.org/wiki/Building_Micro-Manager

### Code References
- Micro-Manager Device Adapter API: `mmCoreAndDevices/MMDevice/MMDevice.h`
- Example camera adapters: `mmCoreAndDevices/DeviceAdapters/DemoCamera/`
- Spinnaker SDK examples: `/opt/spinnaker/examples/`

### Community
- Micro-Manager Community Forum: https://micro-manager.org/wiki/Micro-Manager_Community
- FLIR Support: https://www.flir.com/support-center/

## Conclusion

This plan provides a comprehensive approach to adding macOS Apple Silicon support for FLIR Spinnaker cameras in Micro-Manager. The key challenges are:

1. Adapting to SDK 4.x.x API changes
2. Configuring the build system for macOS ARM64
3. Proper library linking and installation
4. Thorough testing and validation

By following this plan systematically, we can create a robust, performant adapter that enables your Blackfly S camera to work seamlessly with Micro-Manager on macOS Apple Silicon.
