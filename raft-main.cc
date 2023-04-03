#include <raft-server.h>
#include <raft-client.h>

int main(int argc, char *argv[]) {
  if (argc != 2)
    exit(1);
  uint selfId = std::stoi(argv[1]);

  auto homeDir = std::filesystem::path("~/raft-home/raft" + std::to_string(selfId));
  std::filesystem::create_directories(homeDir);

  RaftClient raftClient;
  RaftControl raftControl(
      homeDir,
      selfId, raftClient);

  grpc::ServerBuilder builder;
  RaftServer raftServer(raftControl);

  auto server = [&]() {
    builder.AddListeningPort(utils::getAddress(selfId),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&raftServer);
    return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
  }();
  server->Wait();
}