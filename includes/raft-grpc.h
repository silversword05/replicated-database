#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include "utils.h"

class RaftBookService final
    : public ::replicateddatabase::RaftBook::Service {
public:
  virtual ::grpc::Status RequestVote(::grpc::ServerContext *,
                                     const ::replicateddatabase::ArgsVote *,
                                     ::replicateddatabase::RetVote *);
};

class RaftClient {
  std::vector<std::unique_ptr<replicateddatabase::RaftBook::Stub>> stubVector;

public:
  RaftClient();
  RaftClient(RaftClient const &) = delete;
  void operator=(RaftClient const &) = delete;

  void sendRpc(uint);

};

class ReplicatedDatabaseServer {
  grpc::ServerBuilder builder;
  RaftBookService raft_book_service;
  uint selfId;

public:
  void setSelfId(uint);
  auto operator()();
};