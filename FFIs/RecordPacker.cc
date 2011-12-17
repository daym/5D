#include <sstream>
#include <assert.h>
#ifdef WIN32
//typedef short uint16_t;
#else
#include <stdint.h>
#endif
#include <string.h>
#include "Evaluators/Evaluators"
#include "FFIs/RecordPacker"
#include "Evaluators/FFI"
#include "Numbers/Integer"
#include "Evaluators/Builtins"

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
// TODO sub-records, arrays, endianness. pointers to other stuff.
AST::Str* Record_pack(AST::Str* formatStr, AST::Node* data) {
	size_t offset = 0;
	size_t new_offset = 0;
	bool bBigEndian = false;
	std::stringstream sst;
	if(formatStr == NULL)
		return(NULL);
	size_t maxAlign = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	AST::Cons* consNode = Evaluators::evaluateToCons(data);
	position = 0;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
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
	std::string v = sst.str();
	return(AST::makeStrRaw(strdup(v.c_str()), v.length()));
}
static inline AST::Node* decode(char format, const unsigned char* codedData, size_t size) {
	Numbers::NativeInt value = 0; // TODO bigger ones
	int offset = 0;
	for(; size > 0; --size, offset += 8, ++codedData) {
		value |= (*codedData) << offset;
	}
	return(Numbers::internNative(value));
}
AST::Node* Record_unpack(AST::Str* formatStr, AST::Box* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("Record_unpack needs format string.");
	const unsigned char* codedData = (const unsigned char*) dataStr->native;
	size_t remainderLen = dynamic_cast<AST::Str*>(dataStr) ? ((AST::Str*) dataStr)->size : 9999999; // FIXME
	size_t maxAlign = 0;
	size_t padding = 0;
	bool bBigEndian = false;
	size_t offset = 0;
	size_t new_offset = 0;
	size_t position = 0; // in format
	AST::Cons* result = NULL;
	AST::Cons* tail = NULL;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	position = 0;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t size = getSize(*format);
		if(remainderLen < size)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format.");
		char formatC = *format;
		AST::Cons* n = AST::makeCons(decode(formatC, codedData, size), NULL);
		if(!tail)
			result = n;
		else
			tail->tail = n;
		tail = n;
		codedData += size;
		remainderLen -= size;
		offset += size;
		new_offset = (offset + maxAlign - 1) & ~(maxAlign - 1);
		if(remainderLen < new_offset - offset)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format padding.");
		offset = new_offset;
		codedData += padding;
		remainderLen -= padding;
	}
	return(result);
}
size_t Record_get_size(AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("Record_size needs format string.");
	const char* format = (const char*) formatStr->native;
	size_t maxAlign = 0;
	size_t padding = 0;
	size_t offset = 0;
	size_t new_offset = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	position = 0;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t size = getSize(*format);
		offset += size;
		new_offset = (offset + maxAlign - 1) & ~(maxAlign - 1);
		offset = new_offset;
	}
	return(offset);
}
AST::Str* Record_allocate(size_t size) {
	char* buffer = (char*) calloc(1, size);
	if(size < 1) // TODO
		throw Evaluators::EvaluationException("cannot allocate record with unknown size");
	AST::Str* result = AST::makeStrRaw(buffer, size);
	return(result);
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
	// TODO size
	return(Record_unpack(dynamic_cast<AST::Str*>(a), dynamic_cast<AST::Box*>(b)));
}
DEFINE_BINARY_OPERATION(RecordUnpacker, unpack)
REGISTER_BUILTIN(RecordUnpacker, 2, 0, AST::symbolFromStr("unpackRecord"))

DEFINE_SIMPLE_OPERATION(RecordSizeCalculator, Numbers::internNative((Numbers::NativeInt) Record_get_size(dynamic_cast<AST::Str*>(reduce(argument)))))
REGISTER_BUILTIN(RecordSizeCalculator, 1, 0, AST::symbolFromStr("recordSize"))

static AST::Node* wrapAllocateRecord(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Str* format = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	AST::Node* result = Record_allocate(Record_get_size(format));
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(RecordAllocator, {
	return(wrapAllocateRecord(fn, argument));
})
REGISTER_BUILTIN(RecordAllocator, 2, 0, AST::symbolFromStr("allocateRecord"))
static AST::Node* wrapDuplicateRecord(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Str* record = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	AST::Str* result;
	if(record == NULL)
		result = NULL;
	else {
		result = Record_allocate(record->size);
		memcpy(result->native, record->native, record->size);
	}
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(RecordDuplicator, {
	return(wrapDuplicateRecord(fn, argument));
})
REGISTER_BUILTIN(RecordDuplicator, 2, 0, AST::symbolFromStr("duplicateRecord"))

AST::Node* listFromCharS(const char* text, size_t remainder) {
	if(remainder == 0)
		return(NULL);
	else
		return(makeCons(Numbers::internNative((Numbers::NativeInt) (unsigned char) *text), listFromCharS(text + 1, remainder - 1)));
}
AST::Node* listFromStr(AST::Str* node) {
	return(listFromCharS((const char*) node->native, node->size));
}
AST::Node* strFromList(AST::Cons* node) {
	std::stringstream sst;
	bool B_ok;
	for(; node; node = evaluateToCons(node->tail)) {
		int c = Numbers::toNativeInt(node->head, B_ok);
		if(c < 0 || c > 255) // oops
			throw Evaluators::EvaluationException("list cannot be represented as a str.");
		sst << (char) c;
	}
	std::string v = sst.str();
	AST::Str* result = Record_allocate(v.length());
	memcpy(result->native, v.c_str(), result->size);
	return(result);
}
DEFINE_SIMPLE_OPERATION(ListFromStrGetter, (argument = reduce(argument), str_P(argument) ? listFromStr(dynamic_cast<AST::Str*>(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrFromListGetter, (argument = reduce(argument), (cons_P(argument) || nil_P(argument)) ? strFromList(dynamic_cast<AST::Cons*>(argument)) : FALLBACK))
REGISTER_BUILTIN(ListFromStrGetter, 1, 0, AST::symbolFromStr("listFromStr"))
REGISTER_BUILTIN(StrFromListGetter, 1, 0, AST::symbolFromStr("strFromList"))

}; /* end namespace FFIs */
