#include <client-service.h>

::grpc::Status
ClientServer::ClientAck(::grpc::ServerContext *,
                        const ::replicateddatabase::ArgsAck *args,
                        ::replicateddatabase::RetAck *ret) {
  std::lock_guard _(updateLock);
  assertm(tokenSet.contains(args->reqno()),
          "Request number not in the token set");
  assertm(latencyMap.contains(args->reqno()),
          "Request number not in the latency Map");
  if (tokenSet[args->reqno()] == TokenState::WAITING) {
    auto [startTime, endTime] = latencyMap[args->reqno()];
    assertm(endTime == -1, "endTime getting updated twice");
    if (args->processed()) {
      tokenSet[args->reqno()] = TokenState::SUCCESS;
      latencyMap[args->reqno()].second = utils::getCurrTime();
    } else {
      tokenSet[args->reqno()] = TokenState::FAIL;
      latencyMap[args->reqno()].second =
          -2; // special value to denote failure case
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

  ret->set_idontcare(true);

  return grpc::Status::OK;
}

ClientService::ClientService(uint selfId) {
  clientId = selfId;
  reqNo = 0;
  lastLeaderChannel = -1; // Initialize to -1 implies not set
  // Client-Server Definition
  std::jthread tmp([this, selfId]() {
    grpc::ServerBuilder builder;

    this->serverPtr = std::move([&]() {
      builder.AddListeningPort(utils::getAddress(selfId),
                               grpc::InsecureServerCredentials());
      builder.RegisterService(&(this->server));
      return std::unique_ptr<grpc::Server>(builder.BuildAndStart());
    }());
    this->serverPtr->Wait();
  });

  serverThread = std::move(tmp);

  // Client-RaftServer Channel
  for (uint i = 0; i < utils::machineCount; i++) {
    stubVector.push_back(
        replicateddatabase::RaftBook::NewStub(grpc::CreateChannel(
            utils::getAddress(i), grpc::InsecureChannelCredentials())));
  }
}

ClientService::~ClientService() { serverPtr->Shutdown(); }

std::optional<uint>
ClientService::sendClientRequestRPC(uint clientId, uint reqNo, uint key,
                                    uint val, std::string type, uint toId) {
  assertm(toId < utils::machineCount, "Client Getting Wrong Machine ID");
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
  // Setting these before as this as RTT time being checked
  std::lock_guard _(server.updateLock);
  server.tokenSet[reqNo] = TokenState::WAITING;
  std::pair<long int, long int> timePair(utils::getCurrTime(), -1);
  server.latencyMap[reqNo] = timePair;

  if (lastLeaderChannel != -1) { // In case we have a last put channel
    auto ret = sendClientRequestRPC(clientId, reqNo, key, val, "put",
                                    lastLeaderChannel);
    if (ret.has_value()) {
      return reqNo;
    }
  }

  // Try all channels, get response and update lastLeaderChannel
  for (uint i = 0; i < utils::machineCount; i++) {
    auto ret = sendClientRequestRPC(clientId, reqNo, key, val, "put", i);
    if (ret.has_value()) {
      lastLeaderChannel = i; // Next time we will save time
      return reqNo;
    }
  }

  // Sending failure so remove the entered data
  server.tokenSet.erase(reqNo);
  server.latencyMap.erase(reqNo);

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
  for (uint i = 0; i < utils::machineCount; i++) {
    auto ret = sendClientRequestRPC(clientId, 0, key, 0, "get", i);
    if (ret.has_value()) {
      lastLeaderChannel = i;
      return ret.value();
    }
  }
  return {};
}

void ClientService::outputLatencyMap(std::fstream &outputFs) {
  outputFs << "reqNo,startTime,endTime\n";
  for (auto itr = server.latencyMap.begin(); itr != server.latencyMap.end();
       itr++) {
    auto keyReqNo = itr->first;
    auto [startTime, endTime] = itr->second;

    // utils::print("For reqNo: ", key, " got startTime: ", startTime,
    //              " & endTime: ", endTime);
    outputFs << keyReqNo << "," << startTime << "," << endTime << "\n";
  }
}