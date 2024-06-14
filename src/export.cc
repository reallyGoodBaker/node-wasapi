#include "napi.h"
#include "wrap.cc"

typedef Napi::Value (*NFunc)(const Napi::CallbackInfo &info);

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    const auto bind = [exports, env] (const char* name, NFunc func) -> void {
        exports.Set(
            Napi::String::New(env, name),
            Napi::Function::New(env, func)
        );
    };

    bind("_init", N_Init);
    bind("_initDevice", N_InitDevice);
    bind("_release", N_Release);
    bind("_getDevices", N_GetDevices);

    return exports;
}

NODE_API_MODULE(wasapi, Init)