#pragma once

#include <cstdint>
#include <cstdio>
#include <string_view>
#include <cassert>

enum class Err : int8_t
{
    OK = 0,
    ERROR = -1,
    TIMEOUT = -2,
    NO_CONNECTION = -3,
    INVALID_RESPONSE = -4,
    NO_DATA = -5,
    ALREADY_CONNECTED = -6,
    NOT_INITIALISED = -7,
    INVALID_ARGUMENT = -8,
    NO_MEMORY = -9,
    UNSUPPORTED = -10
};

constexpr std::string_view resultToString(Err r)
{
    switch (r)
    {
    case Err::OK:
        return "OK";
    case Err::ERROR:
        return "ERROR";
    case Err::TIMEOUT:
        return "TIMEOUT";
    case Err::NO_CONNECTION:
        return "NO_CONNECTION";
    case Err::INVALID_RESPONSE:
        return "INVALID_RESPONSE";
    case Err::NO_DATA:
        return "NO_DATA";
    case Err::ALREADY_CONNECTED:
        return "ALREADY_CONNECTED";
    case Err::NOT_INITIALISED:
        return "NOT_INITIALISED";
    case Err::INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case Err::NO_MEMORY:
        return "NO_MEMORY";
    case Err::UNSUPPORTED:
        return "UNSUPPORTED";
    default:
        return "UNKNOWN";
    }
}

template <typename T >
struct ResultOr
{
    static_assert(!std::is_same_v<T, Err>, "ResultOr<T> cannot have T = Result");

    Err result;
    T value;

    bool ok() const { return result == Err::OK; };
    const T& unwrap() const
    {
        if (!ok())
        {
            printf("Tried to unwrap a ResultOr that was not OK: %s\n", resultToString(result).data());
        }
        return value;
    }

    ResultOr() = delete;

    // this one can be implicit
    ResultOr(Err r) : result(r), value() {
        assert(r != Err::OK);
    }
    explicit ResultOr(T v) : result(Err::OK), value(v) {}
};
