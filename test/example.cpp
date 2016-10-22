#include "output/generated.h"
#include <iostream>

int main() {
	for (auto file : myNamespace::fileList()) {
		std::cout << "Name: " << file.name() << "\n";
		// content() returns a "const std::string &" to a static object
		// that is created once only if requested 
		std::cout << "Size: " << file.content().size() << "\n";
		std::cout << "Data: " << file.content() << "\n";
	}
}
