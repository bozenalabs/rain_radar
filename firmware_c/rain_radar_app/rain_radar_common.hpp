#pragma once

#include <cstdint>
#include <cstdio>
#include <string_view>

enum class Result : int8_t {
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

constexpr std::string_view resultToString(Result r) {
    switch(r) {
        case Result::OK: return "OK";
        case Result::ERROR: return "ERROR";
        case Result::TIMEOUT: return "TIMEOUT";
        case Result::NO_CONNECTION: return "NO_CONNECTION";
        case Result::INVALID_RESPONSE: return "INVALID_RESPONSE";
        case Result::NO_DATA: return "NO_DATA";
        case Result::ALREADY_CONNECTED: return "ALREADY_CONNECTED";
        case Result::NOT_INITIALISED: return "NOT_INITIALISED";
        case Result::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case Result::NO_MEMORY: return "NO_MEMORY";
        case Result::UNSUPPORTED: return "UNSUPPORTED";
        default: return "UNKNOWN";
    }
}