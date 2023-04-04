#include <raft-control.h>

void RaftControl::addJthread(std::jthread &&thread) {
  jthreadVector.push_back(std::move(thread));
}

void RaftControl::callStopOnAllThreads() { jthreadVector.clear(); }

bool RaftControl::followerToCandidate(uint &oldTerm) {
  assertm(state == utils::FOLLOWER, "Follower nahi tha");

  {
    std::lock_guard _(stateChangeLock);
    auto termIncSuccess = electionPersistence.incrementTermAndSelfVote(oldTerm);
    if (!termIncSuccess)
      return termIncSuccess;
    oldTerm++;
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
  // TODO: Add dummy entry

  LogEntry logEntry;
  logEntry.fillDummyEntry();
  logEntry.term = electionPersistence.getTerm();
  logPersistence.appendLog(logEntry);

  utils::print("Changing the Machine: ", this->selfId,
               " to LEADER for the term: ", localTerm);
  addJthread(
      std::jthread([this](std::stop_token s) { this->consumerFunc(s); }));
  for (uint i = 0; i < utils::machineCount; i++) {
    if (i == selfId)
      continue;
    addJthread(std::jthread([this, i, localTerm](std::stop_token s) {
      this->followerFunc(localTerm, i, s);
    }));
  }
  state = utils::LEADER;
  return true;
}

void RaftControl::toFollower() {
  std::lock_guard _(stateChangeLock);
  if (state == utils::FOLLOWER)
    return;
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

bool RaftControl::compareState(utils::State expected) {
  std::lock_guard _(stateChangeLock);
  return state == expected;
}

void RaftControl::consumerFunc(const std::stop_token &s) {
  LogEntry logEntry;
  while (clientQueue.try_dequeue(logEntry) && !s.stop_requested()) {
    logEntry.term = electionPersistence.getTerm();
    logPersistence.appendLog(logEntry);
  }
  std::this_thread::yield();
}

void RaftControl::followerFunc(uint localTerm, uint candidateId,
                               const std::stop_token &s) {
  int lastSyncIndex = logPersistence.readLastCommitIndex() + 1;
  while (!s.stop_requested()) {
    auto logEntry = logPersistence.readLog(lastSyncIndex);
    if (logEntry.has_value()) {
      int prevLogTerm = -1;
      auto lastToOneEntry = logPersistence.readLog(lastSyncIndex - 1);
      if (lastToOneEntry.has_value())
        prevLogTerm = lastToOneEntry.value().term;

      if (s.stop_requested())
        break;
      auto syncSuccess = raftClient.sendAppendEntryRpc(
          localTerm, selfId, lastSyncIndex, prevLogTerm,
          logEntry.value().getString(), logPersistence.readLastCommitIndex(),
          candidateId);
      if (syncSuccess.has_value() && syncSuccess.value()) {
        auto shouldCommit = (localTerm == logEntry.value().term);
        logPersistence.markLogSyncBit(lastSyncIndex, candidateId, shouldCommit);
        lastSyncIndex++;
      } else if (syncSuccess.has_value() && !syncSuccess.value()) {
        lastSyncIndex--;
      }
    } else {
      // blank heartbeat
      int prevLogTerm = -1;
      auto lastToOneEntry = logPersistence.readLog(lastSyncIndex - 1);
      if (lastToOneEntry.has_value())
        prevLogTerm = lastToOneEntry.value().term;
      auto syncSuccess = raftClient.sendAppendEntryRpc(
          localTerm, selfId, lastSyncIndex, prevLogTerm, "",
          logPersistence.readLastCommitIndex(), candidateId);
      if (syncSuccess.has_value() && !syncSuccess.value()) {
        lastSyncIndex--;
      }
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds{utils::followerSleep});
  }
}

RaftControl::RaftControl(const std::filesystem::path &homeDir, uint selfId,
                         RaftClient &raftClient_)
    : logPersistence(homeDir, selfId), electionPersistence(homeDir, selfId),
      raftClient(raftClient_), selfId(selfId) {
  std::lock_guard _(stateChangeLock);
  state = utils::FOLLOWER;

  std::jthread timeoutThread([this, selfId]() {
    uint localTerm = this->electionPersistence.getTerm();

    auto sleepRandom = [] {
      std::mt19937 gen(0);
      std::uniform_int_distribution<> dist{utils::baseSleepTime,
                                           utils::maxSleepTime};
      std::this_thread::sleep_for(std::chrono::milliseconds{dist(gen)});
    };

    auto requestVotes = [&] {
      uint majorityCount = 1;
      for (uint i = 0; i < utils::machineCount; i++) {
        if (i == selfId)
          continue;
        auto [lastLogIndex, lastLogTerm] =
            this->logPersistence.getLastLogData();
        auto voteGranted = this->raftClient.sendRequestVoteRpc(
            localTerm, selfId, lastLogIndex, lastLogTerm, i);
        if (!voteGranted.has_value())
          continue;
        if (!voteGranted.value())
          return false;
        majorityCount++;
      }
      return (majorityCount > (utils::machineCount) / 2);
    };

    auto performElecTimeoutCheck = [&] {
      bool expected = true;
      if (this->heartbeatRecv.compare_exchange_strong(expected, false)) {
        return this->electionPersistence.getTerm();
      } else {
        if (this->followerToCandidate(localTerm)) {
          if (requestVotes()) {
            if (!this->candidateToLeader(localTerm))
              assertm(!this->candidateToFollower(),
                      "Pahele to candidate baan jana chaiye");
          } else {
            this->candidateToFollower();
          }
          return this->electionPersistence.getTerm();
        } else {
          return this->electionPersistence.getTerm();
        }
      }
    };

    while (true) {
      sleepRandom();
      if (this->compareState(utils::LEADER))
        localTerm = this->electionPersistence.getTerm();
      else
        localTerm = performElecTimeoutCheck();
    }
  });
  timeoutThread.detach();
}