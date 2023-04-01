#include <raft-control.h>

void RaftControl::addJthread(const std::jthread &thread) {
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
  assertm(state == utils::CANDIDATE, "Candidate nahi tha");

  {
    std::lock_guard _(stateChangeLock);
    state = utils::FOLLOWER;
  }
}