#include <client-service.h>

::grpc::Status
ClientServer::ClientAck(::grpc::ServerContext *,
                        const ::replicateddatabase::ArgsAck *args,
                        ::replicateddatabase::Empty *) {
  return grpc::Status::OK;
  std::string tmp;
  if (args->processed()) {
    tmp = "+ve";
  }
  else {
    tmp = "-ve";
  }
  assertm(tokenSet.contains(args->reqno()),
          "Request number not in the token set");
  if (tokenSet[args->reqno()] == TokenState::WAITING) {
    utils::print2("Received", tmp, "ACK for request Num:", args->reqno());
    if (args->processed()) {
      tokenSet[args->reqno()] = TokenState::SUCCESS;
    } else {
      tokenSet[args->reqno()] = TokenState::FAIL;
    }
  } else {
    if (args->processed()) {
      assertm(tokenSet[args->reqno()] == TokenState::SUCCESS,
              "Got SUCCESS then a FAIL!!");
    } else {
      assertm(tokenSet[args->reqno()] == TokenState::FAIL,
              "Got FAIL then SUCESS!!");
    }
  }

  return grpc::Status::OK;
}

ClientService::ClientService(uint selfId) {
  clientId = selfId;
  reqNo = 0;
  // Client-Server Definition
  std::jthread tmp([this, selfId]() {
    grpc::ServerBuilder builder;

    this->serverPtr = std::move([&]() {
      builder.AddListeningPort(utils::getClientAddress(),
                               grpc::InsecureServerCredentials());
      builder.RegisterService(&(this->server));
      return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
    }());
    this->serverPtr->Wait();
  });

  serverThread = std::move(tmp);

  // Client-RaftServer Channel
  for (uint i = 0; i < utils::maxMachineCount; i++) {
    stubVector.push_back(
        replicateddatabase::RaftBook::NewStub(grpc::CreateChannel(
            utils::getAddress(i), grpc::InsecureChannelCredentials())));
  }
}

ClientService::~ClientService() { serverPtr->Shutdown(); }

std::optional<uint>
ClientService::sendClientRequestRPC(uint clientId, uint reqNo, uint key,
                                    uint val, std::string type, uint toId) {
  utils::print("Send Request from the client to server ", toId);
  assertm(toId < utils::maxMachineCount, "Client Getting Wrong Machine ID");
  grpc::ClientContext context;
  replicateddatabase::ArgsRequest query;
  replicateddatabase::RetRequest response;

  query.set_clientid(clientId);
  query.set_reqno(reqNo);
  query.set_key(key);
  query.set_val(val);
  query.set_type(type);

  auto status = stubVector[toId]->ClientRequest(&context, query, &response);
  if (!status.ok()) {
    return {};
  }
  if (!response.success()) {
    return {};
  }
  return response.val();
}

bool ClientService::putBlocking(uint key, uint val) {
  auto ret = put(key, val);
  if (ret.has_value()) {
    while (server.tokenSet[ret.value()] == TokenState::WAITING)
      ;

    if (server.tokenSet[ret.value()] == TokenState::FAIL)
      return false;
    if (server.tokenSet[ret.value()] == TokenState::SUCCESS)
      return true;

    assertm(false, "Should Not reach here");
  }

  return false;
}

std::optional<uint> ClientService::put(uint key, uint val) {
  reqNo++;
  if (lastLeaderChannel != -1) { // In case we have a last put channel
    auto ret = sendClientRequestRPC(clientId, reqNo, key, val, "put",
                                    lastLeaderChannel);
    if (ret.has_value()) {
      server.tokenSet[reqNo] = TokenState::WAITING;
      return reqNo;
    }
  }
  for (uint i = 0; i < utils::maxMachineCount; i++) {
    auto ret = sendClientRequestRPC(clientId, reqNo, key, val, "put", i);
    if (ret.has_value()) {
      lastLeaderChannel = i; // Next time we will save time
      server.tokenSet[reqNo] = TokenState::WAITING;
      return reqNo;
    }
  }

  return {};
}

std::optional<bool> ClientService::checkPutDone(uint requestNum) {
  if (server.tokenSet[requestNum] == TokenState::WAITING)
    return {};
  if (server.tokenSet[requestNum] == TokenState::SUCCESS)
    return true;
  return false;
}

std::optional<uint> ClientService::get(uint key) {
  if (lastLeaderChannel != -1) {
    auto ret =
        sendClientRequestRPC(clientId, 0, key, 0, "get", lastLeaderChannel);
    if (ret.has_value())
      return ret.value();
  }

  // Try all channels and update last get channel
  for (uint i = 0; i < utils::maxMachineCount; i++) {
    auto ret = sendClientRequestRPC(clientId, 0, key, 0, "get", i);
    if (ret.has_value()) {
      lastLeaderChannel = i;
      return ret.value();
    }
      
  }
  return {};
}