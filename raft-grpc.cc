#include "includes/raft-grpc.h"

void ReplicatedDatabaseServer::setSelfId(uint id) {
  selfId = id;
}

auto ReplicatedDatabaseServer::operator()() {
  builder.AddListeningPort(utils::getAddress(selfId),
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&election_book_service);
  builder.RegisterService(&log_book_service);

  return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
}

ElectionClient::ElectionClient() {
  for(uint i = 0; i < utils::machineCount; i++) {
    stubVector.push_back(std::move(replicateddatabase::ElectionBook::NewStub(grpc::CreateChannel(
            utils::getAddress(i), grpc::InsecureChannelCredentials()))));
  }
}

::grpc::Status
ElectionBookService::RequestVote(::grpc::ServerContext *,
                                 const ::replicateddatabase::ArgsVote *args,
                                 ::replicateddatabase::RetVote *ret) {
  std::cout << "In request vote " << args->message() << std::endl;
  ret->set_response(true);
  return grpc::Status::OK;
}

::grpc::Status
LogBookService::AppendEntries(::grpc::ServerContext *,
                              const ::replicateddatabase::ArgsLog *args,
                              ::replicateddatabase::RetLog *ret) {
  std::cout << "In append entries " << args->message() << std::endl;
  ret->set_response(true);

//  ElectionClient().sendRpc();
  return grpc::Status::OK;
}

void LogClient::sendRpc() {
  grpc::ClientContext context;
  replicateddatabase::ArgsLog query;
  replicateddatabase::RetLog response;

  query.set_message("Hello Log");
  std::cout << "Sending query" << std::endl;
  stub->AppendEntries(&context, query, &response);
}

void ElectionClient::sendRpc(uint machineId) {
  assertm(machineId < utils::machineCount,"nayi machine");
  grpc::ClientContext context;
  replicateddatabase::ArgsVote query;
  replicateddatabase::RetVote response;

  query.set_message("Hello Election");
  stubVector[machineId]->RequestVote(&context, query, &response);
}

// int main(int argc, char *argv[]) {

//   if (argc != 2) exit(1);
//   ReplicatedDatabaseServer replicateddatabaseServer;
//   replicateddatabaseServer.setSelfId(std::stoi(argv[1]));
//   auto server = replicateddatabaseServer();
//   std::jthread t1([&]() { server->Wait(); });
//   std::cout << "Server thread started " << std::endl;

//   std::jthread t2([&]() {
//     std::this_thread::sleep_for(std::chrono::seconds(10));
//     ElectionClient().sendRpc(0);
//     std::cout << "RPC complete" << std::endl;
//     ElectionClient().sendRpc(1);
//     std::cout << "RPC complete" << std::endl;
//     ElectionClient().sendRpc(2);
//     std::cout << "RPC complete" << std::endl;
//     server->Shutdown();
//     std::cout << "Server shutdown complete " << std::endl;
//   });
//   std::cout << "log thread started " << std::endl;

//   return 0;
// }