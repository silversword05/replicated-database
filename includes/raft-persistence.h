#pragma once

#include <bits/stdc++.h>

#define assertm(exp, msg) assert(((void)msg, exp))

constexpr uint intWidth = 4;

struct LogEntry {
  uint term;
  uint index;
  int key;
  int val;

  std::string getString() const {
    std::stringstream ss;
    ss << std::setfill('0');

    [&](auto... args) {
      ([&] { ss << std::setw(intWidth) << args; }(), ...);
    }(term, index, key, val);
    return ss.str();
  }

  void fromString(const std::string &line) {
    assertm(line.size() == intWidth * 4, "Unexpected line size");

    [&](auto &...args) {
      int i = 0;
      ([&] { args = std::stoi(line.substr(i++ * intWidth, intWidth)); }(), ...);
    }(term, index, key, val);
  }
};

class LogPersistence {
public:
  LogPersistence(const std::filesystem::path &); // accepts home directory path
  void appendLog(const LogEntry &);
  std::optional<LogEntry> readLog(int); // log index
  std::vector<LogEntry> readLogRange(int, int);
  void writeLog(int, const LogEntry &);
  uint readLastCommitIndex();
  void markLogSyncBit(uint, uint); // index, machineId;
  ~LogPersistence();

private:
  std::fstream logFs;
  std::fstream lastCommitIndexFs;
  std::recursive_mutex logLock;
  std::shared_mutex lastCommitIndexLock;
  std::unordered_map<uint, std::pair<uint, uint>> logSyncTokens; // index, <voteBits, token>
  uint lastCommitIndexCache{std::numeric_limits<uint>::max()};              // just cache
};

class ElectionPersistence {
  ElectionPersistence(const std::filesystem::path &,
                      uint); // accepts home directory path and selfId
  uint getTerm();
  virtual void incrementTerm() = 0;
  uint getVotedFor();
  void writeVotedFor(uint);
  virtual void incrementTermAndSelfVote() = 0;
  ~ElectionPersistence();

private:
  std::filesystem::path termFsPath;
  std::filesystem::path votedForFsPath;
  std::shared_mutex termLock;
  std::shared_mutex votedForLock;
  uint termCache{std::numeric_limits<uint>::max()}; // just cache
  uint selfId;
};

class Raft final : public ElectionPersistence, public LogPersistence {
  Raft(const std::filesystem::path &, uint);
  void incrementTerm();
  void incrementTermAndSelfVote();
  void addStopToken(std::stop_token);
private:
  std::vector<std::stop_token> stopTokens;
};