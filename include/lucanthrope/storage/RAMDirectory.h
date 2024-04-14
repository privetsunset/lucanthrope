#pragma once

#include <algorithm> // min()
#include <cassert>
#include <cstdint> // uint64_t
#include <cstring> // memcpy()
#include <memory>  // unique_ptr
#include <mutex>
#include <string>
#include <unordered_map>

#include "Directory.h"

namespace lucanthrope {

// Ideally, we want these to have static linkage, but that will not allow us to
// make them friends of RAMDirectory, which is required
class RAMFileIndexOutput;
class RAMFileIndexInput;
class RAMDirectoryLockFile;

class RAMDirectory : public Directory {
  friend RAMFileIndexOutput;
  friend RAMFileIndexInput;
  friend RAMDirectoryLockFile;
private:
  // About {thread,exception,memory}-safety:
  // RAMFile object is first created by RAMFileIndexOutput in its constructor.
  // If any manipulation on RAMFile object through RAMFileIndexOutput throws
  // (which is unlikely), then RAMFile object will be commited to RAMDirectory
  // by RAMFileIndexOutput's destructor, and exception will propagate further.
  // This means that half-written RAMFile object will be present in directory,
  // but then invoked exception handler will (hopefully) manage this. Anyway,
  // when RAMFile object is commited to directory, it gets placed in files map,
  // obtains ref count 1, and its contents become visible to other threads via
  // RAMDirectory's mutex. Then, any openInput() will increase ref count by 1,
  // and subsequent RAMFileIndexInput's destructor call will decrease it.
  // deleteFile() on RAMFile object will remove it from files map and decrease
  // its ref count. RAMFile object will only be deallocated if it has ref count
  // 0, which means that it was removed from directory via deleteFile() and
  // there are no readers. If RAMFile object was removed from directory via
  // deleteFile(), but there is at least one reader, then RAMFile object's
  // deallocation will be triggered by the last reader that will close its
  // stream. All changes to ref count are synchronized with each other by
  // RAMDirectory's mutex.
  // To conclude, if pointers to IndexOutput and IndexInput are properly
  // managed, then it looks like RAMFile objects are thread-safe,
  // exception-safe, and memory-safe
  struct RAMFile {
    static constexpr int kBlockSize = 4096;
    // File state
    // Array of allocated memory blocks
    std::vector<char *> blocks_;
    // Current file size
    uint64_t length = 0;
    // Reference count is managed by RAMDirectory once RAMFile object is
    // commited to directory, which always happens even when exception were
    // thrown.
    mutable int refs_ = 0;
    uint64_t lastModified = 0;

    // These are used by for parent.commit(name), parent.unrefReader()
    const std::string name;
    RAMDirectory *parent;

    // This is only called to "initialize" RAMDirectory::dummy_file
    RAMFile() {}

    RAMFile(RAMDirectory *d, const std::string &fname) noexcept
        : name(fname), parent(d) {}

    RAMFile(const RAMFile &) = delete;
    RAMFile &operator=(const RAMFile &) = delete;

    ~RAMFile() {
      assert(refs_ == 0 && "Reference count is not zero");
      for (char *block : blocks_)
        delete[] block;
    }

    void alloc() {
      char *result = new char[kBlockSize];
      blocks_.push_back(result);
    }

    uint64_t size() const { return length; }

    void finish_writing() { parent->commit(name, this); }
    void finish_reading() const { parent->unrefReader(this); }
  };

  // all uncomitted/lock files point to this file
  static const RAMFile dummy_file;
  std::mutex mu_;
  std::unordered_map<std::string, RAMFile *> files;

  // Private member functions that are called only by RAMFile and RAMDirectoryLockFile:

  // Installs file into files map. After this, file is available for reading
  // from multiple threads, its reference count is set to 1, and its contents
  // cannot be modified. This is called by RAMFile's finish_writing(), which is
  // in turn called by RAMFileIndexOutput's destructor.
  void commit(const std::string &fname, RAMFile *file) noexcept;

  // Decrements file's reference count. If file's reference count becomes zero,
  // it gets deallocated. This is called by RAMFile's finish_reading(), which is
  // in turn called by RAMFileIndexInput's destructor.
  void unrefReader(const RAMFile *file) noexcept;

  // Removes lock file from directory.
  // This is called by RAMDirectoryLockFile's destructor.
  void releaseLock(const std::string &fname) noexcept;

public:
  RAMDirectory() = default;

  // REQUIRES: no one is holding a file for reading or writing
  virtual ~RAMDirectory() override;

  virtual std::vector<std::string> listAll() override;

  virtual void deleteFile(const std::string &fname) override;

  virtual uint64_t fileLength(const std::string &fname) override;

  virtual std::unique_ptr<IndexOutput>
  createOutput(const std::string &fname) override;

  virtual void rename(const std::string &src,
                      const std::string &target) override;

  virtual std::unique_ptr<IndexInput>
  openInput(const std::string &fname) override;

  virtual std::unique_ptr<LockFile>
  obtainLock(const std::string &fname) override;

  virtual bool fileExists(const std::string &fname) override;

  virtual void deleteSegment(const std::string &segment) noexcept override;
};

} // namespace lucanthrope
