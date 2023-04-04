#pragma once

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

class ClientServer final : public ::replicateddatabase::ClientBook::Service {

public:
  ClientServer() = default;
  virtual ::grpc::Status ClientAck(::grpc::ServerContext *,
                                   const ::replicateddatabase::ArgsAck *,
                                   ::replicateddatabase::RetAck *);
};
