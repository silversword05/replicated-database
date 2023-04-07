#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include <utils.h>

enum TokenState { WAITING, SUCCESS, FAIL };

class ClientServer final : public ::replicateddatabase::ClientBook::Service {
public:
  std::unordered_map<uint, TokenState> tokenSet;

  ClientServer() = default;
  virtual ::grpc::Status ClientAck(::grpc::ServerContext *,
                                   const ::replicateddatabase::ArgsAck *,
                                   ::replicateddatabase::Empty *);
};

class ClientService {
  std::vector<std::unique_ptr<replicateddatabase::RaftBook::Stub>> stubVector;
  std::jthread serverThread;
  uint reqNo;
  uint clientId;
  int lastLeaderChannel{-1};

public:
  ClientServer server;

  ClientService(uint);
  ~ClientService();
  std::optional<uint> sendClientRequestRPC(uint, uint, uint, uint, std::string, uint);


  bool putBlocking(uint, uint);
  std::optional<uint> put(uint, uint);
  std::optional<bool> checkPutDone(uint);
  std::optional<uint> get(uint);

private:
  std::unique_ptr<::grpc::Server> serverPtr;
};