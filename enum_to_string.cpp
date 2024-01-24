#include <iostream>
#include <regex>
#include <fstream>

using namespace std::literals;

int main(int argc, char** argv) {
	std::ifstream in{ argv[1] };
	if (false == in.is_open()) {
		throw std::runtime_error("failed to open file"s + argv[1]);
	}
	std::regex identifier{ "[a-zA-Z_][a-zA-Z_0-9]+" };
	std::string line;
	while (std::getline(in, line)) {
		std::sregex_iterator ite{ std::begin(line), std::end(line), identifier };
		auto name = ite->str();
		auto str = "\""s + name + "\""s;
		std::cout << "case " << name << ": return " << str << ";" << std::endl;
	}
}