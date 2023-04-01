#include "includes/raft.h"

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

  ElectionClient().sendRpc();
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

void ElectionClient::sendRpc() {
  grpc::ClientContext context;
  replicateddatabase::ArgsVote query;
  replicateddatabase::RetVote response;

  query.set_message("Hello Election");
  stub->RequestVote(&context, query, &response);
}

int main() {

  ReplicatedDatabaseServer replicateddatabaseServer;
  auto server = replicateddatabaseServer();

  std::jthread t1([&]() { server->Wait(); });
  std::cout << "Server thread started " << std::endl;

  std::jthread t2([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    LogClient().sendRpc();
    std::cout << "RPC complete" << std::endl;
    server->Shutdown();
    std::cout << "Server shutdown complete " << std::endl;
  });
  std::cout << "log thread started " << std::endl;

  return 0;
}