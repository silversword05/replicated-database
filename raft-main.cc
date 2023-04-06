#include <raft-client.h>
#include <raft-server.h>

int main(int argc, char *argv[]) {
  if (argc != 2)
    exit(1);
  uint selfId = std::stoi(argv[1]);

  auto homeDir = utils::home_dir / "raft-home/raft" / std::to_string(selfId);
  if constexpr (utils::cleanStart)
    std::filesystem::remove_all(homeDir);
  std::filesystem::create_directories(homeDir);

  RaftClient raftClient{homeDir};
  RaftControl raftControl(homeDir, selfId, raftClient);

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