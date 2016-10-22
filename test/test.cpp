#include "generated.h"
#include <cassert>

#define ASSERT_EQ(stm, value) assert(stm == value)

int main() {
    ASSERT_EQ(myNS::fileList().size(), 1);
    
    for (auto file : myNS::fileList()) {
        // check file name
        ASSERT_EQ(file.name(), "files/golden_master.bin");
        // check file data
        ASSERT_EQ(file.content().length(), 256);
        for (size_t i = 0; i < 256; ++i) {
            const size_t c = static_cast<unsigned char>(file.content().at(i));
            ASSERT_EQ(c, i);
        }
    }
}
