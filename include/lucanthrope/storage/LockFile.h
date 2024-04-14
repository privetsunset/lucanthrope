#pragma once

#include <string>

namespace lucanthrope {

// Identifies a lock file which protects directory from concurrent write access.
// The lifetime of a LockFile object should be managed only by ObtainLock() and
// destructor.
class LockFile {
public:
  LockFile() = default;
  virtual ~LockFile() = default; // this has to release lock

private:
  // No copying allowed
  LockFile(const LockFile &) = delete;
  void operator=(const LockFile &) = delete;
};

} // namespace lucanthrope