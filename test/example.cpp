#include "output/generated.h"
#include <iostream>

int main() {
	for (auto file : myNamespace::fileList()) {
		std::cout << "Name: " << file.name() << "\n";
		std::cout << "Size: " << file.fileDataSize << "\n";
		std::cout << "Data: " << file.content() << "\n";
	}
}
