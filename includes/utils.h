#pragma once

#include <bits/stdc++.h>
#include <cassert>
#include <chrono>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace utils {
constexpr auto server_address = "localhost:50051";

constexpr uint intWidth = 4;
constexpr uint memberVariableLog = 5;
constexpr uint machineCount = 5;
constexpr uint termStart = 1;
} // namespace utils