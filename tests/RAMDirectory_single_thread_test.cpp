#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "lucanthrope/IO/IndexInput.h"
#include "lucanthrope/IO/IndexOutput.h"
#include "lucanthrope/storage/Directory.h"
#include "lucanthrope/storage/RAMDirectory.h"

int main() {
  // was tested with RAMFile::kBlockSize = 1, 2, 4096
  using namespace lucanthrope;
  try {
    std::unique_ptr<Directory> dir(new RAMDirectory());
    std::string return_statement =
        "The expr-or-braced-init-list of a return statement is called its "
        "operand. A return statement with no operand shall be used only in a "
        "function whose return type is cv void, a constructor, or a "
        "destructor. A return statement with an operand of type void shall be "
        "used only in a function that has a cv void return type. A return "
        "statement with any other operand shall be used only in a function "
        "that has a return type other than cv void; the return statement "
        "initializes the returned reference or prvalue result object of the "
        "(explicit or implicit) function call by copy-initialization from the "
        "operand.";
    uint32_t max_uint32_t = std::numeric_limits<uint32_t>::max();
    uint64_t max_uint64_t = std::numeric_limits<uint64_t>::max();
    uint32_t varint32 = 1928936378;
    uint64_t varint64 = 565675526378912;
    {
      std::unique_ptr<IndexOutput> file = dir->createOutput("test_file");
      file->writeInt64(0); // skip 8 bytes;
      file->writeString(return_statement)
          .writeVarint64(varint64)
          .writeInt32(max_uint32_t)
          .writeVarint32(varint32);
      file->seek(0);
      file->writeInt64(max_uint64_t);
      std::vector<std::string> files = dir->listAll();
      assert(files.size() == 1 && files.back() == "test_file");
    }
    std::unique_ptr<IndexInput> input = dir->openInput("test_file");
    assert(input->readInt64() == max_uint64_t);
    std::string buf;
    input->readString(buf);
    assert(buf == return_statement);
    assert(input->readVarint64() == varint64);
    assert(input->readInt32() == max_uint32_t);
    assert(input->readVarint32() == varint32);
    input->seek(8);
    input->readString(buf);
    assert(buf == return_statement);
  } catch (std::exception &e) {
    std::cout << e.what() << '\n';
  }
  return 0;
}