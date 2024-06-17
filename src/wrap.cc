#include "core.cc"
#include "napi.h"
#include "type-wrapper.cc"
#include "functiondiscoverykeys.h"
#include "atlstr.h"
#include <iostream>

Wasapi *wasapi = new Wasapi();

Napi::Value N_Init(const Napi::CallbackInfo &info)
{
    return Napi::Boolean::New(info.Env(), wasapi->InitCOM());
}

Napi::Value N_InitDevice(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    const size_t len = info.Length();

    std::optional<int32_t> accessibility;
    std::optional<std::string> output;
    std::optional<std::string> input;

    if (len > 0)
        accessibility = GetNumber<int32_t>(info[0]);
    if (len > 1)
        output = GetString(info[1]);
    if (len > 2)
        input = GetString(info[2]);

    return Napi::Boolean::New(env, wasapi->InitDevice(output, input, accessibility));
}

Napi::Value N_GetDevices(const Napi::CallbackInfo &info)
{
    std::optional<int32_t> type;
    EDataFlow flow;

    if (info.Length())
        type = GetNumber<int32_t>(info[0]);

    const Napi::Value JS_NULL = info.Env().Null();

    if (!type.has_value())
    {
        flow = eAll;
    }
    else
    {
        auto typeInt = type.value();

        if (typeInt == 1)
            flow = eRender;
        else if (typeInt == 2)
            flow = eCapture;
        else if (typeInt == 3)
            flow = eAll;
        else
            return info.Env().Null();
    }

    IMMDeviceCollection *devices = NULL;
    if (!wasapi->GetDevices(flow, &devices))
    {
        return JS_NULL;
    }

    UINT count = 0;
    if (FAILED(devices->GetCount(&count)))
    {
        return JS_NULL;
    }

    if (count == 0)
    {
        return JS_NULL;
    }

    Napi::Array result = Napi::Array::New(info.Env());
    IMMDevice *device = NULL;
    LPWSTR deviceId = NULL;
    IPropertyStore *props = NULL;
    PROPVARIANT name;
    PROPVARIANT desc;
    for (UINT i = 0; i < count; i++)
    {
        devices->Item(i, &device);
        device->GetId(&deviceId);
        device->OpenPropertyStore(STGM_READ, &props);
        props->GetValue(PKEY_DeviceInterface_FriendlyName, &name);
        props->GetValue(PKEY_Device_DeviceDesc, &desc);

        Napi::Object obj = Napi::Object::New(info.Env());
        obj["deviceId"] = CW2A(deviceId);
        obj["name"] = CW2A(name.pwszVal);
        obj["description"] = CW2A(desc.pwszVal);
        result.Set(i, obj);

        CoTaskMemFree(deviceId);
        deviceId = NULL;
        PropVariantClear(&name);
        PropVariantClear(&desc);
        props->Release();
        props = NULL;
        device->Release();
    }

    return result;
}

Napi::Value N_InitRenderService(const Napi::CallbackInfo &info)
{
    auto shareMode = GetNumber<int32_t>(info[0]);
    auto bufferDuration = GetNumber<int64_t>(info[1]);

    return Napi::Boolean::New(
        info.Env(),
        wasapi->InitRenderService(
            static_cast<AUDCLNT_SHAREMODE>(shareMode),
            static_cast<REFERENCE_TIME>(bufferDuration)));
}

Napi::Value N_InitCaptureService(const Napi::CallbackInfo &info)
{
    auto shareMode = GetNumber<int32_t>(info[0]);
    auto bufferDuration = GetNumber<int64_t>(info[1]);

    return Napi::Boolean::New(
        info.Env(),
        wasapi->InitCaptureService(
            static_cast<AUDCLNT_SHAREMODE>(shareMode),
            static_cast<REFERENCE_TIME>(bufferDuration)));
}

Napi::Value N_GetFormat(const Napi::CallbackInfo &info)
{
    int32_t type;
    WAVEFORMATEX *wfx;

    const auto JS_NULL = info.Env().Null();

    if (!info.Length())
        return JS_NULL;

    type = GetNumber<int32_t>(info[0]);

    auto done = wasapi->GetFormat(type, &wfx);
    if (!done)
        return JS_NULL;
    
    const auto env = info.Env();
    const auto obj = Napi::Object::New(env);
    const auto dword = unsafe_number_convert_gen<int64_t, DWORD>(env);
    const auto word = unsafe_number_convert_gen<int64_t, WORD>(env);

    obj.Set("cbSize", word(wfx->cbSize));
    obj.Set("nAvgBytesPerSec", dword(wfx->nAvgBytesPerSec));
    obj.Set("nBlockAlign", word(wfx->nBlockAlign));
    obj.Set("nChannels", word(wfx->nChannels));
    obj.Set("nSamplesPerSec", dword(wfx->nSamplesPerSec));
    obj.Set("wFormatTag", word(wfx->wFormatTag));
    obj.Set("wBitsPerSample", word(wfx->wBitsPerSample));

    return obj;
}

Napi::Value N_GetFormatEx(const Napi::CallbackInfo &info)
{
    int32_t type;
    WAVEFORMATEXTENSIBLE *wfx;

    const auto JS_NULL = info.Env().Null();

    if (!info.Length())
        return JS_NULL;

    type = GetNumber<int32_t>(info[0]);

    auto done = wasapi->GetFormatEx(type, &wfx);
    if (!done)
        return JS_NULL;
    
    const auto env = info.Env();
    const auto result = Napi::Object::New(env);
    const auto samples = Napi::Object::New(env);
    const auto dword = unsafe_number_convert_gen<int64_t, DWORD>(env);
    const auto word = unsafe_number_convert_gen<int64_t, WORD>(env);
    const auto format = N_GetFormat(info);

    result.Set("format", format);
    result.Set("dwChannelMask", dword(wfx->dwChannelMask));

    auto guid = wfx->SubFormat;
    auto sGuid = sizeof(guid);
    auto subFormatGetter = Napi::Object::New(env);

    auto N_SubFormatArrayBuffer = Napi::Function::New(env, [&guid](const Napi::CallbackInfo& info) {
        auto env = info.Env();
        auto subFormatValue = Napi::ArrayBuffer::New(env, sizeof(guid));
        memcpy(subFormatValue.Data(), &guid, sizeof(guid));
        return subFormatValue;
    });

    auto N_SubFormatString = Napi::Function::New(env, [&guid](const Napi::CallbackInfo& info) -> Napi::String {
        auto env = info.Env();
        wchar_t guidStr[39];
        StringFromGUID2(guid, guidStr, 39);
        return Napi::String::New(env, wchar_to_string(guidStr));
    });

    subFormatGetter.Set("arrayBuffer", N_SubFormatArrayBuffer);
    subFormatGetter.Set("string", N_SubFormatString);

    result.Set("subFormat", subFormatGetter);

    samples.Set("wValidBitsPerSample", word(wfx->Samples.wValidBitsPerSample));
    samples.Set("wSamplesPerBlock", word(wfx->Samples.wSamplesPerBlock));
    samples.Set("wReserved", word(wfx->Samples.wReserved));
    result.Set("samples", samples);

    return result;
}

Napi::Value N_Start(const Napi::CallbackInfo &info)
{
    auto type = GetNumber<int32_t>(info[0]);
    return Napi::Boolean::New(
        info.Env(),
        wasapi->Start(type));
}

Napi::Value N_Stop(const Napi::CallbackInfo &info)
{
    auto type = GetNumber<int32_t>(info[0]);
    return Napi::Boolean::New(
        info.Env(),
        wasapi->Stop(type));
}

Napi::Value N_Service(const Napi::CallbackInfo &info)
{
    auto framesRender = static_cast<UINT32>(GetNumber<int32_t>(info[0]));

    wasapi->Service(framesRender);
    return info.Env().Undefined();
}


Napi::FunctionReference needDataHandlerRef;
Napi::Value N_SetNeedDataHandler(const Napi::CallbackInfo &info)
{
    auto cb = info[0].As<Napi::Function>();
    needDataHandlerRef = Napi::Persistent(cb);
    auto env = info.Env();
    auto uint32 = unsafe_number_convert_gen<int64_t, UINT32>(env);
    
    auto value = wasapi->SetNeedDataHandler([=](BYTE **data, const UINT32 freeSize) {
        auto& cbRef = needDataHandlerRef;
        auto cbEnv = cbRef.Env();
        auto writeFunc = Napi::Function::New(cbEnv, [=](const Napi::CallbackInfo &info) {
            auto buffer = info[0].As<Napi::ArrayBuffer>();
            memcpy(*data, buffer.Data(), buffer.ByteLength());
            return info.Env().Undefined();
        });
        auto arg0 = uint32(freeSize);
        auto returnVal = cbRef.Call({ arg0, writeFunc });
        return static_cast<UINT32>(returnVal.As<Napi::Number>().Int64Value());
    });

    return Napi::Boolean::New(env, value);
}

Napi::FunctionReference recvDataHandlerRef;
Napi::Value N_SetRecvDataHandler(const Napi::CallbackInfo &info)
{
    auto cb = info[0].As<Napi::Function>();
    recvDataHandlerRef = Napi::Persistent(cb);
    auto env = info.Env();
    auto uint32 = unsafe_number_convert_gen<int64_t, UINT32>(env);
    auto dword = unsafe_number_convert_gen<int32_t, DWORD>(env);
    
    auto value = wasapi->SetRecvDataHandler([=](
        BYTE **ppData,
        UINT32 pNumFramesToRead,
        DWORD pdwFlags,
        UINT64 pu64DevicePosition,
        UINT64 pu64QPCPosition) {
            auto& cbRef = recvDataHandlerRef;
            auto cbEnv = cbRef.Env();
            auto len = static_cast<size_t>(pNumFramesToRead);
            auto buffer = Napi::ArrayBuffer::New(cbEnv, len);
            memcpy(buffer.Data(), &ppData, len);

            cbRef.Call({
                buffer,
                uint32(pNumFramesToRead),
                dword(pdwFlags),
                Napi::BigInt::New(cbEnv, static_cast<uint64_t>(pu64DevicePosition)),
                Napi::BigInt::New(cbEnv, static_cast<uint64_t>(pu64QPCPosition)),
            });
    });

    return Napi::Boolean::New(env, value);
}

Napi::Value N_Release(const Napi::CallbackInfo &info)
{
    if (needDataHandlerRef)
    {
        needDataHandlerRef.Reset();
    }

    if (recvDataHandlerRef)
    {
        recvDataHandlerRef.Reset();
    }
    
    wasapi->Release();
    return info.Env().Undefined();
}

Napi::Value N_StopService(const Napi::CallbackInfo &info)
{
    wasapi->StopService();
    return info.Env().Undefined();
}