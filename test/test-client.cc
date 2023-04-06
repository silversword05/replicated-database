#include <client-service.h>

int main(int argc, char *argv[]) {
  if (argc != 2)
    exit(1);
  uint clientId = std::stoi(argv[1]);
  if (clientId < utils::initialMachineCount) {
    utils::print("Client Id (", clientId,
                 ") cannot be less than machine count (", utils::initialMachineCount,
                 ").");
    exit(1);
  }

  ClientService service(clientId);

  uint reqs = 10;

  std::unordered_set<uint> reqNumSet;
  // Do some put operations
  for (uint i = 0; i < reqs; i++) {
    auto val = service.put(i, i + 100);
    if (val.has_value()) {
      reqNumSet.insert(val.value());
    }
  }

  utils::print("Send some put requests!!");

  // check if put done
  for (auto elem : reqNumSet) {
    while (true) {
      auto check = service.checkPutDone(elem);
      if (check.has_value()) {
        utils::print("Request No: ", elem, " has result: ", check.value());
        break;
      }
    }
  }

  auto val = service.get(100);
  utils::print("Retrived val ",val.value());

  return 0;
}