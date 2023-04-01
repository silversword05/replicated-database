#include <raft-control.h>

void Raft::addJthread(const std::jthread& thread) {
    jthreadVector.push_back(std::move(thread));
}

void Raft::callStopOnAllThreads() {
    for(auto& thread : jthreadVector) 
        thread.request_stop();
}