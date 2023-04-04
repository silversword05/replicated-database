#pragma once

#include <raft-client.h>
#include <raft-persistence.h>

class RaftControl {
public:
  RaftControl(const std::filesystem::path &, uint, RaftClient &);
  void addJthread(std::jthread &&);
  void callStopOnAllThreads();

  bool followerToCandidate(uint oldTerm);
  bool candidateToLeader(uint);
  void leaderToFollower();
  bool candidateToFollower();
  void toFollower();
  bool compareState(utils::State);
  ~RaftControl() = default; // call stop on stopTokens, clear vote

  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  RaftClient &raftClient;
private:
  utils::State state;
  std::vector<std::jthread> jthreadVector;
  std::recursive_mutex stateChangeLock;
  std::atomic<bool> heartbeatRecv;
};