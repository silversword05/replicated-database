#include <raft-persistence.h>

LogPersistence::LogPersistence(const std::filesystem::path &homeDir,
                               uint selfId_)
    : selfId(selfId_),
      machineCountPersistence(MachineCountPersistence::getInstance(homeDir)) {
  logFs = std::fstream(homeDir / "log.txt", std::ios::app);
  logFs = std::fstream(homeDir / "log.txt",
                       std::ios::in | std::ios::out | std::ios::ate);
  lastCommitIndexFs = std::fstream(homeDir / "commit-index.txt", std::ios::app);
  lastCommitIndexFs =
      std::fstream(homeDir / "commit-index.txt",
                   std::ios::in | std::ios::out | std::ios::ate);
  levelDbFs = std::fstream(homeDir / "leveldb-sync.txt", std::ios::app);
  levelDbFs = std::fstream(homeDir / "leveldb-sync.txt",
                           std::ios::in | std::ios::out | std::ios::ate);
  readLastCommitIndex();
  readLastSyncIndex();
}

void LogPersistence::reset() {
  std::lock_guard _(syncLock);
  logSync.clear();
}

void LogPersistence::markSelfSyncBit() {
  std::lock_guard _1(syncLock);
  readLastCommitIndex();
  std::unique_lock _2(lastCommitIndexLock);
  int lastLogIndex = getLastLogIndex();

  for (int i = lastCommitIndexCache; i <= lastLogIndex; i++) {
    if (i == -1)
      continue;
    markLogSyncBit(i, selfId, true);
  }

  // TODO: Discuss this case properly
  if (lastLogIndex == -1)
    return;
  auto lastLogEntry = readLog(lastLogIndex);
  assertm(lastLogEntry.has_value(), "ye log hona chaiye");
  if (lastLogEntry.value().isMemberChange()) {
    LogEntry newLogEntry;
    newLogEntry.fillMemberChangeCommitEntry(lastLogEntry.value());
    appendLog(newLogEntry);
  }
}

void LogPersistence::appendLog(const LogEntry &logEntry) {
  std::lock_guard _(logLock);
  logFs.clear();
  logFs.seekg(0, std::ios::end);
  logFs << logEntry.getString() << std::endl;
  logFs.flush();
  markLogSyncBit(getLastLogIndex(), selfId, true);
}

inline void LogPersistence::seekToLogIndex(uint logIndex) {
  std::lock_guard _(logLock);
  logFs.clear(); // to make sure that reset eof bit if previously set
  logFs.seekg(logIndex * (utils::memberVariableLog * utils::intWidth + 1),
              std::ios::beg);
}

int LogPersistence::getLastLogIndex() {
  std::lock_guard _(logLock);
  logFs.seekg(0, std::ios::end);
  return (logFs.tellg() / (utils::memberVariableLog * utils::intWidth + 1)) - 1;
}

std::pair<int, int> LogPersistence::getLastLogData() {
  std::lock_guard _(logLock);

  int lastLogIndex = getLastLogIndex();
  if (lastLogIndex == -1)
    return {-1, -1};

  auto logEntry = readLog(uint(lastLogIndex));
  assertm(logEntry.has_value(), "ye value hona chaiye");
  return {lastLogIndex, logEntry.value().term};
}

std::optional<LogEntry> LogPersistence::readLog(uint logIndex) {
  std::lock_guard _(logLock);
  seekToLogIndex(logIndex);
  std::string line;
  if (std::getline(logFs, line))
    return LogEntry().fromString(line);
  // assertm(bool(logFs), "Bhai read fail ho gaya!!");
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

std::optional<LogEntry> LogPersistence::writeLog(uint logIndex,
                                                 const LogEntry &logEntry,
                                                 int lastCommitIndex) {
  // TODO: Make a wrapper that ensures you don't call this when leader
  std::lock_guard _(logLock);
  auto oldLogEntry = readLog(logIndex);
  seekToLogIndex(logIndex);
  logFs << logEntry.getString() << std::endl;
  logFs.flush();
  updateLastCommitIndex(lastCommitIndex);
  return oldLogEntry;
}

std::pair<bool, std::optional<LogEntry>>
LogPersistence::checkAndWriteLog(uint logIndex, const LogEntry &logEntry,
                                 int leaderLastCommitIndex, uint prevTerm) {
  int probableCommitIndex = std::min(leaderLastCommitIndex, (int)logIndex);
  std::lock_guard _(logLock);
  if (int(logIndex) <= readLastCommitIndex()) {
    assertm(readLog(logIndex).value().term == logEntry.term,
            "log entry dont match when leaderLastCommitIndex small");
    return {true, {}};
  }
  if (logIndex == 0)
    return {true, writeLog(logIndex, logEntry, probableCommitIndex)};

  auto lastToOneEntry = readLog(logIndex - 1);
  if (!lastToOneEntry.has_value())
    return {false, {}};
  if (lastToOneEntry.value().term != prevTerm)
    return {false, {}};
  return {true, writeLog(logIndex, logEntry, probableCommitIndex)};
}

bool LogPersistence::checkEmptyHeartbeat(uint logIndex,
                                         int leaderLastCommitIndex,
                                         uint prevTerm) {
  assertm(logIndex > 0, "Ek dummy to mangta he");
  std::lock_guard _(logLock);
  assertm(int(logIndex) > readLastCommitIndex(), "Hona nahi chaiye ye unhoni");
  auto lastToOneEntry = readLog(logIndex - 1);
  if (!lastToOneEntry.has_value())
    return false;
  if (lastToOneEntry.value().term != prevTerm)
    return false;
  updateLastCommitIndex(leaderLastCommitIndex);
  return true;
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
    // assertm(bool(lastCommitIndexFs), "Bhai read fail ho gaya!!");
    lastCommitIndexCache = -1;
    return lastCommitIndexCache;
  }
}

int LogPersistence::readLastSyncIndex() {
  std::lock_guard _(levelDbSyncLock);
  if (levelDbSyncCache != -1)
    return levelDbSyncCache;
  levelDbFs.clear();
  levelDbFs.seekg(0, std::ios::beg);
  if (levelDbFs >> levelDbSyncCache)
    return levelDbSyncCache;
  levelDbSyncCache = -1;
  return levelDbSyncCache;
}

void LogPersistence::markLogSyncBit(uint logIndex, uint machineId,
                                    bool updateLastCommit) {
  assertm(machineId < machineCountPersistence.getMachineCount(),
          "Ye kon sa naya machine uga diya");
  std::lock_guard _(syncLock);
  auto &majorityVote = logSync[logIndex];
  assertm(!majorityVote.test(machineId), "Boss kyu bhej rahe ho dubara");
  majorityVote.set(machineId);
  if (majorityVote.count() ==
          (machineCountPersistence.getMachineCount() / 2 + 1) &&
      updateLastCommit)
    updateLastCommitIndex(logIndex);
}

void LogPersistence::updateLastCommitIndex(int lastCommitIndex) {
  std::unique_lock _(lastCommitIndexLock);
  if (lastCommitIndex == lastCommitIndexCache)
    return;
  assertm(lastCommitIndex > lastCommitIndexCache,
          "Bhai commit index me garbar he");
  lastCommitIndexFs.clear();
  lastCommitIndexFs.seekg(0, std::ios::beg);
  lastCommitIndexFs << lastCommitIndex;
  lastCommitIndexCache = lastCommitIndex;

  lastCommitIndexFs.flush();
}

void LogPersistence::incrementLastSyncIndex(int &levelDbSyncIndex) {
  std::lock_guard _(levelDbSyncLock);
  assertm(levelDbSyncIndex == levelDbSyncCache,
          "Bhai bich ka kyu miss kar dia");
  levelDbSyncIndex++;
  levelDbFs.clear();
  levelDbFs.seekg(0, std::ios::beg);
  levelDbFs << levelDbSyncIndex;
  levelDbSyncCache = levelDbSyncIndex;

  levelDbFs.flush();
}

bool LogPersistence::isReadable(uint expectedTerm) {
  if (expectedTerm == utils::termStart)
    return false;
  std::lock_guard _(logLock);
  int lastLogIndex = getLastLogIndex();
  if (lastLogIndex == -1)
    return true;
  auto lastEntry = readLog(lastLogIndex);
  assertm(lastEntry.has_value(), "This should exists");
  return (lastEntry.value().term == expectedTerm);
}

ElectionPersistence::ElectionPersistence(const std::filesystem::path &homeDir,
                                         uint selfId_)
    : selfId(selfId_),
      machineCountPersistence(MachineCountPersistence::getInstance(homeDir)) {
  termFsPath = homeDir / "term.txt";
  std::fstream(termFsPath, std::ios::app);
  votedForFsPath = homeDir / "voted-for.txt";
  std::fstream(votedForFsPath, std::ios::app);
  getTerm();
}

uint ElectionPersistence::getTerm() {
  {
    std::shared_lock _(termLock);
    if (termCache != 0)
      return termCache;
  }
  {
    std::unique_lock _(termLock);
    {
      std::ifstream ifs(termFsPath);
      assertm(bool(ifs), "Bhai file khula nahi, term wala");
      if (ifs >> termCache)
        return termCache;
      // assertm(bool(ifs), "Bhai read fail ho gaya!!");
    }
    {
      std::ofstream ofs(termFsPath);
      termCache = utils::termStart;
      ofs << termCache;
      ofs.flush();
      return termCache;
    }
  }
}

std::optional<uint> ElectionPersistence::getVotedFor() {
  std::lock_guard _(votedForLock);
  std::ifstream ifs(votedForFsPath);
  assertm(bool(ifs), "Bhai file khula nahi, voted for wala");
  uint votedFor;
  if (ifs >> votedFor)
    return votedFor;
  // assertm(bool(ifs), "Bhai read fail ho gaya!!");
  return {};
}

uint ElectionPersistence::writeVotedFor(uint machineId) {
  assertm(machineId < machineCountPersistence.getMachineCount(),
          "Ye kon sa naya machine uga diya");
  std::lock_guard _(votedForLock);
  std::fstream fs(votedForFsPath, std::ios::in | std::ios::out | std::ios::ate);
  assertm(bool(fs), "Bhai file khula nahi, voted for wala");
  if (fs.tellg() == 0) { // file size
    fs << machineId;
    fs.flush();
    return machineId;
  }
  int machineIdVoted;
  fs.clear();
  fs.seekg(0, std::ios::beg);
  assertm(fs >> machineIdVoted, "Boss read fail ho gaya");
  return machineIdVoted;
}

void ElectionPersistence::writeTermAndVote(uint term, uint machineId) {
  std::fstream fs(termFsPath, std::ios::in | std::ios::out);
  fs.clear();
  fs.seekg(0, std::ios::beg);
  fs << term;
  fs.flush();
  termCache = term;
  // clear vote
  std::ofstream ofs(votedForFsPath);
  if (machineId != std::numeric_limits<uint>::max()) {
    ofs << machineId;
    ofs.flush();
  }
}

bool ElectionPersistence::setTermAndSetVote(
    uint term, uint machineId = std::numeric_limits<uint>::max()) {
  std::scoped_lock _{votedForLock, termLock};
  {
    std::fstream fs(termFsPath, std::ios::in | std::ios::out);
    if (fs >> termCache)
      if (termCache >= term)
        return false;
  }

  writeTermAndVote(term, machineId);
  return true;
}

bool ElectionPersistence::incrementTermAndSelfVote(uint oldTerm) {
  std::scoped_lock _{votedForLock, termLock};

  std::fstream fs(termFsPath, std::ios::in | std::ios::out);
  assertm(fs >> termCache, "File should be readable");
  assertm(termCache >= oldTerm, "bsdk election ho gaya he");
  if (termCache == oldTerm) {
    fs.close();
    writeTermAndVote(oldTerm + 1, selfId);
    return true;
  }
  return false;
}

std::pair<bool, bool>
ElectionPersistence::incrementMachineCountTermAndSelfVote(uint logMachineCount,
                                                          uint logTerm) {

  if (incrementTermAndSelfVote(logTerm))
    return {true,
            machineCountPersistence.incrementMachineCount(logMachineCount)};
  return {false, false};
}

MachineCountPersistence::MachineCountPersistence(
    const std::filesystem::path &homeDir) {
  machineCountFsPath = homeDir / "machine-count.txt";
  std::fstream(machineCountFsPath, std::ios::app);
  getMachineCount();
}

uint MachineCountPersistence::getMachineCount() {
  {
    std::shared_lock _(machineCountLock);
    if (machineCountCache != 0)
      return machineCountCache;
  }
  {
    std::unique_lock _(machineCountLock);
    {
      std::ifstream ifs(machineCountFsPath);
      assertm(bool(ifs), "Bhai file khula nahi, term wala");
      if (ifs >> machineCountCache)
        return machineCountCache;
      // assertm(bool(ifs), "Bhai read fail ho gaya!!");
    }
    {
      std::ofstream ofs(machineCountFsPath);
      machineCountCache = utils::initialMachineCount;
      ofs << machineCountCache;
      ofs.flush();
      return machineCountCache;
    }
  }
}

bool MachineCountPersistence::incrementMachineCount(uint logMachineCount) {
  std::unique_lock _{machineCountLock};

  std::fstream fs(machineCountFsPath, std::ios::in | std::ios::out);
  assertm(fs >> machineCountCache, "File should be readable");
  if (machineCountCache == logMachineCount) {
    fs.clear();
    fs.seekg(0, std::ios::beg);
    fs << (logMachineCount + 1);
    fs.flush();
    machineCountCache = logMachineCount + 1;
    return true;
  }
  return false;
}

void MachineCountPersistence::setMachineCount(uint newMachineCount) {
  std::unique_lock _{machineCountLock};

  std::fstream fs(machineCountFsPath, std::ios::in | std::ios::out);
  fs << newMachineCount;
  fs.flush();
  machineCountCache = newMachineCount;
}