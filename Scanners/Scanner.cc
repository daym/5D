/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
	s << "expected " << expected_text << " but got " << got_text << " near position " << position << " in line " << line_number + 1;
	//std::cerr << s.str() << std::endl;
	throw ParseException(s.str().c_str());
}

void Scanner::raise_error(const std::string& expected_text, int got_text) {
	std::string s;
	if(got_text > 127)
		s = "<junk>";
	else
		s = (char) got_text;
	raise_error(expected_text, s);
}

};
