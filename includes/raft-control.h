#pragma once

#include <raft-persistence.h>

class RaftControl {
public:
  RaftControl(const std::filesystem::path &, uint);
  void addJthread(std::jthread &&);
  void callStopOnAllThreads();
  void resetLogAndElection();

  bool followerToCandidate(uint oldTerm);
  bool candidateToLeader(uint);
  void leaderToFollower();
  bool candidateToFollower();
  ~RaftControl() = default; // call stop on stopTokens, clear vote

private:
  utils::State state;
  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  std::vector<std::jthread> jthreadVector;
  std::recursive_mutex stateChangeLock;
  std::atomic<bool> heartbeatRecv;
};