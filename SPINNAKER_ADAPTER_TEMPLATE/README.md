# FLIR Spinnaker SDK 4.x.x Camera Adapter for Micro-Manager

## Overview

This adapter enables Micro-Manager to control FLIR Blackfly and other Spinnaker-based cameras on macOS Apple Silicon (ARM64) using Spinnaker SDK 4.x.x.

**Version**: 1.0.0
**Platform**: macOS 11.0+ (Apple Silicon/ARM64)
**SDK**: FLIR Spinnaker SDK 4.x.x
**Camera Models**: FLIR Blackfly S (tested with BFS-U3-16S2M)

## Features

- Full Micro-Manager camera device adapter interface
- Single image capture
- Continuous streaming
- Sequence acquisition
- Region of Interest (ROI) support
- Exposure and gain control
- Hardware triggering support (Line0, Line1, etc.)
- Binning support (1x, 2x)
- Multiple pixel formats (Mono8, Mono12, RGB8, etc.)

## Requirements

### System Requirements
- macOS 11.0 (Big Sur) or later
- Apple Silicon (M1, M2, or later)
- Xcode Command Line Tools: `xcode-select --install`
- USB 3.0 port (for Blackfly S USB 3 cameras)

### Software Requirements
- FLIR Spinnaker SDK 4.x.x
- Micro-Manager 2.x
- Required build tools:
  - autoconf
  - automake
  - libtool (GNU libtool)
  - pkg-config
  - swig 3.x
  - JDK 11
  - Ant

### SDK Installation

1. Download Spinnaker SDK 4.x.x for macOS Apple Silicon from FLIR website:
   https://www.flir.com/products/spinnaker-sdk/

2. Install the SDK:
   ```bash
   # Mount the DMG and install to default location
   sudo installer -pkg /Volumes/Spinnaker/Spinnaker.pkg -target /

   # SDK will be installed to:
   # /opt/spinnaker/
   ```

3. Verify installation:
   ```bash
   ls -la /opt/spinnaker/
   # Should see: include/, lib/, bin/, examples/, docs/

   # Check library architecture
   file /opt/spinnaker/lib/libSpinnaker.dylib
   # Should show: .../libSpinnaker.dylib: Mach-O 64-bit dynamically linked shared library arm64
   ```

4. Test camera detection:
   ```bash
   # Run SpinView GUI to verify camera is detected
   /opt/spinnaker/bin/SpinView

   # Or use command-line tool
   /opt/spinnaker/bin/SpinListCameras
   ```

## Installation

### Option 1: Building from Source

1. Install build dependencies:
   ```bash
   brew install autoconf automake libtool pkg-config swig@3 boost ant

   # Install JDK 11
   brew install --cask temurin@11

   # Set environment variables
   export JAVA_HOME=$(/usr/libexec/java_home -v 11)
   export SWIG=/usr/local/opt/swig@3/bin/swig
   ```

2. Clone Micro-Manager with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/micro-manager/micro-manager.git
   cd micro-manager
   ```

3. Copy this adapter to the correct location:
   ```bash
   mkdir -p mmCoreAndDevices/DeviceAdapters/Spinnaker4
   cp SPINNAKER_ADAPTER_TEMPLATE/* mmCoreAndDevices/DeviceAdapters/Spinnaker4/
   ```

4. Generate configure script:
   ```bash
   ./autogen.sh
   ```

5. Configure with Spinnaker SDK path:
   ```bash
   ./configure \
       --prefix=/usr/local \
       --with-java=$JAVA_HOME \
       --with-spinnaker=/opt/spinnaker
   ```

6. Build:
   ```bash
   make fetchdeps  # Download dependencies
   make -j$(sysctl -n hw.ncpu)  # Build with all cores
   ```

7. Install:
   ```bash
   sudo make install
   ```

8. Update library paths:
   ```bash
   # Add Spinnaker library path to adapter
   sudo install_name_tool -add_rpath /opt/spinnaker/lib \
       /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
   ```

### Option 2: Using Pre-built Binaries

If pre-built binaries are available:

1. Download the adapter package
2. Copy to Micro-Manager directory:
   ```bash
   cp libmmgr_dal_Spinnaker4.dylib /Applications/Micro-Manager/plugins/
   ```

3. Update library paths:
   ```bash
   install_name_tool -add_rpath /opt/spinnaker/lib \
       /Applications/Micro-Manager/plugins/libmmgr_dal_Spinnaker4.dylib
   ```

## Configuration

### Micro-Manager Device Manager

1. Open Micro-Manager
2. Go to **Tools → Device Manager** (or **Tools → Hardware Configuration Wizard**)
3. Click **Add** → **Camera**
4. Select **Spinnaker4** from the device list
5. Click **Add**
6. Click **Initialize** (should show "Initialized successfully")
7. Click **OK**

### Configuration File

Here's a sample configuration file for reference:

```ini
# Micro-Manager Configuration File for Spinnaker4 Adapter

# Device definition
Device,Spinnaker4,Spinnaker4,Spinnaker4,0
Property,Spinnaker4,Camera Name,Blackfly S BFS-U3-16S2M

# Initialize devices
Initialize,Spinnaker4

# Configure for standard operation
Property,Spinnaker4,Acquisition Mode,Continuous
Property,Spinnaker4,Trigger Mode,Off

# Set initial exposure (in milliseconds)
Property,Spinnaker4,Exposure Time,10.0

# Set gain (in dB)
Property,Spinnaker4,Gain,0.0

# Set pixel format
Property,Spinnaker4,Pixel Format,Mono8

# ROI settings (full sensor)
Property,Spinnaker4,Offset X,0
Property,Spinnaker4,Offset Y,0
Property,Spinnaker4,Width,2048
Property,Spinnaker4,Height,1536
```

## Usage

### Basic Image Capture

```python
import pymmcore
import matplotlib.pyplot as plt

# Create core instance
core = pymmcore.CMMCore()

# Load adapter
core.loadDevice("Camera", "Spinnaker4",
               "/usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib")

# Initialize camera
core.initializeDevice("Camera")

# Set exposure (in milliseconds)
core.setExposure(10.0)

# Capture image
core.snapImage()

# Get image
img = core.getImage()

# Display
plt.imshow(img, cmap='gray')
plt.colorbar()
plt.show()

# Cleanup
core.unloadAllDevices()
```

### Live Streaming

```python
import time
import cv2
import numpy as np

core = pymmcore.CMMCore()
core.loadDevice("Camera", "Spinnaker4",
               "/usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib")
core.initializeDevice("Camera")

# Start live acquisition
core.startContinuousSequenceAcquisition(0)  # 0 = unlimited frames

# Display for 10 seconds
for _ in range(300):  # ~30 fps for 10 seconds
    if core.getRemainingImageCount() > 0:
        img = core.getLastImage()
        cv2.imshow('Live', img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

core.stopSequenceAcquisition()
cv2.destroyAllWindows()
core.unloadAllDevices()
```

### Hardware Triggering

```python
# Configure camera for hardware trigger
core.setProperty("Camera", "Trigger Mode", "On")
core.setProperty("Camera", "Trigger Source", "Line0")
core.setProperty("Camera", "Trigger Selector", "FrameStart")

# Camera will now wait for external trigger on Line0
# Use Micro-Manager's triggering system to send trigger pulses
```

## Properties Reference

### Camera Control
| Property | Type | Range | Description |
|-----------|------|--------|-------------|
| Exposure Time | Float | 10.0 - 1000000.0 | Exposure time in microseconds |
| Gain | Float | 0.0 - 24.0 | Camera gain in dB |
| Frame Rate | Float | 0.0 - 100.0 | Frame rate in fps |

### Image Format
| Property | Type | Options | Description |
|-----------|------|---------|-------------|
| Pixel Format | String | Mono8, Mono12, RGB8, etc. | Pixel data format |
| Width | Integer | 0 - MaxWidth | Image width in pixels |
| Height | Integer | 0 - MaxHeight | Image height in pixels |
| Offset X | Integer | 0 - MaxWidth | X offset for ROI |
| Offset Y | Integer | 0 - MaxHeight | Y offset for ROI |

### Triggering
| Property | Type | Options | Description |
|-----------|------|---------|-------------|
| Trigger Mode | String | On, Off | Enable/disable hardware trigger |
| Trigger Source | String | Line0, Line1, etc. | Which GPIO line to use |
| Trigger Selector | String | FrameStart, etc. | Trigger action |

### Transport Layer
| Property | Type | Range | Description |
|-----------|------|--------|-------------|
| Packet Size | Integer | 500 - 9000 | Network packet size (USB frames) |
| Packet Delay | Integer | 0 - 10000 | Inter-packet delay in ns |

## Troubleshooting

### Camera Not Detected

**Symptom**: "No Spinnaker cameras detected" error

**Solutions**:
1. Check camera is connected via USB 3.0
2. Try unplugging and reconnecting camera
3. Check USB cable is functional
4. Verify with SpinView: `/opt/spinnaker/bin/SpinView`
5. Check camera status with: `/opt/spinnaker/bin/SpinListCameras`

### Library Loading Errors

**Symptom**: "Library not loaded: @rpath/libSpinnaker.dylib"

**Solutions**:
```bash
# Check library is present
ls -la /opt/spinnaker/lib/libSpinnaker.dylib

# Update adapter library paths
sudo install_name_tool -add_rpath /opt/spinnaker/lib \
    /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib

# Verify
otool -L /usr/local/lib/micro-manager/libmmgr_dal_Spinnaker4.dylib
```

### Image Timeout Errors

**Symptom**: "Failed to get valid image" during capture

**Solutions**:
1. Check exposure time is reasonable (try 10ms)
2. Ensure camera is not in continuous mode elsewhere
3. Stop and restart camera: unplug/replug
4. Check for USB bandwidth issues:
   ```bash
   system_profiler SPUSBDataType
   ```

### Build Errors

**Symptom**: Configure or make fails

**Solutions**:
1. Verify Spinnaker SDK is installed:
   ```bash
   ls -la /opt/spinnaker/include/Spinnaker.h
   ```
2. Check SDK architecture matches (must be ARM64):
   ```bash
   file /opt/spinnaker/lib/libSpinnaker.dylib
   ```
3. Ensure SWIG 3.x is being used:
   ```bash
   swig -version  # Should be 3.0.x
   export SWIG=/usr/local/opt/swig@3/bin/swig
   ```
4. Verify Java version:
   ```bash
   java -version  # Should be 11.x
   echo $JAVA_HOME
   ```

### Performance Issues

**Symptom**: Slow frame rates or dropped frames

**Solutions**:
1. Optimize packet size:
   ```python
   core.setProperty("Camera", "Packet Size", "9000")
   ```
2. Reduce image size with ROI
3. Disable chunk mode for faster transfers:
   ```python
   core.setProperty("Camera", "Chunk Mode Active", "Off")
   ```
4. Use Mono8 instead of Mono12 or RGB8
5. Ensure using USB 3.0 port (check system profiler)

## Performance Tips

1. **Use Mono8 pixel format** - fastest transfer rate
2. **Optimize packet size** - try 9000 (USB 3.0 jumbo frames)
3. **Use ROI** - reduce image size for faster acquisition
4. **Disable chunk data** - reduces overhead
5. **Use hardware triggering** - for precise timing
6. **Allocate buffers once** - avoid reallocation in loops

Example performance optimization:
```python
# Configure for maximum speed
core.setProperty("Camera", "Pixel Format", "Mono8")
core.setProperty("Camera", "Packet Size", "9000")
core.setProperty("Camera", "Chunk Mode Active", "Off")
core.setROI(0, 0, 1024, 768)  # Smaller region
core.setExposure(1.0)  # Short exposure

# Now start streaming
core.startContinuousSequenceAcquisition(0)
```

## Limitations

1. **Platform**: Only macOS Apple Silicon (ARM64) supported
2. **SDK**: Requires Spinnaker SDK 4.x.x (not compatible with 2.3.x)
3. **Architecture**: Must build from source on ARM64 (x86_64 builds won't work)
4. **Binning**: Only 1x and 2x supported
5. **Multiple Cameras**: Single camera only (multi-camera not yet implemented)

## Future Enhancements

- [ ] Multi-camera support
- [ ] Python bindings via PyMMCore
- [ ] Zero-copy buffer support
- [ ] Advanced trigger modes
- [ ] Pixel format conversion options
- [ ] Linux ARM64 support
- [ ] Performance benchmarking tools

## Contributing

Contributions are welcome! Please:

1. Fork the Micro-Manager repository
2. Create a feature branch
3. Make your changes
4. Add tests for new features
5. Submit a pull request

For detailed contribution guidelines, see:
https://micro-manager.org/wiki/Contribute_to_Micro-Manager

## Support

### Documentation
- FLIR Spinnaker SDK: https://softwareservices.flir.com/spinnaker/latest/index.html
- Micro-Manager: https://micro-manager.org/wiki/Micro-Manager
- Device Adapter Guide: https://micro-manager.org/wiki/Micro-Manager_Device_Adapters

### Community
- Micro-Manager Forum: https://micro-manager.org/wiki/Micro-Manager_Community
- GitHub Issues: https://github.com/micro-manager/micro-manager/issues

### FLIR Support
- FLIR Support Portal: https://www.flir.com/support-center/
- Spinnaker SDK Support: https://www.flir.com/products/spinnaker-sdk/

## License

This adapter is part of Micro-Manager and is licensed under the LGPL (Lesser General Public License).

See the Micro-Manager license file for details:
https://github.com/micro-manager/micro-manager/blob/main/doc/copyright.txt

## Acknowledgments

- FLIR Systems for the Spinnaker SDK and documentation
- Micro-Manager development team for the device adapter framework
- The open-source microscopy community for feedback and testing

## Changelog

### Version 1.0.0 (2024)
- Initial release
- macOS Apple Silicon (ARM64) support
- Spinnaker SDK 4.x.x integration
- Basic camera operations (snap, live, sequence)
- ROI support
- Hardware triggering
- Exposure and gain control
