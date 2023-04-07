#pragma once

#include <concurrentqueue.h>
#include <raft-client.h>
#include <raft-persistence.h>
#include <raft-leveldb.h>

class RaftControl {
public:
  RaftControl(const std::filesystem::path &, uint, RaftClient &);
  void addJthread(std::jthread &&);
  void callStopOnAllThreads();

  bool followerToCandidate(uint &oldTerm);
  bool candidateToLeader(uint);
  void leaderToFollower();
  bool candidateToFollower();
  void toFollower();
  bool compareState(utils::State);
  bool appendClientEntry(uint, uint, uint, uint);
  bool appendMemberAddEntry(uint);

  void consumerFunc(const std::stop_token &);
  void followerFunc(uint, uint, const std::stop_token &);
  void stateSyncFunc();
  ~RaftControl() = default; // call stop on stopTokens, clear vote

  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  RaftClient &raftClient;
  RaftLevelDB db;
  std::atomic<bool> heartbeatRecv;

private:
  void clearQueue();
  void applyLog(bool, int);

  moodycamel::ConcurrentQueue<LogEntry> clientQueue;
  utils::State state;
  std::vector<std::jthread> jthreadVector;
  std::recursive_mutex stateChangeLock;
  uint selfId;
  MachineCountPersistence &machineCountPersistence;
};