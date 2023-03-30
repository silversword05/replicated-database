#include <raft-persistence.h>

LogPersistence::LogPersistence(const std::filesystem::path &homeDir) {
    logFs = std::fstream(homeDir / "log.txt", std::ios::app);
    logFs = std::fstream(homeDir / "log.txt", std::ios::in | std::ios::out | std::ios::ate);
    lastCommitIndexFs = std::fstream(homeDir / "commit-index.txt", std::ios::app);
    lastCommitIndexFs = std::fstream(homeDir / "commit-index.txt", std::ios::in | std::ios::out | std::ios::ate);
}

void LogPersistence::appendLog(const LogEntry &logEntry) {
    std::lock_guard _(logLock);
    logFs.seekg(0, std::ios::end);
    logFs << logEntry.getString() << "\n";
}

std::optional<LogEntry> LogPersistence::readLog(int logIndex) {
    std::lock_guard _(logLock);
    logFs.seekg(logIndex*(4*intWidth+1), std::ios::beg);
}