#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdint.h>

namespace lucanthrope {

// Simple abstract base class for IndexOutput and IndexInput, which contains
// common parts for both. Every read/write is buffered, and the buffer is either
// external or internal. However, some streams do not support external buffers
// (RAMFileIndexOutput/RAMFileIndexInput). Initially, IndexOutput and IndexInput
// are created without a buffer, and if external buffer is not set before the
// first read/write, then internal buffer is implicitly allocated on first
// read/write. After the buffer is set, it cannot be replaced with another
// buffer through the public API. It would be nice to have such functionality,
// but the problem is, while for IndexOutput it has a clear semantics of
// flushing the old buffer, for IndexInput it isn't obvious what to do with the
// old buffer (do we just ignore unread data? or do we copy unread data into new
// buffer? if so, what if the new buffer is smaller than a number of unread
// bytes? etc.). This may be changed later, but for now trying to set an
// external buffer to the stream that already has a buffer is treated as a
// logical error and will crash the program via failed assertion. Trying to set
// an external buffer when it is not supported behaves in the same way. Again,
// that applies only to the public API, implementations are allowed to do any
// manipulations internally if it would provide efficient I/O.
class IndexIOBase {
protected:
  char *bufStart = nullptr;
  char *bufCur = nullptr; // current read/write position
  char *bufEnd = nullptr; // 1 byte after the end of the buffer

  // current position in the stream
  uint64_t pos = 0;

  // Set buffer (this is for internal use)
  void set(char *bufferStart, size_t size) {
    bufStart = bufferStart;
    bufEnd = bufStart + size;
    assert(bufStart <= bufEnd && "Invalid size!");
  }

  // All implementation-specific code that creates an internal buffer.
  virtual void initInternalBuffer() = 0;

public:
  IndexIOBase() = default;
  IndexIOBase(const IndexIOBase &) = delete;
  IndexIOBase &operator=(const IndexIOBase &) = delete;
  virtual ~IndexIOBase() = default;

  // Maximum length (in bytes) of the varint encoding of a 32-bit value.
  static constexpr size_t kMaxVarintLength32 = 5;

  // Maximum length (in bytes) of the varint encoding of a 64-bit value.
  static constexpr size_t kMaxVarintLength64 = 10;

  // Whether or not an external buffer may be installed. For example, a client
  // might want to provide external buffer if a set of files are to be written
  // one after another, in which case it could be more efficient to reuse a
  // single external buffer for every stream. However, RAMFileIndexOutput and
  // RAMFileIndexInput forbid to set an external buffer, because this
  // would be less efficient for them in ANY context. There may be more
  // IndexOutput/IndexInput implementations in the future that would dissalow
  // external buffers. Anyway, supportsExternalBuffer() allows a client to check
  // this beforehand.
  virtual bool supportsExternalBuffer() const { return false; }

  // If hasBuffer() == true, then it is not allowed to call
  // setExternalBuffer()
  bool hasBuffer() const { return bufStart != nullptr; }

  size_t getBufferSize() const { return bufEnd - bufStart; }

  uint64_t getCurrentPosition() const { return pos; }

  // Set external buffer. bufCur will be set to bufStart for IndexOutput, and to
  // bufEnd for IndexInput.
  void setExternalBuffer(char *bufferStart, size_t size) {
    assert(supportsExternalBuffer() &&
           "External buffers are not supported for this stream!");
    assert(!hasBuffer() && "Stream already has a buffer!");
    bufStart = bufferStart;
    bufEnd = bufStart + size;
    assert(bufStart <= bufEnd && "Invalid size!");
  }

  // Communicate a desired capacity for internal buffer.
  // Implementations in derived classes may ignore it.
  // No-op by default;
  virtual void hintBufferSize([[maybe_unused]] size_t hint) {}

  // Seek to the specified position
  virtual void seek(uint64_t seek_pos) = 0;

  // Return an efficient buffer size for the underlying output mechanism.
  virtual size_t preferredBufferSize() const {
#ifdef _WIN32
    // On Windows BUFSIZ is only 512 which results in more calls to write. This
    // overhead can cause significant performance degradation. Therefore use a
    // better default.
    return (16 * 1024);
#else
    // BUFSIZ is intended to be a reasonable default.
    return BUFSIZ;
#endif
  }
};

} // namespace lucanthrope