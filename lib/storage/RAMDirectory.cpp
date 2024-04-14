#include <iostream>
#include <mutex>
#include <string_view>

#include "IO/IndexInput.h"
#include "IO/IndexOutput.h"
#include "IO/RAMFileIndexInput.h"  // private header
#include "IO/RAMFileIndexOutput.h" // private header
#include "common/Exception.h"
#include "storage/RAMDirectory.h"
#include "storage/RAMDirectoryLockFile.h" // private header

namespace lucanthrope {

const RAMDirectory::RAMFile RAMDirectory::dummy_file;

RAMDirectory::~RAMDirectory() {
  std::lock_guard<std::mutex> guard(mu_);
  for (auto &p : files) {
    if (p.second != &dummy_file) {
      RAMFile *file = p.second;
      assert(file->refs_ == 1 &&
             "Attempt to deallocate a file that is open for reading!");
      file->refs_--; // have to do that so that assertion in RAMFile's
                     // destructor is passed
      delete p.second;
    }
  }
}

// When commit() is called, it is guaranteed that a pair (fname, &dummy_file) is
// already present in files map, so we will only replase &dummy_file with file,
// hence no reallocation of files map's storage would occur. That is, commit()
// never throws.
void RAMDirectory::commit(const std::string &fname, RAMFile *file) noexcept {
  std::lock_guard<std::mutex> guard(mu_);
  assert(files[fname] == &dummy_file &&
         "File name is not registered, RAMDirectory's invariants do not hold!");
  file->refs_ = 1;
  files[fname] = file;
}

void RAMDirectory::unrefReader(const RAMFile *file) noexcept {
  // mutex is toggled manually so that a file's deallocation doesn't block
  mu_.lock();
  int c = --file->refs_;
  mu_.unlock();
  if (!c) {
    delete file;
  }
}

void RAMDirectory::releaseLock(const std::string &fname) noexcept {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it == files.end()) {
    // Theoretically, it is possible that an exception is thrown by
    // obtainLock() after RAMDirecoryLockFile object was successfully allocated,
    // which means that its name is not in file's map. That is extremely
    // unlikely, but we have to handle this without assert(it != files.end()).
    // Just write a warning to std::cerr and return.
    std::cerr << "WARNING: lock file is not found in the directory, directory "
                 "may be corrupted!\n";
    return;
  }
  assert(it->second == &dummy_file && "Lock file refers to unknown RAMFile, "
                                      "RAMDirectory's invariants do not hold!");
  files.erase(it);
}

std::vector<std::string> RAMDirectory::listAll() {
  std::vector<std::string> ret;
  std::lock_guard<std::mutex> guard(mu_);
  for (auto &f : files) {
    ret.push_back(f.first);
  }
  return ret;
}

void RAMDirectory::deleteFile(const std::string &fname) {
  // mutex is toggled manually so that a file's deallocation doesn't block
  mu_.lock();
  auto it = files.find(fname);
  RAMFile *file = nullptr;
  int ref_count;
  if (it != files.end()) {
    file = it->second;
    assert(file != &dummy_file &&
           "Attempt to delete an uncommited file; a file may be deleted only "
           "after it were commited");
    files.erase(it); // won't throw
    ref_count = --file->refs_;
  }
  mu_.unlock();
  if (!file)
    throw Exception(Exception::Code::FileNotFoundException,
                    std::string("In RAMDirectory::deleteFile(): File named ")
                        .append(fname)
                        .append(" is not found in RAMDirectory"));
  if (!ref_count) {
    delete file;
  }
}

uint64_t RAMDirectory::fileLength(const std::string &fname) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it == files.end())
    throw Exception(Exception::Code::FileNotFoundException,
                    std::string("In RAMDirectory::fileLength(): File named ")
                        .append(fname)
                        .append(" is not found in RAMDirectory"));
  return it->second->length;
}

std::unique_ptr<IndexOutput>
RAMDirectory::createOutput(const std::string &fname) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it != files.end())
    throw Exception(Exception::Code::FileAlreadyExistsException,
                    std::string("In RAMDirectory::createOutput(): File named ")
                        .append(fname)
                        .append(" already exists in RAMDirectory"));
  // 'files[fname] = const_cast<RAMFile *>(&dummy_file)' can throw
  // (theoretically); protect against that with unique_ptr
  std::unique_ptr<IndexOutput> output(new RAMFileIndexOutput(this, fname));
  // Insert file's name into files map only after RAMFileIndexOutput was
  // successfully allocated.
  files[fname] = const_cast<RAMFile *>(&dummy_file);

  // As far as i understand C++17, using std::move() is absolutely necessary
  // here: unique_ptr doesn't have a copy constructor, which is required to
  // initialize caller's unique_ptr with 'output' lvalue; this check has to be
  // made even in the presence of copy elision. However, clang doesn't emit any
  // error for this, in fact, it allows to return lvalue that denotes an object
  // of class with only move constructor. For example:
  //
  // struct Uncopyable {
  //    Uncopyable() = default;
  //    Uncopyable(const Uncopyable&) = delete;
  //    Uncopyable(Uncopyable&&) = default;
  // };
  //
  // Uncopyable foo() {
  //    Uncopyable res;
  //    return res;
  // }
  //
  // int main() {
  //    Uncopyable x = foo();
  //    return 0;
  // }
  // clang and gcc compile this ^^^ even with -std=c++14 flag. I think that it
  // is a bug, so i'm using std::move() here.
  return std::move(output);
}

void RAMDirectory::rename(const std::string &src, const std::string &target) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it_src = files.find(src);
  if (it_src == files.end())
    throw Exception(Exception::Code::FileNotFoundException,
                    std::string("In RAMDirectory::rename(): File named ")
                        .append(src)
                        .append(" is not found in RAMDirectory"));
  auto it_target = files.find(target);
  if (it_target != files.end())
    throw Exception(Exception::Code::FileAlreadyExistsException,
                    std::string("In RAMDirectory::rename(): File named ")
                        .append(target)
                        .append(" already exists in RAMDirectory"));
  files[target] = it_src->second;
  files.erase(it_src);
}

std::unique_ptr<IndexInput> RAMDirectory::openInput(const std::string &fname) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it == files.end())
    throw Exception(Exception::Code::FileNotFoundException,
                    std::string("In RAMDirectory::openInput(): File named ")
                        .append(fname)
                        .append(" is not found in RAMDirectory"));
  RAMFile *file = it->second;
  assert(file != &dummy_file &&
         "Attempt to read an uncommited file; a file may be read only "
         "after it were commited");
  RAMFileIndexInput *input = new RAMFileIndexInput(file);
  // Increment reference count only after RAMFileIndexInput was successfully
  // allocated; otherwise, will introduce a leak (file's reference count will
  // never reach 0) if bad_alloc was successfully handled afterwards (it is
  // extremely unlikely, but let's try to be truly exception-safe here).
  file->refs_++;
  return std::unique_ptr<IndexInput>(input);
}

std::unique_ptr<LockFile> RAMDirectory::obtainLock(const std::string &fname) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it != files.end()) // lock is held by someone
    return std::unique_ptr<LockFile>();
  // 'files[fname] = const_cast<RAMFile *>(&dummy_file)' can throw
  // (theoretically); protect against that with unique_ptr
  std::unique_ptr<LockFile> lock(new RAMDirectoryLockFile(this, fname));

  // Insert lock file's name into files map only after RAMDirectoryLockFile was
  // successfully allocated; otherwise, it would be impossible to unlock
  // directory if bad_alloc was successfully handled afterwards (it is extremely
  // unlikely, but let's try to be truly exception-safe here).
  files[fname] = const_cast<RAMFile *>(&dummy_file);
  return std::move(lock);
}

bool RAMDirectory::fileExists(const std::string &fname) {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = files.find(fname);
  if (it != files.end())
    return true;
  return false;
}

void RAMDirectory::deleteSegment(const std::string &segment) noexcept {
  std::lock_guard<std::mutex> guard(mu_);
  size_t size = segment.size();
  for (auto &pair : files) {
    if (pair.first.size() >= size && segment == pair.first.substr(0, size)) {
      assert(pair.second == &dummy_file &&
             "Attempt to delete an uncommited file in exception handler; it is "
             "should not be possible with RAMDirectory's invariants, something "
             "is broken!");
      RAMFile *file = pair.second;
      files.erase(pair.first);
      if (--file->refs_ == 0)
        delete file;
    }
  }
}

} // namespace lucanthrope