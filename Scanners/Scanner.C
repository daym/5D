#include <iostream>
#include <string.h>
#include <string>
#include "Scanners/Scanner"

namespace Scanners {

ParseException::ParseException(const char* s) throw() {
	message = strdup(s);
}
const char* ParseException::what() const throw() {
	return message; //message.c_str();
};

void Scanner::parse_token(void) {
}

void Scanner::push(FILE* input_file, int line_number) {
	this->input_file = input_file;
	this->line_number = line_number;
	this->position = ftell(input_file);
}

void Scanner::pop(void) {
}

void Scanner::raise_error(const std::string& expected_text, std::string got_text) {
	std::stringstream s;
	s << "error: expected " << expected_text << " near position " << position << " in line " << line_number + 1 << std::endl;
	std::cerr << s.str() << std::endl;
	throw ParseException(s.str().c_str());
}

void Scanner::raise_error(const std::string& expected_text, int got_text) {
	std::string s;
	s = (char) got_text;
	raise_error(expected_text, s);
}

};
