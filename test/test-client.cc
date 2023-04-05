#include <client-service.h>

int main(int argc, char *argv[]) {
  if (argc != 2)
    exit(1);
  uint clientId = std::stoi(argv[1]);
  if (clientId < utils::machineCount) {
    utils::print("Client Id (", clientId,
                 ") cannot be less than machine count (", utils::machineCount,
                 ").");
    exit(1);
  }

  ClientService service(clientId);

  uint reqs = 10;
  // Do some put operations
  for (uint i = 0; i < reqs; i++) {
    service.put(i, i + 100);
  }

  utils::print("Send some put requests!!");

  // check if put done
  for (uint i = 0; i < reqs; i++) {
    while (true) {
      auto check = service.checkPutDone(i);
      if (check.has_value()) {
        utils::print("Request No: ", i, " has result: ", check.value());
        break;
      }
    }
  }

  return 0;
}