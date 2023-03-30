#include <raft-persistence.h>

LogPersistence::LogPersistence(const std::filesystem::path &homeDir) {
  logFs = std::fstream(homeDir / "log.txt", std::ios::app);
  logFs = std::fstream(homeDir / "log.txt",
                       std::ios::in | std::ios::out | std::ios::ate);
  lastCommitIndexFs = std::fstream(homeDir / "commit-index.txt", std::ios::app);
  lastCommitIndexFs =
      std::fstream(homeDir / "commit-index.txt",
                   std::ios::in | std::ios::out | std::ios::ate);
}

void LogPersistence::appendLog(const LogEntry &logEntry) {
	// TODO: Add tokens 
  std::lock_guard(logLock);
  logFs.clear();
  logFs.seekg(0, std::ios::end);
  logFs << logEntry.getString() << "\n";
}

inline void LogPersistence::seekToLogIndex(uint logIndex) {
  logFs.clear(); // to make sure that reset eof bit if previously set
  logFs.seekg(logIndex * (4 * intWidth + 1), std::ios::beg);
}

std::optional<LogEntry> LogPersistence::readLog(uint logIndex) {
  std::lock_guard(logLock);
  seekToLogIndex(logIndex);
  std::string line;
  if (std::getline(logFs, line))
    return LogEntry().fromString(line);
  assertm(bool(logFs), "Bhai read fail ho gaya!!");
  return {};
}

std::vector<LogEntry> LogPersistence::readLogRange(uint start, uint end) {
  std::lock_guard(logLock);
  seekToLogIndex(start);
  std::vector<LogEntry> logEntries;
  std::string line;
  while (start++ <= end && std::getline(logFs, line))
    logEntries.push_back(LogEntry().fromString(line));
  return logEntries;
}

void LogPersistence::writeLog(uint logIndex, const LogEntry &logEntry) {
	// TODO: Make a wrapper that ensures you don't call this when leader
  std::lock_guard(logLock);
  seekToLogIndex(logIndex);
  logFs << logEntry.getString() << std::endl;
}

std::optional<uint> LogPersistence::readLastCommitIndex() {
  {
    std::shared_lock(lastCommitIndexLock);
    if (lastCommitIndexCache != std::numeric_limits<uint>::max())
      return lastCommitIndexCache;
  }
  {
    std::unique_lock(lastCommitIndexLock);
    if (lastCommitIndexFs >> lastCommitIndexCache)
      return lastCommitIndexCache;
    assertm(bool(lastCommitIndexFs), "Bhai read fail ho gaya!!");
    return {};
  }
}

void LogPersistence::markLogSyncBit(uint logIndex, uint machineId) {
    assertm(machineId < machineCount, "Ye kon sa naya machine uga diya");
    throw "Not implemented";
		// TODO: Make sure you add tokens in append log
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
    std::shared_lock(termLock);
    if (termCache != std::numeric_limits<uint>::max())
      return termCache;
  }
  {
    std::unique_lock(termLock);
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
  std::shared_lock(votedForLock);
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
  std::unique_lock(votedForLock);
  std::fstream fs(votedForFsPath, std::ios::in | std::ios::out | std::ios::ate);
	assertm(bool(fs), "Bhai file khula nahi, voted for wala");
	if(fs.tellg() == 0) { // file size
		fs << machineId;
		return machineId;
	}
	int machineIdVoted;
	fs >> machineIdVoted;
	return machineIdVoted;
}
