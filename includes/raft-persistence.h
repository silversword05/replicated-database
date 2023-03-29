#include <bits/stdc++.h>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace persist {
constexpr uint intWidth = 4;

struct LogEntry {
  uint term;
  uint index;
  int key;
  int val;

  std::string getString() {
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
  LogEntry readLog(int); // log index
  std::vector<LogEntry> readLogRange(int, int);
  void writeLog(int, const LogEntry &);
  uint readLastCommitIndex();
  void writeLastCommitIndex(uint);
  ~LogPersistence();

private:
  std::fstream logFs;
  std::fstream lastCommitIndexFs;
  std::recursive_mutex logLock;
  std::shared_mutex lastCommitIndexLock;
  std::unordered_map<uint, uint> logVoteCount; // index, voteCount
  uint lastCommitIndexCache{-1};               // just cache
};

class ElectionPersistence {
  ElectionPersistence(const std::filesystem::path &,
                      uint); // accepts home directory path and selfId
  uint getTerm();
  void writeTerm(uint);
  uint getVotedFor();
  void writeVotedFor(uint);
  void incrementTermAndSelfVote();
  ~ElectionPersistence();

private:
  std::filesystem::path termFsPath;
  std::filesystem::path votedForFsPath;
  std::shared_mutex termLock;
  std::shared_mutex votedForLock;
  uint termCache{-1}; // just cache
  uint selfId;
};

} // namespace persist