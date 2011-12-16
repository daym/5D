#include <sstream>
#include <assert.h>
#ifdef WIN32
//typedef short uint16_t;
#else
#include <stdint.h>
#endif
#include "Evaluators/Evaluators"
#include "FFIs/Record"
#include "Evaluators/FFI"

namespace FFIs {

static size_t getSize(char c) {
	switch(c) {
	case 'b': return(sizeof(char));
	case 'h': return(sizeof(int16_t));
	case 'i': return(sizeof(int));
	case 'f': return(sizeof(float));
	case 'd': return(sizeof(double));
	case 'D': return(sizeof(long double));
	case 'l': return(sizeof(long));
	case 'B': return(sizeof(unsigned char));
	case 'H': return(sizeof(uint16_t));
	case 'I': return(sizeof(unsigned int));
	case 'L': return(sizeof(unsigned long));
	case 'p': return(sizeof(void*));
	case 'P': return(sizeof(void*));
	default: return(0); /* FIXME */
	}
}
static size_t getAlignment(char c) {
	switch(c) {
	case 'b': return(1);
	case 'h': return(2);
	case 'i': return(4);
	case 'f': return(4);
	case 'd': return(sizeof(long) == 8 ? 8 : 8); // FIXME latter: 4 for Linux
	case 'D': return(sizeof(long) == 8 ? 16 : 8); // FIXME latter: 4 for Linux
	case 'l': return(sizeof(long));
	case 'p': return(sizeof(long) == 8 ? 8 : 4);
	case 'P': return(sizeof(long) == 8 ? 8 : 4);
	case 'B': return(1);
	case 'H': return(2);
	case 'I': return(4);
	case 'L': return(sizeof(long));
	default: return(0); /* FIXME */
	}
}
// TODO sub-records, arrays, endianness.
AST::Str* Record_pack(AST::Str* formatStr, AST::Node* data) {
	size_t offset = 0;
	size_t padding = 0;
	size_t new_offset = 0;
	bool bBigEndian = false;
	std::stringstream sst;
	if(formatStr == NULL)
		return(NULL);
	size_t maxAlign = 0;
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	AST::Cons* consNode = Evaluators::evaluateToCons(data);
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		if(consNode == NULL)
			throw Evaluators::EvaluationException("Record_pack: not enough data for format.");
		AST::Node* headNode = Evaluators::reduce(consNode->head);
		consNode = Evaluators::evaluateToCons(consNode->tail);
		size_t size = getSize(*format);
		unsigned long value = (unsigned long) Evaluators::get_native_long(headNode); // FIXME the others
/*
int get_native_int(AST::Node* root);
long long get_native_long_long(AST::Node* root);
short get_native_short(AST::Node* root);
void* get_native_pointer(AST::Node* root);
bool get_native_boolean(AST::Node* root);
char* get_native_string(AST::Node* root);
float get_native_float(AST::Node* root);
long double get_native_long_double(AST::Node* root);
double get_native_double(AST::Node* root);

*/
		for(; size > 0; --size) {
			sst << (char) (value & 0xFF);
			value >>= 8;
		}
		offset += size;
		new_offset = (offset + maxAlign - 1) & ~(maxAlign - 1);
		for(; offset < new_offset; ++offset)
			sst << '\0';
	}
	return(new AST::Str(sst.str())); // NOT makeStr
}
AST::Node* Record_unpack(AST::Str* formatStr, AST::Str* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("Record_unpack needs format string.");
	const char* format = formatStr->text.c_str();
	const char* codedData = dataStr->text.c_str();
	size_t remainderLen = dataStr->text.length();
	size_t maxAlign = 0;
	size_t padding = 0;
	bool bBigEndian = false;
	size_t offset = 0;
	size_t new_offset = 0;
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t size = getSize(*format);
		if(remainderLen < size)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format.");
		codedData += size; // FIXME the actual value.
		remainderLen -= size;
		offset += size;
		new_offset = (offset + maxAlign - 1) & ~(maxAlign - 1);
		if(remainderLen < new_offset - offset)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format padding.");
		offset = new_offset;
		codedData += padding;
		remainderLen -= padding;
	}
	return(NULL); // FIXME
}
using namespace Evaluators;
static AST::Node* pack(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	return(Record_pack(dynamic_cast<AST::Str*>(a), b));
}
DEFINE_BINARY_OPERATION(RecordPacker, pack)
REGISTER_BUILTIN(RecordPacker, 2, 0, AST::symbolFromStr("packRecord"))
static AST::Node* unpack(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	return(Record_unpack(dynamic_cast<AST::Str*>(a), dynamic_cast<AST::Str*>(b)));
}
DEFINE_BINARY_OPERATION(RecordUnpacker, unpack)
REGISTER_BUILTIN(RecordUnpacker, 2, 0, AST::symbolFromStr("unpackRecord"))


}; /* end namespace FFIs */

