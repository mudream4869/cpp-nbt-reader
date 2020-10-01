# nbt-reader

Inspired by [cpp-nbt](https://github.com/SpockBotMC/cpp-nbt).

Leave only reader function and remove `_FixString`.

## Usage

Here is a hello-world example.
the program will read the `hello_world.nbt` and
output the name of the root node.

```c++
#include <fstream>
#include <iostream>

#include "nbt.h"

int main() {
    std::ifstream buf("hello_world.nbt");
    auto doc = nbt::readDocument(buf);
    std::cout << doc->getName() << "\n";
}
```

For doing node travesal, please see [`example.cpp`](example.cpp)

