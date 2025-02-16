#pragma once

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include <raft-control.h>

class RaftServer final : public ::replicateddatabase::RaftBook::Service {
  RaftControl &raftControl;
  std::recursive_mutex rpcLock;

public:
  RaftServer(RaftControl &);
  virtual ::grpc::Status RequestVote(::grpc::ServerContext *,
                                     const ::replicateddatabase::ArgsVote *,
                                     ::replicateddatabase::RetVote *);
  virtual ::grpc::Status AppendEntries(::grpc::ServerContext *,
                                       const ::replicateddatabase::ArgsAppend *,
                                       ::replicateddatabase::RetAppend *);
  virtual ::grpc::Status
  ClientRequest(::grpc::ServerContext *,
                const ::replicateddatabase::ArgsRequest *,
                ::replicateddatabase::RetRequest *);
};
