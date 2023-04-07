#pragma once

#include <utils.h>

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
      ([&] { ss << std::setw(utils::intWidth) << args; }(), ...);
    }(term, key, val, clientId, reqNo);
    return ss.str();
  }

  LogEntry fromString(const std::string &line) {
    assertm(line.size() == utils::intWidth * utils::memberVariableLog,
            "Unexpected line size");

    [&](auto &...args) {
      int i = 0;
      (
          [&] {
            args =
                std::stoi(line.substr(i++ * utils::intWidth, utils::intWidth));
          }(),
          ...);
    }(term, key, val, clientId, reqNo);

    return *this;
  }

  void fillDummyEntry() {
    term = 0;
    clientId = 0;
    reqNo = 0;
    key = std::pow(10, utils::intWidth) - 1;
    val = std::pow(10, utils::intWidth) - 1;
  }

  bool isDummy() const {
    uint infinity = std::pow(10, utils::intWidth) - 1;
    return (key == infinity && val == infinity && clientId == 0 && reqNo == 0);
  }

  void fillMemberChangeEntry(uint newMachineId, uint oldMachineCount) {
    term = 0;
    clientId = newMachineId;
    reqNo = oldMachineCount;
    key = std::pow(10, utils::intWidth) - 1;
    val = std::pow(10, utils::intWidth) - 1;
  }

  bool isMemberChange() const {
    uint infinity = std::pow(10, utils::intWidth) - 1;
    return (key == infinity && val == infinity && clientId != 0 && reqNo != 0);
  }

  void fillMemberChangeCommitEntry(const LogEntry &memberChangeEntry) {
    assertm(memberChangeEntry.isMemberChange(), " memeber change bhej");
    term = memberChangeEntry.term;
    clientId = memberChangeEntry.clientId;
    reqNo = memberChangeEntry.reqNo;
    key = std::pow(10, utils::intWidth) - 2;
    val = std::pow(10, utils::intWidth) - 2;
  }

  bool isMemberChangeCommit() const {
    uint infinity2 = std::pow(10, utils::intWidth) - 2;
    return (key == infinity2 && val == infinity2 && clientId != 0 && reqNo != 0);
  }
};

class MachineCountPersistence {
public:
  static MachineCountPersistence &
  getInstance(const std::filesystem::path &path) {
    static MachineCountPersistence instance{path};
    return instance;
  }
  uint getMachineCount();
  bool incrementMachineCount(uint); // leader should use this
  void setMachineCount(uint); // should only be used by the new machine

private:
  MachineCountPersistence(const std::filesystem::path &);
  MachineCountPersistence(const MachineCountPersistence &) = delete;
  MachineCountPersistence &operator=(const MachineCountPersistence &) = delete;
  ~MachineCountPersistence() = default;

  uint machineCountCache{0};
  std::shared_mutex machineCountLock;
  std::filesystem::path machineCountFsPath;
};

class LogPersistence {
public:
  LogPersistence(const std::filesystem::path &,
                 uint); // accepts home directory path

  void appendLog(const LogEntry &);
  int getLastLogIndex();
  std::optional<LogEntry> readLog(uint); // log index
  std::vector<LogEntry> readLogRange(uint, uint);
  std::pair<bool, std::optional<LogEntry>>
  checkAndWriteLog(uint, const LogEntry &, int, uint);
  bool checkEmptyHeartbeat(uint, int, uint);
  int readLastCommitIndex();
  int readLastSyncIndex();
  void markLogSyncBit(uint, uint, bool); // index, machineId;
  void reset();
  void markSelfSyncBit();
  void incrementLastSyncIndex(int &);
  std::pair<int, int> getLastLogData();
  bool isReadable(uint);
  ~LogPersistence() = default;

private:
  std::optional<LogEntry> writeLog(uint, const LogEntry &, int);
  inline void seekToLogIndex(uint);
  void updateLastCommitIndex(int);

  std::fstream logFs;
  std::fstream lastCommitIndexFs;
  std::fstream levelDbFs;
  std::recursive_mutex logLock;
  std::recursive_mutex syncLock; // should be taken before last commit lock
  std::shared_mutex lastCommitIndexLock;
  std::recursive_mutex levelDbSyncLock;
  std::unordered_map<uint, std::bitset<utils::maxMachineCount>>
      logSync;                  // index, majorityBits
  int lastCommitIndexCache{-1}; // just cache
  int levelDbSyncCache{-1};
  uint selfId;
  MachineCountPersistence &machineCountPersistence;
};

class ElectionPersistence {
public:
  ElectionPersistence(const std::filesystem::path &,
                      uint); // accepts home directory path and selfId
  uint getTerm();
  bool setTermAndSetVote(uint, uint machineId);
  bool incrementTermAndSelfVote(uint);
  std::pair<bool, bool> incrementMachineCountTermAndSelfVote(uint, uint);
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
  MachineCountPersistence &machineCountPersistence;
};