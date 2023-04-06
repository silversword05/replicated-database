#include <raft-leveldb.h>

RaftLevelDB::RaftLevelDB(std::filesystem::path homeDir) {
  leveldb::Options options;
  options.create_if_missing = true;

  auto dbFile = homeDir / "db-file";
  if (std::filesystem::exists(dbFile))
    std::filesystem::remove(dbFile);

  leveldb::Status status = leveldb::DB::Open(options, dbFile, &db);
  assertm(status.ok(), ("Level DB start nahi hua" + status.ToString()));
}

void RaftLevelDB::put(uint key, uint val) {
  uint infinity = std::pow(10, utils::intWidth) - 1;
  assertm(key != infinity && val != infinity, "Key val must not be infinity");

  std::string keyString = std::to_string(key);
  std::string valString = std::to_string(val);
  leveldb::WriteOptions writeOptions;
  auto status = db->Put(writeOptions, keyString, valString);
  utils::print("Putting", " ", key, ",", val);
  assertm(status.ok(), "Rocks DB use kar bsdk!!");
}

uint RaftLevelDB::get(uint key) {
  uint infinity = std::pow(10, utils::intWidth) - 1;
  assertm(key != infinity, "Key must not be infinity");

  std::string keyString = std::to_string(key);
  std::string valString;
  auto status = db->Get(leveldb::ReadOptions(), keyString, &valString);
  utils::print("Getting", " ", keyString, ",", valString);
  if (!status.ok())
    return infinity;

  return std::stoi(valString);
}
