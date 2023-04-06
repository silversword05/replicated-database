#pragma once

#include <leveldb/db.h>
#include <utils.h>

using namespace std;

class RaftLevelDB {
public:
  RaftLevelDB(std::filesystem::path);

  uint get(uint);
  void put(uint, uint);

private:
  leveldb::DB *db;
};
