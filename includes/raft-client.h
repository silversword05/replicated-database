#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include <utils.h>

class RaftClient {
  std::vector<std::unique_ptr<replicateddatabase::RaftBook::Stub>> stubVector;

public:
  RaftClient();
  RaftClient(RaftClient const &) = delete;
  void operator=(RaftClient const &) = delete;

  void sendRpc(uint);
  void sendRequestVoteRpc(uint);
};