#pragma once

#include <bits/stdc++.h>
#include <cassert>
#include <chrono>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace utils {
constexpr auto server_address = "localhost:50051";

constexpr uint intWidth = 4;
constexpr uint memberVariableLog = 5;
constexpr uint machineCount = 3;
constexpr uint termStart = 1;
constexpr uint baseSleepTime = 1000;
constexpr uint maxSleepTime = 2000;
constexpr bool forceLocalHost = true;

enum State { LEADER, FOLLOWER, CANDIDATE };

inline uint getSelfId() {
  std::string hostname;
  hostname.resize(HOST_NAME_MAX + 1);
  gethostname(hostname.data(), HOST_NAME_MAX + 1);

  assertm(hostname.starts_with("node"), "Hostname galat dikh raha he");
  return std::stoi(hostname.substr(4, hostname.find(".") - 4));
}

inline std::string getHostName(uint machineId) {
  std::string hostname;
  hostname.resize(HOST_NAME_MAX + 1);
  gethostname(hostname.data(), HOST_NAME_MAX + 1);

  return ("node" + std::to_string(machineId));
}

inline std::string getAddress(uint machineId) {
  if constexpr (forceLocalHost)
    return "localhost:5005" + std::to_string(machineId);
  return getHostName(machineId) + ":5005" + std::to_string(machineId);
}
} // namespace utils