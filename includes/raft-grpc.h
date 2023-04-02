#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include "utils.h"

class ElectionBookService final
    : public ::replicateddatabase::ElectionBook::Service {
public:
  virtual ::grpc::Status RequestVote(::grpc::ServerContext *,
                                     const ::replicateddatabase::ArgsVote *,
                                     ::replicateddatabase::RetVote *);
};

class LogBookService final : public ::replicateddatabase::LogBook::Service {
public:
  virtual ::grpc::Status AppendEntries(::grpc::ServerContext *,
                                       const ::replicateddatabase::ArgsLog *,
                                       ::replicateddatabase::RetLog *);
};

class LogClient {
  std::unique_ptr<replicateddatabase::LogBook::Stub> stub;

public:
  LogClient(LogClient const &) = delete;
  void operator=(LogClient const &) = delete;

  void sendRpc();

  LogClient()
      : stub(replicateddatabase::LogBook::NewStub(grpc::CreateChannel(
            utils::server_address, grpc::InsecureChannelCredentials()))) {}
};

class ElectionClient {
  std::vector<std::unique_ptr<replicateddatabase::ElectionBook::Stub>> stubVector;

public:
  ElectionClient(ElectionClient const &) = delete;
  void operator=(ElectionClient const &) = delete;

  void sendRpc(uint);

  ElectionClient();

};

class ReplicatedDatabaseServer {
  grpc::ServerBuilder builder;
  ElectionBookService election_book_service;
  LogBookService log_book_service;
  uint selfId;

public:
  void setSelfId(uint);
  auto operator()();
};