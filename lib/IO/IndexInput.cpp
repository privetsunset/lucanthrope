#include <algorithm> // min()
#include <cstring>   // memcpy()
#include <cstdint>

#include "IO/IndexIOBase.h"
#include "IO/IndexInput.h"
#include "common/Exception.h"

namespace lucanthrope {

char IndexInput::readByte() {
  if (eof())
    throw Exception(
        Exception::Code::IndexCorruptionException,
        std::string_view(
            "in IndexInput::readByte(): cannot read a byte, EOF is reached"));
  pos++;
  return *bufCur++;
}

// Reads as many as there are, no more than n bytes.
size_t IndexInput::read(char *ptr, size_t size) {
  size_t bytes_copied = 0;
  while (bytes_copied < size && !eof()) {
    size_t bytes_to_copy = std::min(getNumReadableBytes(), size - bytes_copied);
    std::memcpy(ptr + bytes_copied, bufCur, bytes_to_copy);
    bufCur += bytes_to_copy;
    bytes_copied += bytes_to_copy;
  }
  pos += bytes_copied;
  return bytes_copied;
}

uint32_t IndexInput::readInt32() {
  char buf[sizeof(uint32_t)];
  auto n = read(buf, sizeof(uint32_t));
  if (n < sizeof(uint32_t))
    throw Exception(
        Exception::Code::IndexCorruptionException,
        std::string_view(
            "in IndexInput::readInt32(): cannot read an int, EOF is reached"));
  const uint8_t *const buffer = reinterpret_cast<const uint8_t *>(buf);
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

uint64_t IndexInput::readInt64() {
  char buf[sizeof(uint64_t)];
  auto n = read(buf, sizeof(uint64_t));
  if (n < sizeof(uint64_t))
    throw Exception(
        Exception::Code::IndexCorruptionException,
        std::string_view(
            "in IndexInput::readInt32(): cannot read an int, EOF is reached"));
  const uint8_t *const buffer = reinterpret_cast<const uint8_t *>(buf);
  return (static_cast<uint64_t>(buffer[0])) |
         (static_cast<uint64_t>(buffer[1]) << 8) |
         (static_cast<uint64_t>(buffer[2]) << 16) |
         (static_cast<uint64_t>(buffer[3]) << 24) |
         (static_cast<uint64_t>(buffer[4]) << 32) |
         (static_cast<uint64_t>(buffer[5]) << 40) |
         (static_cast<uint64_t>(buffer[6]) << 48) |
         (static_cast<uint64_t>(buffer[7]) << 56);
}

uint64_t IndexInput::readVarint64() {
  uint64_t ret = 0;
  size_t bytes_decoded = 0;
  bool done = false;
  while (bytes_decoded < IndexIOBase::kMaxVarintLength64 && !eof()) {
    uint8_t byte = readByte();
    ret |= ((static_cast<uint64_t>(byte) & 0x7f) << (7 * bytes_decoded));
    bytes_decoded++;
    if (!(byte & 128)) {
      done = true;
      break;
    }
  }
  if (!done)
    throw Exception(
        Exception::Code::IndexCorruptionException,
        std::string_view("in IndexInput::readVarint64(): cannot parse varint"));
  return ret;
}

// FIXME: this and readVarint64() are almost identical, refactor it
uint32_t IndexInput::readVarint32() {
  uint32_t ret = 0;
  size_t bytes_decoded = 0;
  bool done = false;
  while (bytes_decoded < IndexIOBase::kMaxVarintLength32 && !eof()) {
    uint8_t byte = readByte();
    ret |= ((static_cast<uint32_t>(byte) & 0x7f) << (7 * bytes_decoded));
    bytes_decoded++;
    if (!(byte & 128)) {
      done = true;
      break;
    }
  }
  if (!done)
    throw Exception(
        Exception::Code::IndexCorruptionException,
        std::string_view("in IndexInput::readVarint32(): cannot parse varint"));
  return ret;
}

} // namespace lucanthrope