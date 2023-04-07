#pragma once

#include <bits/stdc++.h>
#include <cassert>
#include <chrono>

#define assertm(exp, msg) assert(((void)msg, exp))
#define PRINT(...)                                                             \
  utils::print("[INFO] :", std::this_thread::get_id(), ": ",                   \
               __PRETTY_FUNCTION__, " ", __VA_ARGS__)

namespace utils {
constexpr auto server_address = "localhost:50051";

inline std::filesystem::path home_dir(getenv("HOME"));

constexpr uint intWidth = 8;
constexpr uint memberVariableLog = 5;
constexpr uint machineCount = 3;
constexpr uint termStart = 1;
constexpr uint baseSleepTime = 50; //millisecond
constexpr uint maxSleepTime = 100; //millisecond
constexpr uint followerSleep = 1; //millisecond
constexpr bool forceLocalHost = true;
constexpr bool cleanStart = true;

enum State { FOLLOWER, CANDIDATE, LEADER };

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

inline long long int getCurrTimeinMicro() {
  return duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

inline void print() { std::cout << std::endl; }
template <typename T> inline void print(const T &t) {
  std::cout << t << std::endl;
}
template <typename First, typename... Rest>
inline void print(const First &first, const Rest &...rest) {
  if constexpr (std::is_same_v<First, bool>) {
    std::cout << std::boolalpha << first << " ";
  } else {
    std::cout << first << " ";
  }

  print(rest...); // recursive call using pack expansion syntax
}
} // namespace utils