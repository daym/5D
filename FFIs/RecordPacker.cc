#include <stdio.h>
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
	case 'q': return(sizeof(long long));
	case 'Q': return(sizeof(unsigned long long));
	case 'p': return(sizeof(void*));
	case 'P': return(sizeof(void*));
	default: return(0); /* FIXME */
	}
}
// TODO do 64 bit systems align Q differently?
static size_t getAlignment(char c) {
	switch(c) {
	case 'b': return(1);
	case 'h': return(2);
	case 'i': return(4);
	case 'f': return(4);
	case 'd': return(sizeof(long) == 8 ? 8 : 
#ifdef WIN32
8
#else
4
#endif
);
	case 'D': return(sizeof(long) == 8 ? 16 : 
#ifdef WIN32
8
#else
4
#endif
); 
	case 'l': return(sizeof(long));
	case 'p': return(sizeof(long) == 8 ? 8 : 4);
	case 'P': return(sizeof(long) == 8 ? 8 : 4);
	case 'B': return(1);
	case 'H': return(2);
	case 'I': return(4);
	case 'L': return(sizeof(long));
	case 'q': return(sizeof(long) == 8 ? 8 : 
#ifdef WIN32
8
#else
4
#endif
);
	case 'Q': return(sizeof(long) == 8 ? 8 : 
#ifdef WIN32
8
#else
4
#endif
);
	default: return(0); /* FIXME */
	}
}
// TODO sub-records, arrays, endianness. pointers to other stuff.
// at least: execv "ls" ["ls"]  with record packer "[p]"

#define PACK_BUF(buffer) \
	assert(sizeof(buffer) == size); \
	char* b = (char*) &buffer; \
	for(; size > 0; --size, ++b) \
		sst << *b;

static inline size_t pack_atom_value(char formatC, AST::Node* headNode, std::stringstream& sst) {
	size_t size = getSize(formatC); 
	uint64_t mask = ~0; 
	uint64_t limit;
	long value = 0;
	bool B_out_of_range = false;
	limit = (1 << (8 * size - 1));
	mask = limit - 1;
	if(formatC == 'b' || formatC == 'h' || formatC == 'i' || formatC == 'l' || formatC == 'q') {
		value = (long) Evaluators::get_native_long(headNode); // FIXME bigger values
		mask = ~mask;
		if((value > 0 && (value & mask) != 0) || (value < 0 && ((-value-1) & mask) != 0))
			B_out_of_range = true;
	} else if(formatC == 'B' || formatC == 'H' || formatC == 'I' || formatC == 'L' || formatC == 'Q') {
		value = (long) Evaluators::get_native_long(headNode); // FIXME bigger values
		mask = (mask << 1) + 1;
		mask = ~mask;
		if((value > 0 && (value & mask) != 0) || (value < 0))
			B_out_of_range = true;
	} else { /* non-integral */
		switch(formatC) {
		case 'f': {
			float result = Evaluators::get_native_float(headNode);
			PACK_BUF(result)
			return(size);
		}
		case 'd': {
			double result = Evaluators::get_native_double(headNode);
			PACK_BUF(result)
			return(size);
		}
		case 'D': {
			long double result = Evaluators::get_native_long_double(headNode);
			PACK_BUF(result)
			return(size);
		}
		case 'p': {
			if(headNode == NULL)
				throw Evaluators::EvaluationException("packRecord: that field cannot be nil");
			void* result = Evaluators::get_native_pointer(headNode);
			PACK_BUF(result)
			return(size);
		}
		case 'P': {
			void* result = headNode ? Evaluators::get_native_pointer(headNode) : NULL;
			PACK_BUF(result)
			return(size);
		}
		default: {
			std::stringstream sst;
			sst << "unknown format \"" << formatC << "\"";
			std::string v = sst.str();
			throw Evaluators::EvaluationException(v.c_str());
		}
		}
	}
	if(B_out_of_range) {
		std::stringstream sst;
		sst << "value out of range for format \"" << formatC << "\"";
		std::string v = sst.str();
		throw Evaluators::EvaluationException(v.c_str());
	}
	for(size_t c; c > 0; --c) {
		sst << (char) (((unsigned char) value) & 0xFF);
		value >>= 8;
	}
	return(size);
}

AST::Str* Record_pack(AST::Str* formatStr, AST::Node* data) {
	size_t offset = 0;
	size_t new_offset = 0;
	bool bBigEndian = false;
	std::stringstream sst;
	if(formatStr == NULL)
		return(NULL);
	size_t position = 0; // in format
	AST::Cons* consNode = Evaluators::evaluateToCons(data);
	position = 0;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=')
			continue;
		if(consNode == NULL)
			throw Evaluators::EvaluationException("packRecord: not enough data for format.");
		AST::Node* headNode = Evaluators::reduce(consNode->head);
		consNode = Evaluators::evaluateToCons(consNode->tail);

		if(formatC == '[') {
		} else {
			size_t align = getAlignment(formatC); 
			size_t size = pack_atom_value(formatC, headNode, sst);
			offset += size;
			new_offset = (offset + align - 1) & ~(align - 1);
			for(; offset < new_offset; ++offset)
				sst << '\0';
/*
long long get_native_long_long(AST::Node* root);
void* get_native_pointer(AST::Node* root);
bool get_native_boolean(AST::Node* root);
char* get_native_string(AST::Node* root);
float get_native_float(AST::Node* root);
long double get_native_long_double(AST::Node* root);
double get_native_double(AST::Node* root);
*/
		}
	}
	std::string v = sst.str();
	return(AST::makeStrCXX(v));
}
#define DECODE_BUF(destination) \
	unsigned char* d = (unsigned char*) &destination; \
	for(; size > 0; --size, ++codedData, ++d) \
		*d = *codedData;

static inline AST::Node* decode(char formatC, const unsigned char* codedData, size_t size) {
	switch(formatC) {
	case 'b':
		{
			char value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'h':
		{
			short value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'i':
		{
			int value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'l':
		{
			long value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'B':
		{
			unsigned char value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'H':
		{
			unsigned short value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'I':
		{
			unsigned int value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'L':
		{
			unsigned long value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'q':
		{
			long long value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value)); // FIXME larger
		}
	case 'Q':
		{
			unsigned long long value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeInt) value)); // FIXME larger
		}
	case 'f':
		{
			float value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeFloat) value));
		}
	case 'd':
		{
			double value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeFloat) value)); // FIXME larger
		}
	case 'D':
		{
			long double value;
			DECODE_BUF(value)
			return(Numbers::internNative((Numbers::NativeFloat) value)); // FIXME larger
		}
	case 'p':
		{
			void* value;
			DECODE_BUF(value)
			if(value == NULL)
				throw Evaluators::EvaluationException("unpack: cannot decode NULL pointer (maybe use \"P\" ?)");
			return(AST::makeBox(value)); // this could also be made to reuse existing wrappers.
		}
	case 'P':
		{
			void* value;
			DECODE_BUF(value)
			if(value == NULL)
				return(NULL);
			else
				return(AST::makeBox(value));
		}
	default:
		{
			std::stringstream sst;
			sst << "unpack: unknown format \"" << formatC << "\"";
			std::string v = sst.str();
			throw Evaluators::EvaluationException(v.c_str());
		}
	}
}
// TODO record packer "[p]"
// TODO float etc.
AST::Node* Record_unpack(AST::Str* formatStr, AST::Box* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("unpackRecord needs format string.");
	const unsigned char* codedData = (const unsigned char*) dataStr->native;
	size_t remainderLen = dynamic_cast<AST::Str*>(dataStr) ? ((AST::Str*) dataStr)->size : 9999999; // FIXME
	bool bBigEndian = false;
	size_t offset = 0;
	size_t new_offset = 0;
	size_t position = 0; // in format
	AST::Cons* result = NULL;
	AST::Cons* tail = NULL;
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=')
			continue;
		size_t size = getSize(*format);
		size_t align = getAlignment(*format);
		if(remainderLen < size)
			throw Evaluators::EvaluationException("unpackRecord: not enough coded data for format.");
		AST::Cons* n = AST::makeCons(decode(formatC, codedData, size), NULL);
		if(!tail)
			result = n;
		else
			tail->tail = n;
		tail = n;
		codedData += size;
		remainderLen -= size;
		offset += size;
		new_offset = (offset + align - 1) & ~(align - 1);
		if(remainderLen < new_offset - offset)
			throw Evaluators::EvaluationException("unpackRecord: not enough coded data for format padding.");
		offset = new_offset;
	}
	return(result);
}
// this doesn't work with record packers like "[p]" which have dynamic length.
size_t Record_get_size(AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("recordSize needs format string.");
	const char* format = (const char*) formatStr->native;
	size_t offset = 0;
	size_t new_offset = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
			continue;
		size_t size = getSize(*format);
		size_t align = getAlignment(*format);
		offset += size;
		new_offset = (offset + align - 1) & ~(align - 1);
		offset = new_offset;
	}
	return(offset);
}
AST::Str* Record_allocate(size_t size) {
	if(size < 1) // TODO
		throw Evaluators::EvaluationException("cannot allocate record with unknown size");
	char* buffer = new char[size];
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
static AST::Node* substr(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Str* mBox = dynamic_cast<AST::Str*>(iter->second);
	if(iter->second == NULL)
		return(NULL);
	++iter;
	if(!mBox)
		throw Evaluators::EvaluationException("substr on non-string is undefined");
	int beginning = Evaluators::get_native_int(iter->second);
	++iter;
	int end = Evaluators::get_native_int(iter->second);
	++iter;
	char* p = (char*) mBox->native;
	size_t sz = mBox->size;
	if(beginning < 0)
		beginning = sz + beginning;
	if(end < 0)
		end = sz + end;
	if(beginning < 0)
		beginning = 0;
	if(end > sz)
		end = sz;
	int len = (end > beginning) ? end - beginning : 0;
	if(len == 0)
		return(NULL);
	p += beginning;
	return(AST::makeStrRaw(p, len)); // TODO maybe copy.
}
static AST::Str* str_until_zero(AST::Str* value) {
	AST::Str* result;
	if(value == NULL)
		return(NULL);
	result = AST::makeStrRaw((char*) value->native, strlen((char*) value->native)); // TODO copy?
	return(result);
}

DEFINE_SIMPLE_OPERATION(ListFromStrGetter, (argument = reduce(argument), str_P(argument) ? listFromStr(dynamic_cast<AST::Str*>(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrFromListGetter, (argument = reduce(argument), (cons_P(argument) || nil_P(argument)) ? strFromList(dynamic_cast<AST::Cons*>(argument)) : FALLBACK))
DEFINE_FULL_OPERATION(SubstrGetter, return(substr(fn, argument));)
DEFINE_SIMPLE_OPERATION(StrUntilZeroGetter, str_until_zero(dynamic_cast<AST::Str*>(reduce(argument))))
REGISTER_BUILTIN(ListFromStrGetter, 1, 0, AST::symbolFromStr("listFromStr"))
REGISTER_BUILTIN(StrFromListGetter, 1, 0, AST::symbolFromStr("strFromList"))
REGISTER_BUILTIN(SubstrGetter, 3, 0, AST::symbolFromStr("substr"))
REGISTER_BUILTIN(StrUntilZeroGetter, 1, 0, AST::symbolFromStr("strUntilZero"))

}; /* end namespace FFIs */
