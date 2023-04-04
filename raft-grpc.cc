#include <raft-server.h>
#include <raft-client.h>

RaftClient::RaftClient() {
  for (uint i = 0; i < utils::machineCount; i++) {
    stubVector.push_back(
        replicateddatabase::RaftBook::NewStub(grpc::CreateChannel(
            utils::getAddress(i), grpc::InsecureChannelCredentials())));
  }
}

RaftServer::RaftServer(RaftControl &raftControl_) : raftControl(raftControl_) {}

::grpc::Status
RaftServer::RequestVote(::grpc::ServerContext *,
                        const ::replicateddatabase::ArgsVote *args,
                        ::replicateddatabase::RetVote *ret) {
  std::lock_guard _(requestVoteLock);
  auto lastLogIndexAndTerm = raftControl.logPersistence;
  if (lastLogIndexAndTerm.term > args->lastLogTerm()) {
    // cant vote
    ret->set_voteGranted(false);
    ret->set_term(raftControl.electionPersistence.getTerm());
    return grpc::Status::OK;
  }


  return grpc::Status::OK;
}

std::optional<bool> RaftClient::sendRequestVoteRpc(uint term, uint candidateId, int lastLogIndex, int lastLogTerm) {
  assertm(candidateId < utils::machineCount, "nayi machine");
  grpc::ClientContext context;
  replicateddatabase::ArgsVote query;
  replicateddatabase::RetVote response;

  query.set_term(term);
  query.set_candidateid(candidateId);
  query.set_lastlogindex(lastLogIndex);
  query.set_lastlogterm(lastLogTerm);

  auto status = stubVector[candidateId]->RequestVote(&context, query, &response);
  if(!status.ok())
    return {};
  return response.votegranted();
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