#include "napi.h"
#include <iostream>

std::string GetString(Napi::Value v)
{
    return v.As<Napi::String>().Utf8Value();
}

template <typename T>
T GetNumber(Napi::Value v)
{
    Napi::Number num = v.As<Napi::Number>();

    if (typeid(T) == typeid(int32_t))
    {
        return num.Int32Value();
    }

    if (typeid(T) == typeid(int64_t))
    {
        return num.Int64Value();
    }

    if (typeid(T) == typeid(float))
    {
        return num.FloatValue();
    }

    if (typeid(T) == typeid(double_t))
    {
        return num.DoubleValue();
    }

    return T();
}

template <typename To, typename Frome>
inline std::function<Napi::Number(Frome)> unsafe_number_convert_gen(const Napi::Env &env)
{
    return [env](Frome v) -> Napi::Number
    {
        return Napi::Number::New(env, static_cast<To>(v));
    };
}