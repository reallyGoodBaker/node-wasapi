#include <string>
#include <optional>
#include <vector>
#include "util.cc"
#include <audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <assert.h>
#include <iostream>
#include <functional>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IMMDevice = __uuidof(IMMDevice);

HRESULT ghr;

// Helper macro for SAFE_RELEASE
#define SAFE_RELEASE(punk)     \
    {                          \
        if (punk)              \
        {                      \
            (punk)->Release(); \
            (punk) = NULL;     \
        }                      \
    }

#define EXIT_ON_ERROR()  \
    if (FAILED(ghr)) { goto Exit; }

typedef std::function<UINT32 (BYTE**, UINT32)> NeedDataHandler;
typedef std::function<void (
    BYTE**,
    UINT32,
    DWORD,
    UINT64,
    UINT64
)> RecvDataHandler;

class Wasapi
{
private:
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pOutDevice = NULL;
    IMMDevice *pInDevice = NULL;
    IAudioClient *pRenderClient = NULL;
    IAudioClient *pCaptureClient = NULL;
    IAudioRenderClient *renderService = NULL;
    IAudioCaptureClient *captureService = NULL;
    WAVEFORMATEX *owfx = NULL;
    WAVEFORMATEX *iwfx = NULL;
    RecvDataHandler cbi = NULL;
    NeedDataHandler cbo = NULL;
    bool serving = false;

public:
    const int32_t A_OUT = 1;
    const int32_t A_IN = 2;

    void Release();
    bool InitCOM();
    bool InitDevice(std::optional<std::string> sout, std::optional<std::string> sin, std::optional<int32_t> accessibility);
    bool GetDevices(EDataFlow dataFlow, IMMDeviceCollection **devices);
    bool InitRenderService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration);
    bool InitCaptureService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration);
    bool Start(int32_t type);
    bool Stop(int32_t type);
    bool GetFormat(int32_t type, WAVEFORMATEX **wfx);
    bool GetFormatEx(int32_t type, WAVEFORMATEXTENSIBLE **wfx);
    bool SetNeedDataHandler(NeedDataHandler cb);
    bool SetRecvDataHandler(RecvDataHandler cb);
    void Service(UINT32 nFramesRender);
    void StopService();
    
};

bool Wasapi::InitCOM()
{
    if (FAILED(ghr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        return false;
    }

    if (FAILED(
            ghr = CoCreateInstance(
                CLSID_MMDeviceEnumerator,
                NULL,
                CLSCTX_ALL,
                IID_IMMDeviceEnumerator,
                (void **)&pEnumerator)))
    {
        return false;
    }

    return true;
}

void Wasapi::Release()
{
    CoTaskMemFree(iwfx);
    CoTaskMemFree(owfx);
    SAFE_RELEASE(pEnumerator);
    SAFE_RELEASE(pInDevice);
    SAFE_RELEASE(pOutDevice);
    SAFE_RELEASE(pCaptureClient);
    SAFE_RELEASE(pRenderClient);
    SAFE_RELEASE(renderService);
    SAFE_RELEASE(captureService);
    CoUninitialize();
}

bool Wasapi::InitDevice(std::optional<std::string> sout, std::optional<std::string> sin, std::optional<int32_t> accessibility)
{
    auto acc = accessibility.value_or(A_OUT);

    if (acc & A_OUT)
    {
        if (FAILED(ghr = sout.has_value()
                ? pEnumerator->GetDevice(str2wstr(sout.value()), &pOutDevice)
                : pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pOutDevice)))
        {
            return false;
        }

        if (FAILED(ghr = pOutDevice->Activate(
                IID_IAudioClient,
                CLSCTX_ALL,
                NULL,
                (void **)&pRenderClient)))
        {
            return false;
        }

        if (FAILED(ghr = pRenderClient->GetMixFormat(&owfx)))
        {
            return false;
        }
    }

    if (acc & A_IN)
    {
        if (FAILED(ghr = sin.has_value()
                ? pEnumerator->GetDevice(str2wstr(sin.value()), &pInDevice)
                : pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pInDevice)))
        {
            return false;
        }

        if (FAILED(ghr = pInDevice->Activate(
                IID_IAudioClient,
                CLSCTX_ALL,
                NULL,
                (void **)&pCaptureClient)))
        {
            return false;
        }

        if (FAILED(ghr = pCaptureClient->GetMixFormat(&iwfx)))
        {
            return false;
        }
        
    }

    return true;
}

bool Wasapi::GetDevices(EDataFlow dataFlow, IMMDeviceCollection **devices)
{
    return SUCCEEDED(
        ghr = pEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, devices));
}

bool Wasapi::InitRenderService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration)
{
    if (FAILED(ghr = pRenderClient->Initialize(
            shareMode,
            0,
            bufferDuration,
            0,
            owfx,
            NULL)))
    {
        return false;
    }

    if (FAILED(ghr = pRenderClient->GetService(IID_IAudioRenderClient, (void **)&renderService)))
    {
        return false;
    }

    return true;
}

bool Wasapi::InitCaptureService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration)
{
    if (FAILED(ghr = pCaptureClient->Initialize(
            shareMode,
            0,
            bufferDuration,
            0,
            iwfx,
            NULL)))
    {
        return false;
    }

    if (FAILED(ghr = pCaptureClient->GetService(IID_IAudioCaptureClient, (void **)&captureService)))
    {
        return false;
    }

    return true;
}

bool Wasapi::Start(int32_t type)
{
    if (type & A_IN)
    {
        if (FAILED(ghr = pCaptureClient->Start()))
        {
            return false;
        }
        
    }

    if (type & A_OUT)
    {
        if (FAILED(ghr = pRenderClient->Start()))
        {
            return false;
        }
    }
    
    return true;
}

bool Wasapi::Stop(int32_t type)
{
    if (type & A_IN)
    {
        if (FAILED(ghr = pCaptureClient->Stop()))
        {
            return false;
        }
        
    }

    if (type & A_OUT)
    {
        if (FAILED(ghr = pRenderClient->Stop()))
        {
            return false;
        }
    }
    
    return true;
}

bool Wasapi::GetFormat(int32_t type, WAVEFORMATEX **wfx)
{
    switch (type)
    {
    case 1:
        *wfx = owfx;
        return true;

    case 2:
        *wfx = iwfx;
        return true;

    default:
        return false;
    }
}

bool Wasapi::GetFormatEx(int32_t type, WAVEFORMATEXTENSIBLE **wfx)
{
    switch (type)
    {
    case 1:
        *wfx = (WAVEFORMATEXTENSIBLE *)owfx;
        return true;

    case 2:
        *wfx = (WAVEFORMATEXTENSIBLE *)iwfx;
        return true;

    default:
        return false;
    }
}

bool Wasapi::SetNeedDataHandler(NeedDataHandler cb)
{
    if (pRenderClient && renderService)
    {
        cbo = cb;
        return true;
    }

    return false;
}

bool Wasapi::SetRecvDataHandler(RecvDataHandler cb)
{
    if (pCaptureClient && captureService)
    {
        cbi = cb;
        return true;
    }

    return false;
}

void Wasapi::Service(UINT32 nFramesRender)
{
    UINT32 framesToRead;
    BYTE* dataCaptured;
    DWORD pdwFlags;
    UINT64 pu64DevicePosition;
    UINT64 pu64QPCPosition;

    UINT32 nframeWritten;
    UINT32 rFramesSize;
    UINT32 rFramesPadding;
    BYTE* dataToRender;

    serving = true;

    while (serving)
    {
        if (pCaptureClient && captureService)
        {
            ghr = captureService->GetBuffer(
                &dataCaptured,
                &framesToRead,
                &pdwFlags,
                &pu64DevicePosition,
                &pu64QPCPosition
            );
            EXIT_ON_ERROR();

            if (cbi)
                cbi(&dataCaptured, framesToRead, pdwFlags, pu64DevicePosition, pu64QPCPosition);

            ghr = captureService->ReleaseBuffer(framesToRead);
            EXIT_ON_ERROR();
        }

        if (pRenderClient && renderService)
        {
            ghr = pRenderClient->GetBufferSize(&rFramesSize);
            EXIT_ON_ERROR();
            ghr = pRenderClient->GetCurrentPadding(&rFramesPadding);
            EXIT_ON_ERROR();

            auto rFramesFree = rFramesSize - rFramesPadding;
            if (rFramesFree < nFramesRender)
                continue;

            ghr = renderService->GetBuffer(nFramesRender, &dataToRender);
            EXIT_ON_ERROR();

            if (cbo)
                nframeWritten = cbo(&dataToRender, rFramesFree);

            ghr = renderService->ReleaseBuffer(nframeWritten, 0);
            EXIT_ON_ERROR();
        }
        
    }

    if (false)
    {
        Exit:
        std::cout << "Error: " << ghr << std::endl;
        return;
    }
    
}

void Wasapi::StopService()
{
    serving = false;
}