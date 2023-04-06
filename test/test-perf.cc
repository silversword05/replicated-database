#include <client-service.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    utils::print("./test-perf clientId numRequests Throughput");
    exit(1);
  }
  uint clientId = std::stoi(argv[1]);
  if (clientId < utils::machineCount) {
    utils::print("Client Id (", clientId,
                 ") cannot be less than machine count (", utils::machineCount,
                 ").");
    exit(1);
  }

  ClientService service(clientId);

  uint reqs = std::stoi(argv[2]);
  uint throughput = std::stoi(argv[3]); // reqs/sec
  uint microSleep = 1000 * 1000 / throughput;

  std::unordered_set<uint> reqNumSet;

  auto start = utils::getCurrTime();
  // Do some put operations
  for (uint i = 0; i < reqs; i++) {
    auto val = service.put(i, i + 100);
    // std::this_thread::sleep_for(std::chrono::microseconds{microSleep});
    if (val.has_value()) {
      reqNumSet.insert(val.value());
    }
  }
  auto timeInSec = utils::getCurrTime() - start;
  auto orgThrput = reqs / timeInSec;

  utils::print(reqs, "put requests done with original Throughput of",
               orgThrput, "!!");

  // check if put done
  for (auto elem : reqNumSet) {
    while (true) {
      auto check = service.checkPutDone(elem);
      if (check.has_value()) {
        // utils::print("Request No: ", elem, " has result: ",
        //   check.value());
        break;
      }
    }
  }

  std::fstream outputFs;
  outputFs = std::fstream(utils::home_dir / "raft-home/clientLatency.csv",
                          std::ios::out | std::ios::trunc);
  service.outputLatencyMap(outputFs);

  return 0;
}