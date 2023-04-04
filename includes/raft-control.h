#pragma once

#include <raft-client.h>
#include <raft-persistence.h>
#include <concurrentqueue.h>

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

  void consumerFunc();
  void followerFunc(uint);
  ~RaftControl() = default; // call stop on stopTokens, clear vote

  LogPersistence logPersistence;
  ElectionPersistence electionPersistence;
  RaftClient &raftClient;
  std::atomic<bool> heartbeatRecv;
private:
  moodycamel::ConcurrentQueue<uint> clientQueue;
  utils::State state;
  std::vector<std::jthread> jthreadVector;
  std::recursive_mutex stateChangeLock;
};