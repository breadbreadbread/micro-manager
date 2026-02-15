/**
 * Micro-Manager Device Adapter for FLIR Spinnaker SDK 4.x.x
 * macOS Apple Silicon (ARM64) Support
 *
 * Implementation file
 */

#include "Spinnaker4Camera.h"
#include <sstream>
#include <cmath>

// Device name and description
const char* g_CameraName = Spinnaker4Camera::DeviceName();
const char* g_CameraDescription = Spinnaker4Camera::Description();

// Module interface
MODULE_API void InitializeModuleData()
{
    RegisterDevice(g_CameraName, MM::CameraDevice, g_CameraDescription);
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
    if (deviceName == 0)
        return 0;

    std::string name(deviceName);

    if (name == g_CameraName)
    {
        return new Spinnaker4Camera();
    }

    return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
    delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////
// Spinnaker4Camera implementation
///////////////////////////////////////////////////////////////////////////////

Spinnaker4Camera::Spinnaker4Camera() :
    CCameraBase<Spinnaker4Camera>(),
    initialized_(false),
    acquiring_(false),
    sequenceAcquiring_(false),
    sequenceNumImages_(0),
    sequenceImageCount_(0),
    imageWidth_(0),
    imageHeight_(0),
    imageBytesPerPixel_(1),
    imageBitDepth_(8),
    imageBuffer_(0),
    imageBufferSize_(0),
    roiX_(0),
    roiY_(0),
    roiWidth_(0),
    roiHeight_(0),
    roiEnabled_(false),
    exposureMs_(10.0),
    binning_(1),
    stopThread_(true)
{
    // Initialize Spinnaker system
    try {
        system_ = System::GetInstance();
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Failed to initialize Spinnaker system: ") + e.what());
    }

    // Set initial single-byte transfer mode (for metadata)
    SetProperty(MM::g_Keyword_Metadata_NumChannels, 1);
}

Spinnaker4Camera::~Spinnaker4Camera()
{
    Shutdown();

    if (imageBuffer_)
    {
        delete[] imageBuffer_;
        imageBuffer_ = 0;
    }

    if (system_)
    {
        system_->ReleaseInstance();
        system_ = 0;
    }
}

int Spinnaker4Camera::Initialize()
{
    if (initialized_)
        return DEVICE_OK;

    CPropertyAction* pAct = new CPropertyAction(this, &Spinnaker4Camera::OnBinningChanged);
    CreateProperty(MM::g_Keyword_Binning, "1", MM::Integer, false, pAct);
    AddAllowedValue(MM::g_Keyword_Binning, "1");
    AddAllowedValue(MM::g_Keyword_Binning, "2");

    // Enumerate cameras
    int ret = EnumerateCameras();
    if (ret != DEVICE_OK)
        return ret;

    // Select first camera
    ret = SelectCamera(0);
    if (ret != DEVICE_OK)
        return ret;

    // Get camera info
    ret = GetCameraInfo();
    if (ret != DEVICE_OK)
        return ret;

    // Initialize camera
    try {
        GenICam::gcstring errorMessage;
        camera_->Init(errorMessage);

        if (errorMessage != "") {
            LogMessage(std::string("Camera initialization warning: ") +
                      errorMessage.c_str());
        }
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Failed to initialize camera: ") + e.what());
        return DEVICE_NATIVE_MODULE_FAILED;
    }

    // Configure camera
    ret = ConfigureCamera();
    if (ret != DEVICE_OK)
        return ret;

    // Create device properties
    ret = CreateDeviceProperties();
    if (ret != DEVICE_OK)
        return ret;

    // Cache node properties
    ret = CacheNodeProperties();
    if (ret != DEVICE_OK)
        return ret;

    initialized_ = true;

    // Get current image dimensions
    CIntegerPtr ptrWidth = camera_->GetNodeMap().GetNode("Width");
    CIntegerPtr ptrHeight = camera_->GetNodeMap().GetNode("Height");

    if (IsAvailable(ptrWidth) && IsReadable(ptrWidth))
        imageWidth_ = static_cast<unsigned>(ptrWidth->GetValue());

    if (IsAvailable(ptrHeight) && IsReadable(ptrHeight))
        imageHeight_ = static_cast<unsigned>(ptrHeight->GetValue());

    // Allocate image buffer
    imageBufferSize_ = imageWidth_ * imageHeight_ * imageBytesPerPixel_;
    if (imageBuffer_)
        delete[] imageBuffer_;
    imageBuffer_ = new unsigned char[imageBufferSize_];

    return DEVICE_OK;
}

int Spinnaker4Camera::Shutdown()
{
    if (!initialized_)
        return DEVICE_OK;

    StopSequenceAcquisition();

    if (camera_)
    {
        try {
            camera_->DeInit();
        }
        catch (Spinnaker::Exception& e) {
            LogMessage(std::string("Error deinitializing camera: ") + e.what());
        }
    }

    initialized_ = false;
    return DEVICE_OK;
}

int Spinnaker4Camera::EnumerateCameras()
{
    try {
        cameraList_ = system_->GetCameras();

        unsigned int numCameras = cameraList_.GetSize();

        if (numCameras == 0)
        {
            LogMessage("No Spinnaker cameras detected");
            return DEVICE_NOT_CONNECTED;
        }

        std::ostringstream msg;
        msg << "Found " << numCameras << " Spinnaker camera(s)";
        LogMessage(msg.str());

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error enumerating cameras: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::SelectCamera(int index)
{
    try {
        camera_ = cameraList_.GetByIndex(index);

        if (!camera_ || !camera_->IsValid())
        {
            LogMessage("Invalid camera selected");
            return DEVICE_INVALID_PARAMETER;
        }

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error selecting camera: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::GetCameraInfo()
{
    try {
        // Get device information
        CNodePtr ptrDeviceSerialNumber = camera_->GetTLDeviceNodeMap().GetNode("DeviceSerialNumber");
        if (IsAvailable(ptrDeviceSerialNumber) && IsReadable(ptrDeviceSerialNumber))
        {
            serialNumber_ = GetNodeSymbolic(ptrDeviceSerialNumber);
        }

        CNodePtr ptrDeviceModelName = camera_->GetTLDeviceNodeMap().GetNode("DeviceModelName");
        if (IsAvailable(ptrDeviceModelName) && IsReadable(ptrDeviceModelName))
        {
            cameraModel_ = GetNodeSymbolic(ptrDeviceModelName);
        }

        cameraName_ = "Spinnaker4_";
        cameraName_ += serialNumber_;

        LogMessage(std::string("Camera Model: ") + cameraModel_);
        LogMessage(std::string("Serial Number: ") + serialNumber_);

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error getting camera info: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::ConfigureCamera()
{
    int ret = ConfigureAcquisitionMode();
    if (ret != DEVICE_OK)
        return ret;

    ret = ConfigurePixelFormat();
    if (ret != DEVICE_OK)
        return ret;

    ret = ConfigureTriggering();
    if (ret != DEVICE_OK)
        return ret;

    return DEVICE_OK;
}

int Spinnaker4Camera::ConfigureAcquisitionMode()
{
    try {
        // Set acquisition mode to continuous
        CEnumerationPtr ptrAcquisitionMode = camera_->GetNodeMap().GetNode("AcquisitionMode");
        if (IsAvailable(ptrAcquisitionMode) && IsWritable(ptrAcquisitionMode))
        {
            CEnumEntryPtr ptrAcquisitionModeContinuous =
                ptrAcquisitionMode->GetEntryByName("Continuous");
            if (IsAvailable(ptrAcquisitionModeContinuous) && IsReadable(ptrAcquisitionModeContinuous))
            {
                ptrAcquisitionMode->SetIntValue(ptrAcquisitionModeContinuous->GetValue());
                LogMessage("Acquisition mode set to Continuous");
            }
        }

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error configuring acquisition mode: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::ConfigurePixelFormat()
{
    try {
        // Set pixel format to Mono8
        CEnumerationPtr ptrPixelFormat = camera_->GetNodeMap().GetNode("PixelFormat");
        if (IsAvailable(ptrPixelFormat) && IsWritable(ptrPixelFormat))
        {
            CEnumEntryPtr ptrPixelFormatMono8 = ptrPixelFormat->GetEntryByName("Mono8");
            if (IsAvailable(ptrPixelFormatMono8) && IsReadable(ptrPixelFormatMono8))
            {
                ptrPixelFormat->SetIntValue(ptrPixelFormatMono8->GetValue());
                LogMessage("Pixel format set to Mono8");
                imageBytesPerPixel_ = 1;
                imageBitDepth_ = 8;
            }
        }

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error configuring pixel format: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::ConfigureTriggering()
{
    try {
        // Disable triggering initially (software trigger)
        CEnumerationPtr ptrTriggerMode = camera_->GetNodeMap().GetNode("TriggerMode");
        if (IsAvailable(ptrTriggerMode) && IsWritable(ptrTriggerMode))
        {
            CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
            if (IsAvailable(ptrTriggerModeOff) && IsReadable(ptrTriggerModeOff))
            {
                ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());
                LogMessage("Trigger mode disabled");
            }
        }

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error configuring triggering: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::CreateDeviceProperties()
{
    // TODO: Create properties for:
    // - Exposure time
    // - Gain
    // - Frame rate
    // - Pixel format
    // - Trigger mode
    // - Trigger source

    return DEVICE_OK;
}

int Spinnaker4Camera::CacheNodeProperties()
{
    // TODO: Cache camera node map properties for quick access

    return DEVICE_OK;
}

int Spinnaker4Camera::SnapImage()
{
    if (!initialized_)
        return DEVICE_NOT_CONNECTED;

    return AcquireImage();
}

int Spinnaker4Camera::AcquireImage()
{
    try {
        if (!acquiring_)
            StartAcquisition();

        // Get next image with timeout (5 seconds)
        latestImage_ = camera_->GetNextImage(5000);

        if (!latestImage_->IsValid())
        {
            LogMessage("Failed to get valid image");
            StopAcquisition();
            return DEVICE_ERR;
        }

        // Convert to Mono8 if necessary
        int ret = ConvertImage(latestImage_, convertedImage_);
        if (ret != DEVICE_OK)
        {
            latestImage_->Release();
            StopAcquisition();
            return ret;
        }

        // Extract image data
        ret = ExtractImageData(convertedImage_);
        if (ret != DEVICE_OK)
        {
            latestImage_->Release();
            StopAcquisition();
            return ret;
        }

        // Release original image
        latestImage_->Release();

        StopAcquisition();

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error acquiring image: ") + e.what());
        StopAcquisition();
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::ConvertImage(ImagePtr& image, ImagePtr& converted)
{
    try {
        // Convert to Mono8 for compatibility
        converted = image->Convert(PixelFormat_Mono8, HQ_LINEAR);
        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error converting image: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::ExtractImageData(ImagePtr image)
{
    try {
        // Get image dimensions
        unsigned width = image->GetWidth();
        unsigned height = image->GetHeight();
        unsigned size = width * height;

        // Check if buffer needs to be reallocated
        if (size > imageBufferSize_)
        {
            if (imageBuffer_)
                delete[] imageBuffer_;
            imageBufferSize_ = size;
            imageBuffer_ = new unsigned char[imageBufferSize_];
        }

        // Copy image data
        unsigned char* data = static_cast<unsigned char*>(image->GetData());
        memcpy(imageBuffer_, data, size);

        imageWidth_ = width;
        imageHeight_ = height;

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error extracting image data: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::StartAcquisition()
{
    try {
        camera_->BeginAcquisition();
        acquiring_ = true;
        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error starting acquisition: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::StopAcquisition()
{
    try {
        if (acquiring_)
        {
            camera_->EndAcquisition();
            acquiring_ = false;
        }
        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error stopping acquisition: ") + e.what());
        return DEVICE_ERR;
    }
}

const unsigned char* Spinnaker4Camera::GetImageBuffer()
{
    return imageBuffer_;
}

unsigned Spinnaker4Camera::GetImageWidth() const
{
    return imageWidth_;
}

unsigned Spinnaker4Camera::GetImageHeight() const
{
    return imageHeight_;
}

unsigned Spinnaker4Camera::GetImageBytesPerPixel() const
{
    return imageBytesPerPixel_;
}

long Spinnaker4Camera::GetImageBufferSize() const
{
    return imageBufferSize_;
}

unsigned Spinnaker4Camera::GetBitDepth() const
{
    return imageBitDepth_;
}

int Spinnaker4Camera::GetBinning() const
{
    return binning_;
}

int Spinnaker4Camera::SetBinning(int binSize)
{
    if (binSize != 1 && binSize != 2)
        return DEVICE_UNSUPPORTED_BINNING;

    binning_ = binSize;
    return DEVICE_OK;
}

void Spinnaker4Camera::SetExposure(double exposureMs)
{
    exposureMs_ = exposureMs;

    try {
        CFloatPtr ptrExposureTime = camera_->GetNodeMap().GetNode("ExposureTime");
        if (IsAvailable(ptrExposureTime) && IsWritable(ptrExposureTime))
        {
            // Convert to microseconds
            ptrExposureTime->SetValue(exposureMs_ * 1000.0);
        }
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error setting exposure: ") + e.what());
    }
}

double Spinnaker4Camera::GetExposure()
{
    try {
        CFloatPtr ptrExposureTime = camera_->GetNodeMap().GetNode("ExposureTime");
        if (IsAvailable(ptrExposureTime) && IsReadable(ptrExposureTime))
        {
            // Convert from microseconds
            exposureMs_ = ptrExposureTime->GetValue() / 1000.0;
        }
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error getting exposure: ") + e.what());
    }

    return exposureMs_;
}

int Spinnaker4Camera::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize)
{
    try {
        CIntegerPtr ptrOffsetX = camera_->GetNodeMap().GetNode("OffsetX");
        CIntegerPtr ptrOffsetY = camera_->GetNodeMap().GetNode("OffsetY");
        CIntegerPtr ptrWidth = camera_->GetNodeMap().GetNode("Width");
        CIntegerPtr ptrHeight = camera_->GetNodeMap().GetNode("Height");

        if (!IsAvailable(ptrOffsetX) || !IsWritable(ptrOffsetX) ||
            !IsAvailable(ptrOffsetY) || !IsWritable(ptrOffsetY) ||
            !IsAvailable(ptrWidth) || !IsWritable(ptrWidth) ||
            !IsAvailable(ptrHeight) || !IsWritable(ptrHeight))
        {
            return DEVICE_NOT_SUPPORTED;
        }

        // Get sensor dimensions
        CIntegerPtr ptrSensorWidth = camera_->GetNodeMap().GetNode("SensorWidth");
        CIntegerPtr ptrSensorHeight = camera_->GetNodeMap().GetNode("SensorHeight");

        if (!IsAvailable(ptrSensorWidth) || !IsReadable(ptrSensorWidth) ||
            !IsAvailable(ptrSensorHeight) || !IsReadable(ptrSensorHeight))
        {
            return DEVICE_ERR;
        }

        int64_t sensorWidth = ptrSensorWidth->GetValue();
        int64_t sensorHeight = ptrSensorHeight->GetValue();

        // Validate ROI
        if (x + xSize > sensorWidth || y + ySize > sensorHeight)
            return DEVICE_INVALID_ROI;

        // Set ROI
        ptrOffsetX->SetValue(x);
        ptrOffsetY->SetValue(y);
        ptrWidth->SetValue(xSize);
        ptrHeight->SetValue(ySize);

        roiX_ = x;
        roiY_ = y;
        roiWidth_ = xSize;
        roiHeight_ = ySize;
        roiEnabled_ = true;

        // Reallocate buffer
        imageBufferSize_ = xSize * ySize * imageBytesPerPixel_;
        if (imageBuffer_)
            delete[] imageBuffer_;
        imageBuffer_ = new unsigned char[imageBufferSize_];

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error setting ROI: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize)
{
    if (!roiEnabled_)
    {
        x = 0;
        y = 0;
        xSize = imageWidth_;
        ySize = imageHeight_;
        return DEVICE_OK;
    }

    x = roiX_;
    y = roiY_;
    xSize = roiWidth_;
    ySize = roiHeight_;

    return DEVICE_OK;
}

int Spinnaker4Camera::ClearROI()
{
    try {
        // Reset to full sensor
        CIntegerPtr ptrSensorWidth = camera_->GetNodeMap().GetNode("SensorWidth");
        CIntegerPtr ptrSensorHeight = camera_->GetNodeMap().GetNode("SensorHeight");

        if (!IsAvailable(ptrSensorWidth) || !IsReadable(ptrSensorWidth) ||
            !IsAvailable(ptrSensorHeight) || !IsReadable(ptrSensorHeight))
        {
            return DEVICE_ERR;
        }

        int64_t sensorWidth = ptrSensorWidth->GetValue();
        int64_t sensorHeight = ptrSensorHeight->GetValue();

        return SetROI(0, 0, sensorWidth, sensorHeight);
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error clearing ROI: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::StartSequenceAcquisition(long numImages, double intervalMs)
{
    if (sequenceAcquiring_)
        return DEVICE_CAMERA_BUSY_ACQUIRING;

    sequenceNumImages_ = numImages;
    sequenceImageCount_ = 0;
    sequenceAcquiring_ = true;
    stopThread_ = false;

    // Start acquisition thread
    int ret = acquisitionThread_.Start(AcquisitionThreadFunction, this);
    if (ret != DEVICE_OK)
    {
        sequenceAcquiring_ = false;
        return ret;
    }

    return DEVICE_OK;
}

int Spinnaker4Camera::StopSequenceAcquisition()
{
    if (!sequenceAcquiring_)
        return DEVICE_OK;

    stopThread_ = true;
    acquisitionThread_.Wait();

    sequenceAcquiring_ = false;
    sequenceImageCount_ = 0;

    StopAcquisition();

    return DEVICE_OK;
}

int Spinnaker4Camera::PrepareSequenceAcquisition()
{
    return DEVICE_OK;
}

int Spinnaker4Camera::StartTriggeredSequenceAcquisition()
{
    // For hardware triggered sequences
    return DEVICE_OK;
}

int Spinnaker4Camera::AcquisitionThreadFunction(void* device)
{
    Spinnaker4Camera* camera = static_cast<Spinnaker4Camera*>(device);
    return camera->AcquisitionThread();
}

int Spinnaker4Camera::AcquisitionThread()
{
    try {
        StartAcquisition();

        while (!stopThread_ &&
               (sequenceNumImages_ == 0 || sequenceImageCount_ < sequenceNumImages_))
        {
            // Get next image with timeout
            latestImage_ = camera_->GetNextImage(1000);

            if (latestImage_->IsValid())
            {
                // Convert and extract image
                ConvertImage(latestImage_, convertedImage_);
                ExtractImageData(convertedImage_);
                latestImage_->Release();

                // Notify Micro-Manager
                InsertImage();

                sequenceImageCount_++;
            }
        }

        StopAcquisition();
        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error in acquisition thread: ") + e.what());
        StopAcquisition();
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::SetProperty(const char* name, const char* value)
{
    // TODO: Implement property setting
    return DEVICE_OK;
}

int Spinnaker4Camera::GetProperty(const char* name, char* value)
{
    // TODO: Implement property getting
    return DEVICE_OK;
}

int Spinnaker4Camera::GetProperty(const char* name, double& value)
{
    // TODO: Implement property getting
    return DEVICE_OK;
}

int Spinnaker4Camera::GetProperty(const char* name, long& value)
{
    // TODO: Implement property getting
    return DEVICE_OK;
}

int Spinnaker4Camera::SetProperty(const char* name, double value)
{
    // TODO: Implement property setting
    return DEVICE_OK;
}

int Spinnaker4Camera::SetProperty(const char* name, long value)
{
    // TODO: Implement property setting
    return DEVICE_OK;
}

int Spinnaker4Camera::OnBinningChanged(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        // Nothing to do
    }
    else if (eAct == MM::AfterSet)
    {
        // Update binning
        long bin = 1;
        pProp->Get(bin);
        SetBinning(bin);
    }

    return DEVICE_OK;
}

void Spinnaker4Camera::GetCoreErrorText(int errorCode, char* text, size_t nSize)
{
    CDeviceUtils::GetDefaultErrorText(errorCode, text, nSize);
}

// Utility methods
std::string Spinnaker4Camera::GetNodeSymbolic(CNodePtr node)
{
    try {
        CStringPtr ptrString = node;
        if (IsAvailable(ptrString) && IsReadable(ptrString))
        {
            return std::string(ptrString->GetValue().c_str());
        }
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error getting node symbolic: ") + e.what());
    }

    return "";
}

bool Spinnaker4Camera::IsNodeAvailable(CNodePtr node)
{
    return IsAvailable(node);
}

bool Spinnaker4Camera::IsNodeWritable(CNodePtr node)
{
    return IsAvailable(node) && IsWritable(node);
}

bool Spinnaker4Camera::IsNodeReadable(CNodePtr node)
{
    return IsAvailable(node) && IsReadable(node);
}

int Spinnaker4Camera::UpdatePropertyFromNode(const char* propertyName, CNodePtr node)
{
    // TODO: Update property cache from node
    return DEVICE_OK;
}

int Spinnaker4Camera::SetNodeFromProperty(CNodePtr node, const char* value)
{
    // TODO: Set node from property value
    return DEVICE_OK;
}

int Spinnaker4Camera::CheckNodeAvailability(CNodePtr node)
{
    if (!IsNodeAvailable(node))
        return DEVICE_NOT_SUPPORTED;
    return DEVICE_OK;
}

int Spinnaker4Camera::CheckNodeWritable(CNodePtr node)
{
    if (!IsNodeWritable(node))
        return DEVICE_CAN_NOT_SET_PROPERTY;
    return DEVICE_OK;
}

int Spinnaker4Camera::SetTriggerMode(bool enabled)
{
    try {
        CEnumerationPtr ptrTriggerMode = camera_->GetNodeMap().GetNode("TriggerMode");
        if (!IsAvailable(ptrTriggerMode) || !IsWritable(ptrTriggerMode))
            return DEVICE_NOT_SUPPORTED;

        if (enabled)
        {
            CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
            if (IsAvailable(ptrTriggerModeOn) && IsReadable(ptrTriggerModeOn))
            {
                ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());
            }
        }
        else
        {
            CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
            if (IsAvailable(ptrTriggerModeOff) && IsReadable(ptrTriggerModeOff))
            {
                ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());
            }
        }

        return DEVICE_OK;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error setting trigger mode: ") + e.what());
        return DEVICE_ERR;
    }
}

int Spinnaker4Camera::SetTriggerSource(const char* source)
{
    try {
        CEnumerationPtr ptrTriggerSource = camera_->GetNodeMap().GetNode("TriggerSource");
        if (!IsAvailable(ptrTriggerSource) || !IsWritable(ptrTriggerSource))
            return DEVICE_NOT_SUPPORTED;

        CEnumEntryPtr ptrTriggerSourceEntry = ptrTriggerSource->GetEntryByName(source);
        if (IsAvailable(ptrTriggerSourceEntry) && IsReadable(ptrTriggerSourceEntry))
        {
            ptrTriggerSource->SetIntValue(ptrTriggerSourceEntry->GetValue());
            return DEVICE_OK;
        }

        return DEVICE_INVALID_PROPERTY_VALUE;
    }
    catch (Spinnaker::Exception& e) {
        LogMessage(std::string("Error setting trigger source: ") + e.what());
        return DEVICE_ERR;
    }
}
