#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

int main() {
	std::ifstream in{"to_string.txt"};
	assert(in.is_open());
	std::string line;
	while (std::getline(in, line)) {
		std::cout << "\"";
		for (auto& c : line) {
			if (c == '"') {
				std::cout << "\\\"";
			}
			else {
				std::cout << c;
			}
		}
		std::cout << "\\n\"";
		std::cout << std::endl;
	}
	return 0;
}