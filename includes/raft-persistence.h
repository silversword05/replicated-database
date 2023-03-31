#pragma once

#include <bits/stdc++.h>

#define assertm(exp, msg) assert(((void)msg, exp))

constexpr uint intWidth = 4;
constexpr uint memberVariableLog = 5;
constexpr uint machineCount = 5;
constexpr uint termStart = 1;

struct LogEntry {
  uint term;
  uint key;
  uint val;
  uint clientId;
  uint reqNo;

  std::string getString() const {
    std::stringstream ss;
    ss << std::setfill('0');

    [&](auto... args) {
      ([&] { ss << std::setw(intWidth) << args; }(), ...);
    }(term, key, val, clientId, reqNo);
    return ss.str();
  }

  LogEntry fromString(const std::string &line) {
    assertm(line.size() == intWidth * 4, "Unexpected line size");

    [&](auto &...args) {
      int i = 0;
      ([&] { args = std::stoi(line.substr(i++ * intWidth, intWidth)); }(), ...);
    }(term, key, val, clientId, reqNo);

    return *this;
  }
};

class LogPersistence {
public:
  LogPersistence(const std::filesystem::path &,
                 uint); // accepts home directory path
  void appendLog(const LogEntry &);
  inline int getLastLogIndex();
  std::optional<LogEntry> readLog(uint); // log index
  std::vector<LogEntry> readLogRange(uint, uint);
  std::pair<bool, std::optional<LogEntry>>
  checkAndWriteLog(uint, const LogEntry &, int, uint);
  int readLastCommitIndex();
  void markLogSyncBit(uint, uint); // index, machineId;
  ~LogPersistence() = default;

private:
  std::optional<LogEntry> writeLog(uint, const LogEntry &, int);
  inline void seekToLogIndex(uint);
  void incrementLastCommitIndex(uint);

  std::fstream logFs;
  std::fstream lastCommitIndexFs;
  std::recursive_mutex logLock;
  std::recursive_mutex syncLock; // should be taken before last commit lock
  std::shared_mutex lastCommitIndexLock;
  std::unordered_map<uint, std::bitset<machineCount>>
      logSync;                  // index, majorityBits
  int lastCommitIndexCache{-1}; // just cache
  uint selfId;
};

class ElectionPersistence {
  ElectionPersistence(const std::filesystem::path &,
                      uint); // accepts home directory path and selfId
  uint getTerm();
  void setTermAndSetVote(uint, uint);
  bool incrementTermAndSelfVote(uint);
  std::optional<uint> getVotedFor();
  uint writeVotedFor(uint);
  ~ElectionPersistence() = default;

private:
  void writeTermAndVote(uint, uint);

  std::filesystem::path termFsPath;
  std::filesystem::path votedForFsPath;
  std::shared_mutex termLock;
  std::recursive_mutex votedForLock;
  uint termCache{0}; // just cache
  uint selfId;
};

class Raft final : public ElectionPersistence, public LogPersistence {
  Raft(const std::filesystem::path &, uint);
  void addStopToken(const std::stop_token &);
  ~Raft(); // call stop on stopTokens, clear vote

private:
  std::vector<std::stop_token> stopTokens;
};