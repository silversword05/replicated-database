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

enum State { LEADER, FOLLOWER, CANDIDATE };

uint getSelfId() {
  std::string hostname;
  hostname.resize(HOST_NAME_MAX + 1);
  gethostname(hostname.data(), HOST_NAME_MAX + 1);

  assertm(hostname.starts_with("node"), "Hostname galat dikh raha he");
  return std::stoi(hostname.substr(4, hostname.find(".") - 4));
}

std::string getHostName(uint machineId) {
  std::string hostname;
  hostname.resize(HOST_NAME_MAX + 1);
  gethostname(hostname.data(), HOST_NAME_MAX + 1);

  return ("node" + std::to_string(machineId) + hostname.substr(hostname.find(".")));
}
} // namespace utils