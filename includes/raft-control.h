#pragma once

#include <raft-persistence.h>

class RaftControl {
  RaftControl(const std::filesystem::path &, uint);
  void addJthread(const std::jthread &);
  void callStopOnAllThreads();

  bool followerToCandidate(uint oldTerm);
  void candidateToLeader();
  void leaderToFollower();
  bool candidateToFollower();
  ~RaftControl(); // call stop on stopTokens, clear vote

private:
  utils::State state;
  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  std::vector<std::jthread> jthreadVector;
  std::recursive_mutex stateChangeLock;
};