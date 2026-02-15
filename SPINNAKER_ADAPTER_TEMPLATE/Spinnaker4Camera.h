/**
 * Micro-Manager Device Adapter for FLIR Spinnaker SDK 4.x.x
 * macOS Apple Silicon (ARM64) Support
 *
 * This adapter provides support for FLIR Blackfly and other Spinnaker-based cameras
 * on macOS Apple Silicon using Spinnaker SDK 4.x.x.
 *
 * Camera Class Implementation
 */

#ifndef _SPINNAKER4_CAMERA_H_
#define _SPINNAKER4_CAMERA_H_

#include "MMDevice.h"
#include "DeviceUtils.h"
#include "Spinnaker.h"

#include <string>
#include <map>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class Spinnaker4Camera : public CCameraBase<Spinnaker4Camera>
{
public:
    Spinnaker4Camera();
    ~Spinnaker4Camera();

    // MMCore API
    int Initialize();
    int Shutdown();

    // Image acquisition
    int SnapImage();
    const unsigned char* GetImageBuffer();
    unsigned GetImageWidth() const;
    unsigned GetImageHeight() const;
    unsigned GetImageBytesPerPixel() const;
    long GetImageBufferSize() const;
    unsigned GetBitDepth() const;
    int GetBinning() const;
    int SetBinning(int binSize);
    void SetExposure(double exposureMs);
    double GetExposure();
    int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize);
    int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize);
    int ClearROI();
    int IsExposureSequenceable(bool& isSequenceable) const { isSequenceable = false; return DEVICE_OK; }

    // Property interface
    int SetProperty(const char* name, const char* value);
    int GetProperty(const char* name, char* value);
    int GetProperty(const char* name, double& value);
    int GetProperty(const char* name, long& value);
    int SetProperty(const char* name, double value);
    int SetProperty(const char* name, long value);

    // Action interface
    int OnBinningChanged(MM::PropertyBase* pProp, MM::ActionType eAct);

    // Sequence acquisition
    int StartSequenceAcquisition(long numImages, double intervalMs);
    int StopSequenceAcquisition();
    int PrepareSequenceAcquisition();
    int StartTriggeredSequenceAcquisition();

    // Buffer management
    void GetCoreErrorText(int errorCode, char* text, size_t nSize);

    // Name and description
    static const char* DeviceName() { return "Spinnaker4"; }
    static const char* Description() { return "FLIR Spinnaker SDK 4.x.x Camera Adapter"; }

protected:
    // Device properties
    int CreateDeviceProperties();
    int UpdatePropertyCache();

    // Camera operations
    int ConfigureCamera();
    int ConfigureAcquisitionMode();
    int ConfigureTriggering();
    int ConfigurePixelFormat();
    int StartAcquisition();
    int StopAcquisition();

    // Image handling
    int AcquireImage();
    int ConvertImage(ImagePtr& image, ImagePtr& converted);
    int ExtractImageData(ImagePtr image);

private:
    // Spinnaker SDK objects
    SystemPtr system_;
    CameraList_t cameraList_;
    CameraPtr camera_;
    ImagePtr latestImage_;
    ImagePtr convertedImage_;

    // Camera state
    bool initialized_;
    bool acquiring_;
    bool sequenceAcquiring_;
    long sequenceNumImages_;
    long sequenceImageCount_;

    // Image parameters
    unsigned imageWidth_;
    unsigned imageHeight_;
    unsigned imageBytesPerPixel_;
    unsigned imageBitDepth_;
    unsigned char* imageBuffer_;
    long imageBufferSize_;

    // ROI parameters
    unsigned roiX_;
    unsigned roiY_;
    unsigned roiWidth_;
    unsigned roiHeight_;
    bool roiEnabled_;

    // Exposure
    double exposureMs_;

    // Binning
    int binning_;

    // Camera name/model
    std::string cameraName_;
    std::string cameraModel_;
    std::string serialNumber_;

    // Property cache
    std::map<std::string, std::string> propertyCache_;

    // Thread for continuous acquisition
    MM::MMThread acquisitionThread_;
    MM::MMEventLock threadLock_;
    bool stopThread_;

    // Private methods
    int EnumerateCameras();
    int SelectCamera(int index);
    int GetCameraInfo();
    int UpdatePropertyFromNode(const char* propertyName, CNodePtr node);
    int SetNodeFromProperty(CNodePtr node, const char* value);
    int CacheNodeProperties();
    int CheckNodeAvailability(CNodePtr node);
    int CheckNodeWritable(CNodePtr node);

    // Trigger management
    int SetTriggerMode(bool enabled);
    int SetTriggerSource(const char* source);

    // Utility methods
    std::string GetNodeSymbolic(CNodePtr node);
    bool IsNodeAvailable(CNodePtr node);
    bool IsNodeWritable(CNodePtr node);
    bool IsNodeReadable(CNodePtr node);

    // Acquisition thread function
    static int AcquisitionThreadFunction(void* device);
    int AcquisitionThread();
};

#endif // _SPINNAKER4_CAMERA_H_
