#include "lucanthrope/document/Document.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

int main() {
  using namespace lucanthrope;
  Document doc;
  doc.add(Field::text("some field 1", "some string value"));
  std::istream *stream = new std::istringstream("some istream value");
  doc.add(Field::text("some field 2", std::unique_ptr<std::istream>(stream)));
  std::cout << doc.toString() << '\n';
  auto it = doc.find("some field 1");
  assert(it != doc.end());
  std::cout << "string valued field: " << it->getStringValue() << '\n';
  it = doc.find("some field 2");
  assert(it != doc.end());
  char buf[100];
  it->getIStreamValue().read(buf, 100);
  std::cout << "istream value field: "
            << std::string(buf, it->getIStreamValue().gcount()) << '\n';
  return 0;
}