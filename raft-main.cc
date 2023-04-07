#include <cxxopts.hpp>
#include <raft-client.h>
#include <raft-server.h>

cxxopts::Options setOptions() {
  cxxopts::Options options("Raft Service", "Server machines for Raft");
  options.add_options()("h,help", "Print usage")("i,id", "Machine Id (uint)",
                                                 cxxopts::value<uint>())(
      "j,join", "Join the cluster (bool/flag)",
      cxxopts::value<bool>()->default_value("false"));
  return options;
}

void startRaft(const auto &homeDir, const auto selfId) {
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

bool startJoinRequest(const auto selfId) {
  MemberClient().sendAddMemberRpc(selfId);

  std::promise<bool> promiseSuccess;
  auto futureSuccess = promiseSuccess.get_future();

  grpc::ServerBuilder builder;
  MemberServer memberServer(std::move(promiseSuccess));

  auto server = [&]() {
    builder.AddListeningPort(utils::getAddress(selfId),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&memberServer);
    return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
  }();
  std::jthread _([&] { server->Wait(); });

  auto success = futureSuccess.get();
  server->Shutdown();
  return success;
}

int main(int argc, char *argv[]) {
  auto options = setOptions();
  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  uint selfId = result["id"].as<uint>();
  auto homeDir = utils::home_dir / "raft-home/raft" / std::to_string(selfId);
  if constexpr (utils::cleanStart)
    std::filesystem::remove_all(homeDir);
  std::filesystem::create_directories(homeDir);

  if (result["join"].as<bool>()) {
    auto success = startJoinRequest(selfId);
    utils::print("Join result", success);
    utils::print("New machine count", selfId + 1);
    MachineCountPersistence::getInstance(homeDir).setMachineCount(selfId + 1);
    // TODO: Increase self machine count on success
    // Discuss: Has to change machine count on updateCommitIndex
  } else {
    startRaft(homeDir, selfId);
  }
}