#include <client-service.h>

int main() {
  uint clientId = 9;
  ClientService service(clientId);

  uint i=0;
  while(true) {
    utils::print2("CLIENT putting key:", i, "val:", i + 100);
    service.put(i, i + 100);
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});
    i++;
  }

  return 0;
}