// PRIVATE HEADER
#pragma once

#include <cassert>
#include <cstddef> // size_t

#include "IO/IndexInput.h"
#include "storage/RAMDirectory.h"

namespace lucanthrope {

class RAMFileIndexInput : public IndexInput {
private:
  RAMDirectory::RAMFile *file;
  size_t current_block = 0;
  size_t last_block = 0;       // last block index
  size_t last_block_bytes = 0; // number of bytes in the last block

public:
  RAMFileIndexInput(RAMDirectory::RAMFile *f) : file(f) {
    assert(file->length && "File is empty!");

    // file->blocks_.size() may be "lying" about the number of blocks which
    // actually have data (see comment in RAMFileIndexOutput.h), so last_block
    // index is computed in another way
    last_block = (file->length -1) / RAMDirectory::RAMFile::kBlockSize + 1;

    // if file->length is a multiple of RAMDirectory::RAMFile::kBlockSize, then
    // (file->length - 1) % RAMDirectory::RAMFile::kBlockSize + 1 ==
    // RAMDirectory::RAMFile::kBlockSize, otherwise, (file->length - 1) %
    // RAMDirectory::RAMFile::kBlockSize + 1 == file->length %
    // RAMDirectory::RAMFile::kBlockSize
    last_block_bytes =
        (file->length - 1) % RAMDirectory::RAMFile::kBlockSize + 1;
  }
  ~RAMFileIndexInput() { file->finish_reading(); }

  virtual void initInternalBuffer() override {
    set(file->blocks_[0], RAMDirectory::RAMFile::kBlockSize);
    bufCur = bufStart;
    if (!last_block)
      sentinel = bufCur + last_block_bytes;
    else
      sentinel = bufEnd;
  }

  virtual bool fillImpl() override {
    assert(!hasPendingData() && "Buffer is not empty!");
    if (!bufStart) {
      initInternalBuffer();
      return true;
    }
    if (current_block == last_block) // EOF
      return false;
    current_block++;
    set(file->blocks_[current_block], RAMDirectory::RAMFile::kBlockSize);
    bufCur = bufStart;
    if (current_block == last_block)
      sentinel = bufCur + last_block_bytes;
    else
      sentinel = bufEnd;
    return true;
  }

  virtual void seek(uint64_t seek_pos) override {
    assert(seek_pos < file->length &&
           "Seeking past the end of  file is not supported!");
    current_block = seek_pos / RAMDirectory::RAMFile::kBlockSize;
    size_t block_offset = seek_pos % RAMDirectory::RAMFile::kBlockSize;
    set(file->blocks_[current_block], RAMDirectory::RAMFile::kBlockSize);
    bufCur = bufStart + block_offset;
    if (current_block == last_block)
      sentinel = bufStart + last_block_bytes;
    else
      sentinel = bufEnd;
  }
};

} // namespace lucanthrope