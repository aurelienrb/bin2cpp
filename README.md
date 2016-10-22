# bin2cpp
Generates C++ source files to embed the content of other external files. The generated code is in C++98 to allow better reuse.

## Features
 - can wrap the generated code into a given namespace
 - can iterate (recursively) over all the files in a given folder to embed them all at once
 - names of the embedded files comes along their data to allow querying by file name

## License
 - This is free and unencumbered software released into the **public domain**.
 - Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.
 - For more information, please refer to [http://unlicense.org/]()
 
## Example

```bin2cpp -ns embedded -o myfiles mydirectory/```

```cpp
#pragma once

namespace embedded {
    struct FileInfo {
        const char * file_name;
        const char file_data;
        const size_t file_data_length;
    };

    extern const unsigned int fileListSize;
    extern const FileInfo fileList[];
}
```

```cpp
#include "myfiles.h"

namespace /* anonymous */ {
    const char * file0_name = "test.txt";
    const unsigned int file0_data_size = 9;
    const char file0_data[file0_data_size] = {
        0x4f,0x4b,0x31,0xd,0xa,
        0x4f,0x4b,0x32,0x21,
    };
}

namespace embedded {
    const unsigned int fileListSize = 1;
    const FileInfo fileList[] = {
            { file0_name, file0_data, file0_data_size },
    };
}```

## Supported compilers
 - Visual C++ 2015
