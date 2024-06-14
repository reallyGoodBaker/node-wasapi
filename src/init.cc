#include <string>
#include <optional>
#include <vector>
#include "util.cc"
#include "audioclient.h"
#include "audiopolicy.h"
#include "mmdeviceapi.h"
#include "assert.h"
#include <iostream>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

// Helper macro for SAFE_RELEASE
#define SAFE_RELEASE(punk)     \
    {                          \
        if (punk)              \
        {                      \
            (punk)->Release(); \
            (punk) = NULL;     \
        }                      \
    }


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

public:
    const int A_OUT = 1;
    const int A_IN = 2;

    void Release();
    bool InitCOM();    
    bool InitDevice(std::optional<std::string> sout, std::optional<std::string> sin, std::optional<int32_t> accessibility);
    bool GetDevices(EDataFlow dataFlow, IMMDeviceCollection** devices);
    bool InitRenderService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration);
    bool InitCaptureService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration);
    bool startRender();
    bool startCapture();
    bool stopRender();
    bool stopCapture();
};


bool Wasapi::InitCOM()
{
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        return false;
    }

    if (FAILED(
        CoCreateInstance(
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
    CoTaskMemFree(pwfx);
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
    HRESULT rInitRenderer, rInitRecorder = rInitRenderer = S_OK;
    auto acc = accessibility.value_or(A_OUT);

    if (acc & A_OUT)
    {
        rInitRenderer = sout.has_value()
            ? pEnumerator->GetDevice(str2wstr(sout.value()), &pOutDevice)
            : pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pOutDevice);
    }

    if (acc & A_IN)
    {
        rInitRecorder = sin.has_value()
            ? pEnumerator->GetDevice(str2wstr(sin.value()), &pInDevice)
            : pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pInDevice);
    }

    SAFE_RELEASE(pEnumerator);

    return SUCCEEDED(rInitRenderer) && SUCCEEDED(rInitRecorder);
}

bool Wasapi::GetDevices(EDataFlow dataFlow, IMMDeviceCollection** devices)
{
    return SUCCEEDED(
        pEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, devices)
    );
}

bool Wasapi::InitRenderService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration)
{
    if (!pOutDevice)
    {
        return false;
    }

    if (FAILED(pOutDevice->Activate(
            IID_IAudioClient,
            CLSCTX_ALL,
            NULL,
            (void **)&pRenderClient
    )))
    {
        return false;
    }
    
    if (FAILED(pRenderClient->GetMixFormat(&owfx)))
    {
        return false;
    }

    if (FAILED(pRenderClient->Initialize(
            shareMode,
            0,
            bufferDuration,
            0,
            owfx,
            NULL
    )))
    {
        return false;
    }

    if (FAILED(pRenderClient->GetService(IID_IAudioRenderClient, (void **)&renderService)))
    {
        return false;
    }
    

    return true;
}

bool Wasapi::InitCaptureService(AUDCLNT_SHAREMODE shareMode, REFERENCE_TIME bufferDuration)
{
    if (!pInDevice)
    {
        return false;
    }

    if (FAILED(pInDevice->Activate(
            IID_IAudioClient,
            CLSCTX_ALL,
            NULL,
            (void **)&pCaptureClient
    )))
    {
        return false;
    }
    
    if (FAILED(pCaptureClient->GetMixFormat(&iwfx)))
    {
        return false;
    }

    if (FAILED(pCaptureClient->Initialize(
            shareMode,
            0,
            bufferDuration,
            0,
            iwfx,
            NULL
    )))
    {
        return false;
    }

    if (FAILED(pCaptureClient->GetService(IID_IAudioCaptureClient, (void **)&captureService)))
    {
        return false;
    }

    return true;
}

bool Wasapi::startRender()
{
    return SUCCEEDED(pRenderClient->Start());
}

bool Wasapi::startCapture()
{
    return SUCCEEDED(pCaptureClient->Start());
}

bool Wasapi::stopRender()
{
    return SUCCEEDED(pRenderClient->Stop());
}

bool Wasapi::stopCapture()
{
   return SUCCEEDED(pCaptureClient->Stop());
}

