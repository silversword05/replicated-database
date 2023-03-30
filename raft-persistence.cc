#include <raft-persistence.h>

LogPersistence::LogPersistence(const std::filesystem::path &homeDir,
                               uint selfId_)
    : selfId(selfId_) {
  logFs = std::fstream(homeDir / "log.txt", std::ios::app);
  logFs = std::fstream(homeDir / "log.txt",
                       std::ios::in | std::ios::out | std::ios::ate);
  lastCommitIndexFs = std::fstream(homeDir / "commit-index.txt", std::ios::app);
  lastCommitIndexFs =
      std::fstream(homeDir / "commit-index.txt",
                   std::ios::in | std::ios::out | std::ios::ate);
  readLastCommitIndex();
}

void LogPersistence::appendLog(const LogEntry &logEntry) {
  std::lock_guard _(logLock);
  logFs.clear();
  logFs.seekg(0, std::ios::end);
  logFs << logEntry.getString() << std::endl;
  markLogSyncBit(getLastLogIndex() , selfId);
}

inline void LogPersistence::seekToLogIndex(uint logIndex) {
  std::lock_guard _(logLock);
  logFs.clear(); // to make sure that reset eof bit if previously set
  logFs.seekg(logIndex * (memberVariableLog * intWidth + 1), std::ios::beg);
}

inline int LogPersistence::getLastLogIndex() {
  std::lock_guard _(logLock);
  logFs.seekg(0, std::ios::end);
  return (logFs.tellg() / (memberVariableLog * intWidth + 1)) - 1 ;
}

std::optional<LogEntry> LogPersistence::readLog(uint logIndex) {
  std::lock_guard _(logLock);
  seekToLogIndex(logIndex);
  std::string line;
  if (std::getline(logFs, line))
    return LogEntry().fromString(line);
  assertm(bool(logFs), "Bhai read fail ho gaya!!");
  return {};
}

std::vector<LogEntry> LogPersistence::readLogRange(uint start, uint end) {
  std::lock_guard _(logLock);
  seekToLogIndex(start);
  std::vector<LogEntry> logEntries;
  std::string line;
  while (start++ <= end && std::getline(logFs, line))
    logEntries.push_back(LogEntry().fromString(line));
  return logEntries;
}

std::optional<LogEntry> LogPersistence::writeLog(uint logIndex, const LogEntry &logEntry,
                              int lastCommitIndex) {
  // TODO: Make a wrapper that ensures you don't call this when leader
  std::lock_guard _(logLock);
  auto oldLogEntry = readLog(logIndex);
  if(oldLogEntry)
    assertm(oldLogEntry.value().term <= logEntry.term, "Bhai old term higher than new log entry");
  seekToLogIndex(logIndex);
  logFs << logEntry.getString() << std::endl;
  incrementLastCommitIndex(lastCommitIndex);
  return oldLogEntry;
}

int LogPersistence::readLastCommitIndex() {
  {
    std::shared_lock _(lastCommitIndexLock);
    if (lastCommitIndexCache != -1)
      return lastCommitIndexCache;
  }
  {
    std::unique_lock _(lastCommitIndexLock);
    lastCommitIndexFs.clear();
    lastCommitIndexFs.seekg(0, std::ios::beg);
    if (lastCommitIndexFs >> lastCommitIndexCache)
      return lastCommitIndexCache;
    assertm(bool(lastCommitIndexFs), "Bhai read fail ho gaya!!");
    lastCommitIndexCache = -1;
    return lastCommitIndexCache;
  }
}

void LogPersistence::markLogSyncBit(uint logIndex, uint machineId) {
  assertm(machineId < machineCount, "Ye kon sa naya machine uga diya");
  std::lock_guard _(syncLock);
  auto& majorityVote = logSync[logIndex];
  assertm(majorityVote.test(machineId), "Boss kyu bhej rahe ho dubara");
  majorityVote.set(machineId);
  if(majorityVote.count() == (machineCount / 2 + 1))
    incrementLastCommitIndex(logIndex);
}

void LogPersistence::incrementLastCommitIndex(uint lastCommitIndex) {
  std::unique_lock _(lastCommitIndexLock);
  if(lastCommitIndex == lastCommitIndexCache) return;
  assertm(lastCommitIndex == lastCommitIndexCache + 1,
          "Bhai commit index me garbar he");
  lastCommitIndexFs.clear();
  lastCommitIndexFs.seekg(0, std::ios::beg);
  lastCommitIndexFs << lastCommitIndex;
  lastCommitIndexCache = lastCommitIndex;
}

ElectionPersistence::ElectionPersistence(const std::filesystem::path &homeDir,
                                         uint selfId_)
    : selfId(selfId_) {
  termFsPath = homeDir / "term.txt";
  std::fstream(termFsPath, std::ios::app);
  votedForFsPath = homeDir / "voted-for.txt";
  std::fstream(votedForFsPath, std::ios::app);
}

uint ElectionPersistence::getTerm() {
  {
    std::shared_lock _(termLock);
    if (termCache != std::numeric_limits<uint>::max())
      return termCache;
  }
  {
    std::unique_lock _(termLock);
    {
      std::ifstream ifs(termFsPath);
      assertm(bool(ifs), "Bhai file khula nahi, term wala");
      if (ifs >> termCache)
        return termCache;
      assertm(bool(ifs), "Bhai read fail ho gaya!!");
    }
    {
      std::ofstream ofs(termFsPath);
      termCache = termStart;
      ofs << termCache;
      return termCache;
    }
  }
}

std::optional<uint> ElectionPersistence::getVotedFor() {
  std::shared_lock _(votedForLock);
  std::ifstream ifs(votedForFsPath);
  assertm(bool(ifs), "Bhai file khula nahi, voted for wala");
  uint votedFor;
  if (ifs >> votedFor)
    return votedFor;
  assertm(bool(ifs), "Bhai read fail ho gaya!!");
  return {};
}

uint ElectionPersistence::writeVotedFor(uint machineId) {
  assertm(machineId < machineCount, "Ye kon sa naya machine uga diya");
  std::unique_lock _(votedForLock);
  std::fstream fs(votedForFsPath, std::ios::in | std::ios::out | std::ios::ate);
  assertm(bool(fs), "Bhai file khula nahi, voted for wala");
  if (fs.tellg() == 0) { // file size
    fs << machineId;
    return machineId;
  }
  int machineIdVoted;
  fs >> machineIdVoted;
  return machineIdVoted;
}
