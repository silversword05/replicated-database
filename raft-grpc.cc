#include "includes/raft-grpc.h"

void ReplicatedDatabaseServer::setSelfId(uint id) {
  selfId = id;
}

auto ReplicatedDatabaseServer::operator()() {
  builder.AddListeningPort(utils::getAddress(selfId),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&raft_book_service);

  return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
}

RaftClient::RaftClient() {
  for(uint i = 0; i < utils::machineCount; i++) {
    stubVector.push_back(std::move(replicateddatabase::RaftBook::NewStub(grpc::CreateChannel(
            utils::getAddress(i), grpc::InsecureChannelCredentials()))));
  }
}

::grpc::Status
RaftBookService::RequestVote(::grpc::ServerContext *,
                                 const ::replicateddatabase::ArgsVote *args,
                                 ::replicateddatabase::RetVote *ret) {
  std::cout << "In request vote " << args->message() << std::endl;
  ret->set_response(true);
  return grpc::Status::OK;
}

void RaftClient::sendRpc(uint machineId) {
  assertm(machineId < utils::machineCount,"nayi machine");
  grpc::ClientContext context;
  replicateddatabase::ArgsVote query;
  replicateddatabase::RetVote response;

  query.set_message("Hello Election");
  stubVector[machineId]->RequestVote(&context, query, &response);
}

int main(int argc, char *argv[]) {

  if (argc != 2) exit(1);
  ReplicatedDatabaseServer replicateddatabaseServer;
  replicateddatabaseServer.setSelfId(std::stoi(argv[1]));
  auto server = replicateddatabaseServer();

  std::jthread t1([&]() { server->Wait(); });
  std::cout << "Server thread started " << std::endl;

  std::jthread t2([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    RaftClient().sendRpc(0);
    std::cout << "RPC complete" << std::endl;
    RaftClient().sendRpc(1);
    std::cout << "RPC complete" << std::endl;
    RaftClient().sendRpc(2);
    std::cout << "RPC complete" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(10));
    server->Shutdown();
    std::cout << "Server shutdown complete " << std::endl;
  });
  std::cout << "log thread started " << std::endl;

  return 0;
}