#include <client-service.h>

::grpc::Status
ClientServer::ClientAck(::grpc::ServerContext *,
                                       const ::replicateddatabase::ArgsAck *,
                                       ::replicateddatabase::RetAck *) {

  return grpc::Status::OK;
}

int main() {
    
}