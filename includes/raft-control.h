#pragma once

#include <raft-persistence.h>

class Raft {
  Raft(const std::filesystem::path &, uint);
  void addJthread(const std::jthread &);
  void callStopOnAllThreads();

  void followerToCandidate();
  void candidateToLeader();
  void leaderToFollower();
  void candidateToFollower();
  ~Raft(); // call stop on stopTokens, clear vote

private:
  utils::State state;
  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  std::vector<std::jthread> jthreadVector;
};