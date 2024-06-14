#include "init.cc"
#include "napi.h"
#include "getter.cc"
#include "functiondiscoverykeys.h"
#include "atlstr.h"
#include <iostream>

Wasapi *wasapi = new Wasapi();

Napi::Value N_Init(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), wasapi->InitCOM());
}

Napi::Value N_InitDevice(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::optional<std::string> output(GetString(info[0]));
    std::optional<std::string> input(GetString(info[1]));
    std::optional<int32_t> accessibility(GetNumber<int32_t>(info[2]));

    return Napi::Boolean::New(env, wasapi->InitDevice(output, input, accessibility));
}

Napi::Value N_Release(const Napi::CallbackInfo& info) {
    wasapi->Release();
    return info.Env().Undefined();
}

Napi::Value N_GetDevices(const Napi::CallbackInfo& info) {
    std::optional<std::string> type(GetString(info[0]));
    EDataFlow flow;

    const Napi::Value JS_NULL = info.Env().Null();

    if (!type.has_value())
    {
        flow = eAll;
    }
    else
    {
        auto typeStr = type.value();

        if (typeStr == "output")
            flow = eRender;
        else if (typeStr == "input")
            flow = eCapture;
        else if (typeStr == "all")
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