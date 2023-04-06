#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>

#include <replicated-database.grpc.pb.h>
#include <replicated-database.pb.h>

#include <raft-persistence.h>

class RaftClient {
  std::vector<std::unique_ptr<replicateddatabase::RaftBook::Stub>> stubVector;
  std::unordered_map<int, std::unique_ptr<replicateddatabase::ClientBook::Stub>> stubMap;
  MachineCountPersistence &machineCountPersistence;

public:
  RaftClient(const std::filesystem::path &);
  RaftClient(RaftClient const &) = delete;
  void operator=(RaftClient const &) = delete;

  auto& getClientStub(uint);

  std::optional<bool> sendRequestVoteRpc(uint, uint, int, int, uint);
  std::optional<bool> sendAppendEntryRpc(uint, uint, uint, int, std::string, int, uint);
  void sendClientAck(uint, uint, bool);
};