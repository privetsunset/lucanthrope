// PRIVATE HEADER
#pragma once

#include <string>

#include "storage/LockFile.h"
#include "storage/RAMDirectory.h"

namespace lucanthrope {

class RAMDirectoryLockFile : public LockFile {
private:
  RAMDirectory *parent;
  std::string name;

public:
  RAMDirectoryLockFile(RAMDirectory *dir, const std::string &fname)
      : parent(dir), name(fname) {}
  virtual ~RAMDirectoryLockFile() override { parent->releaseLock(name); }
};

} // namespace lucanthrope