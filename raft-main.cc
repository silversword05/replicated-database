#include <raft-control.h>
#include <raft-grpc.h>


int main(int argc, char *argv[]) {
    if (argc != 2) exit(1);

    uint selfId = std::stoi(argv[1]);
    RaftControl raftcontrol("~/raft-home/raft"+std::to_string(selfId), selfId);

    
}