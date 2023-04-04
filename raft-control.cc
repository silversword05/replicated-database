#include <raft-control.h>

void RaftControl::addJthread(std::jthread &&thread) {
  jthreadVector.push_back(std::move(thread));
}

void RaftControl::callStopOnAllThreads() {
  for (auto &thread : jthreadVector)
    thread.request_stop();
}

bool RaftControl::followerToCandidate(uint oldTerm) {
  assertm(state == utils::FOLLOWER, "Follower nahi tha");

  {
    std::lock_guard _(stateChangeLock);
    auto termIncSuccess = electionPersistence.incrementTermAndSelfVote(oldTerm);
    if (!termIncSuccess)
      return termIncSuccess;
    state = utils::CANDIDATE;
    return termIncSuccess;
  }
}

bool RaftControl::candidateToFollower() {
  // TODO: Wrapper to call SetVoteSetVotedFor
  {
    std::lock_guard _(stateChangeLock);
    if (state != utils::CANDIDATE)
      return false;
    state = utils::FOLLOWER;
    return true;
  }
}

void RaftControl::leaderToFollower() {
  assertm(state == utils::LEADER, "Leader nahi tha");

  {
    std::lock_guard _(stateChangeLock);
    callStopOnAllThreads();
    logPersistence.reset();
    state = utils::FOLLOWER;
  }
}

bool RaftControl::candidateToLeader(uint localTerm) {
  std::lock_guard _(stateChangeLock);
  if (state != utils::CANDIDATE)
    return false;

  if (localTerm != electionPersistence.getTerm())
    return false;
  logPersistence.reset();
  logPersistence.markSelfSyncBit();
  // TODO: Spawn threads
  state = utils::LEADER;
  return true;
}

void RaftControl::toFollower() {
  std::lock_guard _(stateChangeLock);
  if (state == utils::FOLLOWER) return;
  if (state == utils::LEADER) {
    leaderToFollower();
    return;
  }
  if (state == utils::CANDIDATE) {
    candidateToFollower();
    return;
  }
  assertm(false, "konsa naya state hai");
}

RaftControl::RaftControl(const std::filesystem::path &homeDir, uint selfId,
                         RaftClient &raftClient_)
    : logPersistence(homeDir, selfId), electionPersistence(homeDir, selfId),
      raftClient(raftClient_) {
  std::lock_guard _(stateChangeLock);
  state = utils::FOLLOWER;

  std::jthread timeoutThread([&]() {
    int localTerm = utils::termStart;

    auto sleepRandom = [] {
      std::mt19937 gen(0);
      std::uniform_int_distribution<> dist{utils::baseSleepTime,
                                           utils::maxSleepTime};
      std::this_thread::sleep_for(std::chrono::milliseconds{dist(gen)});
    };

    auto requestVotes = [&](uint currTerm) {
      std::vector<bool> requestStatus(utils::machineCount, false);
      for (int i = 0; i < utils::machineCount; i++) {
        this->raftClient;
      }
    };

    auto performElecTimeoutCheck = [&](uint currTerm) {
      bool expected = true;
      if (this->heartbeatRecv.compare_exchange_strong(expected, false)) {
        return this->electionPersistence.getTerm();
      } else {
        if (this->followerToCandidate(currTerm)) {
          // TODO: Request Vote RPCs

          // TODO: If success candidate to leader, else candidate to follower
          // TODO: If candidate to leader fails then candidate to follower
          return this->electionPersistence.getTerm();
        } else {
          return this->electionPersistence.getTerm();
        }
      }
    };

    while (true) {
      if (this->state == utils::LEADER) {
        localTerm = this->electionPersistence.getTerm();
        sleepRandom();
      }

      localTerm = performElecTimeoutCheck(localTerm);
      sleepRandom();
    }
  });
  timeoutThread.detach();
}