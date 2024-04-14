#pragma once

#include <cstdint> // uint64_t
#include <memory>  // unique_ptr
#include <string>
#include <vector>

namespace lucanthrope {

class IndexOutput;
class IndexInput;
class LockFile;

// Directory provides an abstraction layer for storing a list of files. A
// directory ontains only files (no sub-folder hierarchy).
//
// Implementing classes must comply with the following:
// - A file in a directory can be created (createOutput()), written to, then
// closed.
// - A file open for writing may not be available for read access until the
// corresponding IndexOutput is closed.
// - Once a file is created it must only be opened for input (openInput()), or
// deleted (deleteFile()). Calling createOutput() on an existing file must
// throw.
class Directory {
public:
  // Closes the directory and releases all resources associated with it
  virtual ~Directory() = default;

  // Returns names of all files stored in this directory.
  // Throws exception in case of I/O error.
  virtual std::vector<std::string> listAll() = 0;

  // Removes an existing file in the directory.
  // Throws exception if fname points to a non-existing file, or in case of I/O
  // error.
  virtual void deleteFile(const std::string &fname) = 0;

  // Returns the byte length of a file in the directory.
  // Throws exception if fname points to a non-existing file, or in case of I/O
  // error.
  virtual uint64_t fileLength(const std::string &fname) = 0;

  // Creates a new, empty file in the directory and returns a pointer to object
  // for writing data to this file. Throws exception if the file already exists,
  // or in case of I/O error.
  virtual std::unique_ptr<IndexOutput>
  createOutput(const std::string &fname) = 0;

  // Renames file src to target, where target must not already exist in the
  // directory. Throws exception if the file named target already exists, or in
  // case of I/O error.
  virtual void rename(const std::string &src, const std::string &target) = 0;

  // Returns a pointer to object for reading an existing file.
  // Note that a file may exist, but be unavailable for reading (because writer
  // didn't finish yet), exception is thrown in such case. Throws exception if
  // fname points to a non-existing file, or in case of I/O error.
  virtual std::unique_ptr<IndexInput> openInput(const std::string &fname) = 0;

  // Acquires and returns a pointer to the lock file for a directory.
  // Used to prevent concurrent write access to the same directory by multiple
  // threads/processes. If somebody else already holds the lock, immediately
  // returns nullptr. I.e., this call does not wait for existing lock to go
  // away. On success, returns a pointer to the object that represents the
  // acquired lock, and creates a lock file in the directory. The returned
  // object's destructor will release the lock. Throws exception in case of I/O
  // error.
  virtual std::unique_ptr<LockFile> obtainLock(const std::string &fname) = 0;

  // Returns true if the named file exists.
  // Throws exception in case of I/O error.
  virtual bool fileExists(const std::string &fname) = 0;

  // Atomically removes all files that belong to the specified segment without
  // throwing. This is indended to be used in exception handlers when an
  // unrecoverable error occurred while writing index files. No-op by deafault.
  virtual void
  deleteSegment([[maybe_unused]] const std::string &segment) noexcept {}
};

} // namespace lucanthrope