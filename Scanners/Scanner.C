#include <iostream>
#include <string>
#include "Scanners/Scanner"

namespace Scanners {

void Scanner::parse_token(void) {
}

void Scanner::push(FILE* input_file) {
	this->input_file = input_file;
	this->position = ftell(input_file);
}

void Scanner::pop(void) {
}

void Scanner::raise_error(const std::string& expected_text, std::string got_text) {
	std::cerr << "error: expected " << expected_text << " near position " << position << std::endl;
}

void Scanner::raise_error(const std::string& expected_text, int got_text) {
	std::string s;
	s = (char) got_text;
	raise_error(expected_text, s);
}

};
