# Quick Reference: Spinnaker SDK 4.x.x for Micro-Manager

## Key Differences: Spinnaker 2.3.x → 4.x.x

### Namespace Changes
```cpp
// 2.3.x
#include "SpinVideo.h"
using namespace Spinnaker;

// 4.x.x
#include "Spinnaker.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
```

### Camera Initialization
```cpp
// 2.3.x Pattern
CameraPtr camera = cameraList.GetByIndex(0);
camera->Init();

// 4.x.x Pattern (similar but with additional error checking)
CameraPtr camera = cameraList.GetByIndex(0);
if (!camera.IsValid()) {
    return DEVICE_NOT_CONNECTED;
}

GenICam::gcstring errorMessage;
camera->Init(errorMessage);
if (errorMessage != "") {
    return DEVICE_ERR;
}
```

### Image Buffer Access
```cpp
// 2.3.x
ImagePtr pImage = camera->GetNextImage();
unsigned char* data = static_cast<unsigned char*>(pImage->GetData());

// 4.x.x
ImagePtr pImage = camera->GetNextImage(1000);  // Timeout in ms
if (!pImage.IsValid()) {
    return DEVICE_ERR;
}

// Convert to desired format (important!)
ImagePtr convertedImage = pImage->Convert(PixelFormat_Mono8, HQ_LINEAR);
unsigned char* data = static_cast<unsigned char*>(convertedImage->GetData());
```

### Node Map Access (Properties)
```cpp
// Both versions use GenICam node maps, but 4.x.x has more error checking

// 2.3.x
CEnumerationPtr ptrAcquisitionMode = camera->GetNodeMap().GetNode("AcquisitionMode");
ptrAcquisitionMode->SetIntValue(ptrAcquisitionMode->GetEntryByName("Continuous")->GetValue());

// 4.x.x (with error checking)
CEnumerationPtr ptrAcquisitionMode = camera->GetNodeMap().GetNode("AcquisitionMode");
if (IsAvailable(ptrAcquisitionMode) && IsWritable(ptrAcquisitionMode)) {
    CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
    if (IsAvailable(ptrAcquisitionModeContinuous) && IsReadable(ptrAcquisitionModeContinuous)) {
        ptrAcquisitionMode->SetIntValue(ptrAcquisitionModeContinuous->GetValue());
    }
}
```

## Build Commands Reference

### Minimal Build (Adapter Only)
```bash
cd mmCoreAndDevices/DeviceAdapters/Spinnaker4

# Create a simple Makefile for testing
cat > Makefile.local << 'EOF'
CXX = clang++
CXXFLAGS = -I/opt/spinnaker/include -fPIC -std=c++14
LDFLAGS = -L/opt/spinnaker/lib -lSpinnaker

all: libmmgr_dal_Spinnaker4.dylib

libmmgr_dal_Spinnaker4.dylib: Spinnaker4.cpp Spinnaker4Camera.cpp
	$(CXX) $(CXXFLAGS) -dynamiclib -o $@ $^ $(LDFLAGS) \
		-undefined dynamic_lookup \
		-install_name @rpath/$@

test: test_camera.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.dylib test
EOF

make -f Makefile.local
```

### Full Micro-Manager Build
```bash
# Complete build with all components
./autogen.sh
./configure \
    --prefix=/usr/local \
    --with-java=$JAVA_HOME \
    --with-spinnaker=/opt/spinnaker

make fetchdeps  # Download dependencies
make -j8        # Build with 8 parallel jobs
sudo make install
```

## Debugging Commands

### Check SDK Installation
```bash
# Verify Spinnaker SDK is installed
ls -la /opt/spinnaker/
ls -la /opt/spinnaker/lib/
ls -la /opt/spinnaker/include/

# Check library architecture
file /opt/spinnaker/lib/libSpinnaker.dylib
# Should output: .../libSpinnaker.dylib: Mach-O 64-bit dynamically linked shared library arm64

# Test library loading
otool -L /opt/spinnaker/lib/libSpinnaker.dylib
```

### Check Adapter Loading
```bash
# Verify adapter was built
ls -la /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4*

# Check adapter dependencies
otool -L /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
# Should include: @rpath/libSpinnaker.dylib (current_version) (compatibility_version)

# Test loading in Python (for quick testing)
python3 << 'EOF'
import ctypes
try:
    lib = ctypes.CDLL("/usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib")
    print("Adapter loaded successfully!")
except Exception as e:
    print(f"Failed to load: {e}")
EOF
```

### Camera Discovery
```bash
# Use Spinnaker's built-in tools to verify camera detection
/opt/spinnaker/bin/SpinView

# List cameras
/opt/spinnaker/bin/SpinListCameras

# Get camera info
/opt/spinnaker/bin/SpinGetCameraInfo
```

## Micro-Manager Configuration

### Device Manager Configuration
```
# Add to Device Manager configuration file

Device,Spinnaker4,Spinnaker4,Spinnaker4,0
Property,Spinnaker4,Camera Name,Blackfly S BFS-U3-16S2M

# Initialize
Initialize,Spinnaker4

# Set acquisition mode
Property,Spinnaker4,Acquisition Mode,Continuous

# Configure for hardware trigger (if needed)
Property,Spinnaker4,Trigger Mode,On
Property,Spinnaker4,Trigger Source,Line0
```

### Property Map Reference
```
# Common Spinnaker properties exposed through Micro-Manager

# Camera Control
Property,Spinnaker4,Acquisition Mode,Continuous
Property,Spinnaker4,Exposure Time,10000.0  # microseconds
Property,Spinnaker4,Gain,0.0  # dB
Property,Spinnaker4,Frame Rate,30.0  # fps

# Image Format
Property,Spinnaker4,Pixel Format,Mono8
Property,Spinnaker4,Width,2048
Property,Spinnaker4,Height,1536

# Triggering
Property,Spinnaker4,Trigger Mode,Off
Property,Spinnaker4,Trigger Source,Line0
Property,Spinnaker4,Trigger Selector,FrameStart

# Transport Layer
Property,Spinnaker4,Packet Size,1500
Property,Spinnaker4,Packet Delay,0
```

## Common Issues and Solutions

### Issue 1: "Library not loaded: @rpath/libSpinnaker.dylib"
```bash
# Solution: Update library paths
sudo install_name_tool -change @rpath/libSpinnaker.dylib \
    /opt/spinnaker/lib/libSpinnaker.dylib \
    /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib

sudo install_name_tool -add_rpath /opt/spinnaker/lib \
    /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
```

### Issue 2: "Image timeout"
```cpp
// Increase timeout value
ImagePtr pImage = camera->GetNextImage(5000);  // 5 second timeout
```

### Issue 3: Wrong pixel format
```cpp
// Check supported formats
CEnumerationPtr ptrPixelFormat = camera->GetNodeMap().GetNode("PixelFormat");
for (int i = 0; i < ptrPixelFormat->GetEntryCount(); i++) {
    CEnumEntryPtr ptrEntry = ptrPixelFormat->GetEntryByIndex(i);
    printf("Supported format: %s\n", ptrEntry->GetSymbolic().c_str());
}
```

### Issue 4: Camera not detected
```bash
# Check USB permissions
ls -la /dev/bus/usb/

# Reset camera (unplug and reconnect, or use):
/opt/spinnaker/bin/SpinResetCamera
```

## Performance Optimization Tips

### 1. Use Zero-Copy Buffers
```cpp
// Configure camera to write directly to user buffers
// (advanced - requires studying Spinnaker SDK docs)
```

### 2. Reduce Overhead
```cpp
// Disable unnecessary features during acquisition
CBooleanPtr ptrChunkModeActive = camera->GetNodeMap().GetNode("ChunkModeActive");
if (IsAvailable(ptrChunkModeActive) && IsWritable(ptrChunkModeActive)) {
    ptrChunkModeActive->SetValue(false);
}
```

### 3. Optimize Packet Size
```cpp
// Set optimal packet size for USB 3.0
CIntegerPtr ptrPacketSize = camera->GetTLStreamNodeMap().GetNode("PacketSize");
if (IsAvailable(ptrPacketSize) && IsWritable(ptrPacketSize)) {
    ptrPacketSize->SetValue(9000);  // Jumbo frames if network
    // For USB 3.0, typically 1500-9000
}
```

## Testing Script

### Python Test Script
```python
#!/usr/bin/env python3
"""
Quick test for Spinnaker adapter in Micro-Manager
"""

import sys
import time

# Try to import pymmcore (Python bindings)
try:
    import pymmcore
    print("✓ PyMMCore loaded successfully")
except ImportError:
    print("✗ PyMMCore not found. Install with: pip install pymmcore")
    sys.exit(1)

# Create core instance
core = pymmcore.CMMCore()
print("✓ MMCore instance created")

# Load adapter
try:
    core.loadDevice("Spinnaker4", "Spinnaker4", "/usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib")
    print("✓ Spinnaker adapter loaded")
except Exception as e:
    print(f"✗ Failed to load adapter: {e}")
    sys.exit(1)

# Initialize camera
try:
    core.initializeDevice("Spinnaker4")
    print("✓ Camera initialized")
except Exception as e:
    print(f"✗ Failed to initialize camera: {e}")
    sys.exit(1)

# Get camera properties
print("\nCamera Properties:")
try:
    print(f"  Exposure: {core.getExposure()} ms")
    print(f"  Gain: {core.getProperty('Spinnaker4', 'Gain')}")
except Exception as e:
    print(f"  Error getting properties: {e}")

# Test snap image
print("\nTesting image capture...")
try:
    core.snapImage()
    print("✓ Image captured")
    img = core.getImage()
    print(f"  Size: {img.shape}")
except Exception as e:
    print(f"✗ Failed to capture image: {e}")

# Cleanup
try:
    core.unloadAllDevices()
    print("\n✓ Cleanup complete")
except Exception as e:
    print(f"✗ Cleanup error: {e}")
```

## Resources

### FLIR Spinnaker SDK Documentation
- Main API Reference: https://softwareservices.flir.com/spinnaker/latest/index.html
- macOS Guide: https://softwareservices.flir.com/spinnaker/latest/getting_started/macos.html
- API Examples: https://softwareservices.flir.com/spinnaker/latest/examples.html

### Micro-Manager Resources
- Device Adapter API: `mmCoreAndDevices/MMDevice/MMDevice.h`
- Example Adapters: `mmCoreAndDevices/DeviceAdapters/`
- Build Guide: `doc/how-to-build.md`

### Quick Links
- FLIR Support Portal: https://www.flir.com/support-center/
- Micro-Manager Forum: https://micro-manager.org/wiki/Micro-Manager_Community
- Spinnaker SDK Download: https://www.flir.com/products/spinnaker-sdk/
