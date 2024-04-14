// PRIVATE HEADER
#pragma once

#include <cassert>
#include <cstddef> // size_t
#include <cstdint>
#include <vcruntime.h>

#include "IO/IndexOutput.h"
#include "storage/RAMDirectory.h"

namespace lucanthrope {

class RAMFileIndexOutput : public IndexOutput {
private:
  RAMDirectory::RAMFile *file;
  size_t current_block = 0;

public:
  RAMFileIndexOutput(RAMDirectory *d, const std::string &fname)
      : file(new RAMDirectory::RAMFile(d, fname)) {}
  RAMFileIndexOutput(const RAMFileIndexOutput &) = delete;
  RAMFileIndexOutput &operator=(const RAMFileIndexOutput &) = delete;
  ~RAMFileIndexOutput() {
    if (file->length < pos) // "flush"
      file->length = pos;
    file->finish_writing();
  }

private:
  virtual void initInternalBuffer() override {
    file->alloc();
    set(file->blocks_[0], RAMDirectory::RAMFile::kBlockSize);
    bufCur = bufStart;
  }

  // WARNING: if flush() is called by user with a fully written block, then an
  // empty block will be allocated. If no subsequent writes will write to that
  // last block, than it will remain empty in commited file. Except for wasted
  // space, this is fine and doesn't break anything. The same situation can also
  // occur with seek(). Write functions will never lead to this state though.
  virtual void writeImpl() override {
    if (file->length < pos)
      file->length = pos;
    if (bufCur == bufEnd) { // block is full, advance to the next block
      if (current_block == file->blocks_.size() - 1) {
        file->alloc();
      }
      current_block++;
      set(file->blocks_[current_block], RAMDirectory::RAMFile::kBlockSize);
    } else
      bufStart = bufCur; // see IndexOutput::flushNonEmpty()
  }

  virtual void seek(uint64_t seek_pos) override {
    if (file->length < pos)
      file->length = pos;
    assert(seek_pos <= file->length &&
           "Seeking past one-past-the-end of file is not supported!");
    current_block = seek_pos / RAMDirectory::RAMFile::kBlockSize;
    // Corner case: seek to one-past-the-end of the last block (entirely
    // filled); act as writeImpl() in this case
    if (current_block == file->blocks_.size()) {
      file->alloc();
    }
    size_t offset_in_current_block =
        seek_pos % RAMDirectory::RAMFile::kBlockSize;
    set(file->blocks_[current_block], RAMDirectory::RAMFile::kBlockSize);
    bufCur = bufStart + offset_in_current_block;
    pos = seek_pos;
  }

  virtual size_t preferredBufferSize() const override {
    return RAMDirectory::RAMFile::kBlockSize;
  }
};

} // namespace lucanthrope