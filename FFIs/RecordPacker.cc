#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <sstream>
#include <assert.h>
#ifndef WIN32
#include <endian.h>
#include <stdint.h>
#endif
#include <string.h>
#include "Evaluators/Evaluators"
#include "FFIs/RecordPacker"
#include "Evaluators/FFI"
#include "Numbers/Integer"
#include "Evaluators/Builtins"

namespace FFIs {

/* design is as follows:


struct packing can used in the following modes:

'=' means machine mode
'<' means little endian mode
'>' means big endian mode

In machine mode, the format character maps to alignment and the actual C datatype for that format, see #getSize.
In the other modes, the format character means to use old well-known sizes (and no alignment).

It is assumed that the user does the mapping of user-defined types to actual C intrinsic on hir own, it is NOT within the scope of the struct module to do so.

Note that 'V' is already taken by the FFI Trampolines (for VARIANT).
*/

// TODO add =, <, > standard size without alignment! (@ with alignment)

static size_t getSize(enum ByteOrder byteOrder, char c) {
	if(byteOrder == MACHINE_BYTE_ORDER_ALIGNED) {
		switch(c) {
		case 'b': return(sizeof(char));
		case 'h': return(sizeof(int16_t));
		case 'i': return(sizeof(int));
		case 'f': return(sizeof(float));
		case 'd': return(sizeof(double));
		case 'g': return(sizeof(long double));
		case 'l': return(sizeof(long));
		case 'B': return(sizeof(unsigned char));
		case 'H': return(sizeof(uint16_t));
		case 'I': return(sizeof(unsigned int));
		case 'L': return(sizeof(unsigned long));
		case 'q': return(sizeof(long long));
		case 'Q': return(sizeof(unsigned long long));
		case 'p': return(sizeof(void*));
		case 'P': return(sizeof(void*));
		case 'z': return(sizeof(char*));
		case 'Z': return(sizeof(char*));
		case 's': return(sizeof(char*));
		case 'S': return(sizeof(char*));
#ifdef WIN32
		case 'V': return(sizeof(int));
#endif
		default: return(0); /* FIXME */
		}
	} else { /* defaults */
		switch(c) {
		case 'b': return(1);
		case 'h': return(2);
		case 'i': return(4);
		case 'f': return(4);
		case 'd': return(8);
		case 'g': return(16); // FIXME
		case 'l': return(4);
		case 'B': return(1);
		case 'H': return(2);
		case 'I': return(4);
		case 'L': return(4);
		case 'q': return(8);
		case 'Q': return(8);
		case 'p': return(sizeof(void*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 'P': return(sizeof(void*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 'z': return(sizeof(char*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 'Z': return(sizeof(char*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 's': return(sizeof(char*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 'S': return(sizeof(char*)); // this isn't useful if it isn't the machine pointer, so maybe think about it.
		case 'x': return(1); // padding
#ifdef WIN32
		case 'V': return(4);
#endif
		default: return(0); /* FIXME */
		}
	}
}
// TODO do 64 bit systems align Q differently?
static size_t getAlignment(char c) { /* note that this is only used for MACHINE_BYTE_ORDER_ALIGNED */
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
	case 'g': return(sizeof(long) == 8 ? 16 : 
#ifdef WIN32
8
#else
4
#endif
); 
	case 'l': return(sizeof(long));
	case 'p': return(sizeof(long) == 8 ? 8 : 4);
	case 'P': return(sizeof(long) == 8 ? 8 : 4);
	case 'z': return(sizeof(long) == 8 ? 8 : 4);
	case 'Z': return(sizeof(long) == 8 ? 8 : 4);
	case 's': return(sizeof(long) == 8 ? 8 : 4);
	case 'S': return(sizeof(long) == 8 ? 8 : 4);
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
	case 'Q': return(sizeof(unsigned long) == 8 ? 8 : 
#ifdef WIN32
8
#else
4
#endif
);
#ifdef WIN32
	case 'V': return(8);
#endif
	default: return(0); /* FIXME */
	}
}
// TODO sub-records, arrays, endianness. pointers to other stuff.
// at least: execv "ls" ["ls"]  with record packer "[p]"

#define FALLBACK_IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

static inline bool machineIntegerBigEndianP(void) /* TODO pure */ {
#if __BYTE_ORDER  == __BIG_ENDIAN
	return(true);
#elif __BYTE_ORDER__ == __LITTLE_ENDIAN
	return(false);
#else
	return(FALLBACK_IS_BIG_ENDIAN);
#endif
}
static inline bool machineFloatingPointBigEndianP(void) /* TODO pure */ {
#if __BYTE_ORDER  == __BIG_ENDIAN
	return(true);
#elif __BYTE_ORDER__ == __LITTLE_ENDIAN
	return(false);
#else
	return(FALLBACK_IS_BIG_ENDIAN);
#endif
}
static inline bool machineNoneBigEndianP(void) /* TODO pure */{
	return(false);
}

#define BIG_ENDIAN_BYTE_ORDER_P(type) (byteOrder == BIG_ENDIAN_BYTE_ORDER || ((byteOrder == MACHINE_BYTE_ORDER || byteOrder == MACHINE_BYTE_ORDER_ALIGNED) && machine ## type ## BigEndianP()))

#define PACK_BUF(type, buffer) \
	assert(sizeof(buffer) == size); \
	size_t size2 = size; \
	if(BIG_ENDIAN_BYTE_ORDER_P(type) != machine ## type ## BigEndianP()) { \
		char* b = (char*) &buffer; \
		b += size; \
		for(--b; size2 > 0; --size2, --b) \
			sst << *b; \
	} else { \
		char* b = (char*) &buffer; \
		for(; size2 > 0; --size2, ++b) \
			sst << *b; \
	}
static inline size_t pack_atom_value(enum ByteOrder byteOrder, char formatC, AST::NodeT headNode, std::stringstream& sst) {
	size_t size = getSize(byteOrder, formatC); 
	uint64_t mask = ~0; 
	uint64_t limit;
	long long value = 0;
	bool B_out_of_range = false;
	limit = (1 << (8 * size - 1));
	mask = limit - 1;
	if(formatC == 'b' || formatC == 'h' || formatC == 'i' || formatC == 'l' || formatC == 'q') {
		value = (long long) Evaluators::get_long_long(headNode); // FIXME bigger values
		mask = ~mask;
		if((value > 0 && (value & mask) != 0) || (value < 0 && ((-value-1) & mask) != 0))
			B_out_of_range = true;
	} else if(formatC == 'B' || formatC == 'H' || formatC == 'I' || formatC == 'L' || formatC == 'Q') {
		value = (long long) Evaluators::get_long_long(headNode); // FIXME bigger values
		mask = (mask << 1) + 1;
		mask = ~mask;
		if((value > 0 && (value & mask) != 0) || (value < 0))
			B_out_of_range = true;
	} else { /* non-integral */
		switch(formatC) {
		case 'f': {
			assert(size == 4);
			float result = Evaluators::get_float(headNode);
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'd': {
			assert(size == 8);
			double result = Evaluators::get_double(headNode);
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'g': {
			assert(size == 16);
			long double result = Evaluators::get_long_double(headNode);
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'z':
		case 's':
		case 'p': {
			if(headNode == NULL)
				throw Evaluators::EvaluationException("packRecord: that field cannot be nil");
			void* result = Evaluators::get_pointer(headNode);
			PACK_BUF(None, result)
			return(size);
		}
		case 'Z':
		case 'S':
		case 'P': {
			void* result = headNode ? Evaluators::get_pointer(headNode) : NULL;
			PACK_BUF(None, result)
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
	if(BIG_ENDIAN_BYTE_ORDER_P(Integer)) {
		int shift = 8 * size - 8;
		for(size_t c = size; c > 0; --c, shift -= 8)
			sst << (char) (((unsigned char) (value >> shift)) & 0xFF);
	} else {
		for(size_t c = size; c > 0; --c) {
			sst << (char) (((unsigned char) value) & 0xFF);
			value >>= 8;
		}
	}
	return(size);
}
static void Record_skip_format(size_t& position /* in format Str */, AST::Str* formatStr) {
	if(position > formatStr->size)
		position = formatStr->size;
	for(const char* format = ((const char*) formatStr->native) + position; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == ']')
			break;
		if(formatC == '[') {
			++position;
			size_t subPosition;
			Record_skip_format(position, formatStr);
			format = ((const char*) formatStr->native) + position;
			if(*format != ']')
				throw Evaluators::EvaluationException("packRecord: format Str is invalid");
		}
	}
}
void Record_pack(enum ByteOrder byteOrder, size_t& position /* in format Str */, size_t& offset /* in output */, AST::Str* formatStr, AST::NodeT data, std::stringstream& sst, std::vector<size_t>& offsets) {
	size_t new_offset = 0;
	if(formatStr == NULL)
		return;
	AST::Cons* consNode = Evaluators::evaluateToCons(data);
	if(position > formatStr->size)
		position = formatStr->size;
	for(const char* format = ((const char*) formatStr->native) + position; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == ']')
			break;
		if(formatC == '<' || formatC == '>' || formatC == '=' || formatC == '@') {
			if(formatC == '<')
				byteOrder = LITTLE_ENDIAN_BYTE_ORDER;
			else if(formatC == '>')
				byteOrder = BIG_ENDIAN_BYTE_ORDER;
			else if(formatC == '=')
				byteOrder = MACHINE_BYTE_ORDER;
			else
				byteOrder = MACHINE_BYTE_ORDER_ALIGNED;
			continue;
		}
		if(formatC == 'x') {
			sst << (char) 0;
			offset += 1;
			continue;
		}
		if(consNode == NULL)
			throw Evaluators::EvaluationException("packRecord: not enough data for format.");
		AST::NodeT headNode = Evaluators::reduce(consNode->head);
		consNode = Evaluators::evaluateToCons(consNode->tail);
		if(formatC == '[') {
			++position;
			size_t subPosition;
			subPosition = position;
			for(AST::Cons* consNode = Evaluators::evaluateToCons(headNode); consNode; consNode = Evaluators::evaluateToCons(Evaluators::reduce(consNode->tail))) {
				subPosition = position;
				AST::NodeT headNode = Evaluators::reduce(consNode->head);
				Record_pack(byteOrder, subPosition, offset, formatStr, headNode, sst, offsets);
			}
			if(subPosition == position) { /* didn't do anything, so we have to fake the advance in the format string. */
				Record_skip_format(subPosition, formatStr);
			}
			position = subPosition;
			format = ((const char*) formatStr->native) + position;
			if(*format != ']')
				throw Evaluators::EvaluationException("packRecord: format Str is invalid");
		} else {
			if(byteOrder == MACHINE_BYTE_ORDER_ALIGNED) {
				size_t align = getAlignment(formatC); 
				new_offset = (offset + align - 1) & ~(align - 1);
				for(; offset < new_offset; ++offset)
					sst << '\0';
			}
			offsets.push_back(offset);
			size_t size = pack_atom_value(byteOrder, formatC, headNode, sst);
			offset += size;
/*
long long get_long_long(AST::NodeT root);
void* get_pointer(AST::NodeT root);
bool get_boolean(AST::NodeT root);
char* get_string(AST::NodeT root);
float get_float(AST::NodeT root);
long double get_long_double(AST::NodeT root);
double get_double(AST::NodeT root);
*/
		}
	}
}
#define DECODE_BUF(type, destination) \
	unsigned char* d = (unsigned char*) &destination; \
	if(BIG_ENDIAN_BYTE_ORDER_P(type) != machine ## type ## BigEndianP()) { \
		d += size; \
		for(--d; size > 0; --size, ++codedData, --d) \
			*d = *codedData; \
	} else \
		for(; size > 0; --size, ++codedData, ++d) \
			*d = *codedData;

static AST::NodeT skipApplications(AST::NodeT app, size_t count) {
	for(; count > 0; --count) {
		if(application_P(app))
			app = get_application_operand(app);
		else {
			throw Evaluators::EvaluationException("skipApplications cannot handle non-applications");
		}
	}
	return(app);
}
static inline AST::NodeT decode(enum ByteOrder byteOrder, AST::NodeT repr, size_t reprOffset, char formatC, const unsigned char* codedData, size_t size) {
	switch(formatC) {
	case 'b':
		{
			char value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'h':
		{
			short value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'i':
		{
			int value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'l':
		{
			long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value)); // FIXME size
		}
	case 'B':
		{
			unsigned char value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'H':
		{
			unsigned short value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'I':
		{
			unsigned int value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'L':
		{
			unsigned long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNativeU((unsigned long long) value));
		}
	case 'q':
		{
			long long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative(value));
		}
	case 'Q':
		{
			unsigned long long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNativeU(value));
		}
	case 'f':
		{
			float value;
			DECODE_BUF(FloatingPoint, value)
			return(Numbers::internNative((Numbers::NativeFloat) value));
		}
	case 'd':
		{
			double value;
			DECODE_BUF(FloatingPoint, value)
			return(Numbers::internNative((Numbers::NativeFloat) value));
		}
	case 'g':
		{
			long double value;
			DECODE_BUF(FloatingPoint, value)
			return(Numbers::internNative((Numbers::NativeFloat) value)); // FIXME larger
		}
	case 'p':
		{
			void* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				throw Evaluators::EvaluationException("unpack: cannot decode NULL pointer (maybe use \"P\" ?)");
			AST::NodeT r = AST::makeApplication(AST::symbolFromStr("head"), skipApplications(repr, reprOffset));
			return(AST::makeBox(value, r)); // this could also be made to reuse existing wrappers.
		}
	case 'P':
		{
			void* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				return(NULL);
			else {
				AST::NodeT r = AST::makeApplication(AST::symbolFromStr("head"), skipApplications(repr, reprOffset));
				return(AST::makeBox(value, r));
			}
		}
	case 'z':
	case 's':
		{
			char* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				throw Evaluators::EvaluationException("unpack: cannot decode NULL pointer (maybe use \"P\" ?)");
			return(AST::makeStr(value));
		}
	case 'Z':
	case 'S':
		{
			char* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				return(NULL);
			else
				return(AST::makeStr(value));
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
/* builds (tail (tail (tail ... (tail suffix))))) with a total of #count tails. */
static AST::NodeT tailtailtail(AST::NodeT suffix, size_t count) {
	if(count == 0)
		return(suffix);
	else
		return(AST::makeApplication(AST::symbolFromStr("tail"), tailtailtail(suffix, count - 1)));
}

// TODO record unpacker for "[p]"
AST::NodeT Record_unpack(enum ByteOrder byteOrder, AST::Str* formatStr, AST::Box* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("unpackRecord needs format string.");
	AST::NodeT repr = AST::makeApplication(AST::makeApplication(&RecordUnpacker, formatStr), dataStr);
	size_t resultOffset = 0;
	size_t resultCount = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=' || formatC == '@')
			continue;
		++resultCount;
	}
	position = 0;
	if(dataStr == NULL) { // TODO check formatstr etc.
		if(resultCount == 0) // fine.
			return(NULL);
		throw Evaluators::EvaluationException("unpackRecord needs data string.");
	}
	const unsigned char* codedData = (const unsigned char*) dataStr->native;
	size_t remainderLen = dynamic_cast<AST::Str*>(dataStr) ? ((AST::Str*) dataStr)->size : 9999999; // FIXME
	size_t offset = 0;
	size_t new_offset = 0;
	AST::Cons* result = NULL;
	AST::Cons* tail = NULL;
	repr = tailtailtail(repr, resultCount);
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=' || formatC == '@') {
			if(formatC == '<')
				byteOrder = LITTLE_ENDIAN_BYTE_ORDER;
			else if(formatC == '>')
				byteOrder = BIG_ENDIAN_BYTE_ORDER;
			else if(formatC == '=')
				byteOrder = MACHINE_BYTE_ORDER;
			else
				byteOrder = MACHINE_BYTE_ORDER_ALIGNED;
			continue;
		}
		size_t size = getSize(byteOrder, formatC);
		if(byteOrder == MACHINE_BYTE_ORDER_ALIGNED) {
			size_t align = getAlignment(formatC);
			new_offset = (offset + align - 1) & ~(align - 1);
		} else
			new_offset = offset;
		if(remainderLen < new_offset - offset)
			throw Evaluators::EvaluationException("unpackRecord: not enough coded data for format padding.");
		remainderLen -= new_offset - offset;
		codedData += new_offset - offset;
		offset = new_offset;
		if(remainderLen < size)
			throw Evaluators::EvaluationException("unpackRecord: not enough coded data for format.");
		AST::Cons* n = AST::makeCons(decode(byteOrder, repr, resultCount - resultOffset, formatC, codedData, size), NULL);
		if(!tail)
			result = n;
		else
			tail->tail = n;
		++resultOffset;
		tail = n;
		codedData += size;
		remainderLen -= size;
		offset += size;
	}
	return(result);
}
// this doesn't work with record packers like "[p]" which have dynamic length.
size_t Record_get_size(enum ByteOrder byteOrder, AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("recordSize needs format string.");
	size_t offset = 0;
	size_t new_offset = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=' || formatC == '@') {
			if(formatC == '<')
				byteOrder = LITTLE_ENDIAN_BYTE_ORDER;
			else if(formatC == '>')
				byteOrder = BIG_ENDIAN_BYTE_ORDER;
			else if(formatC == '=')
				byteOrder = MACHINE_BYTE_ORDER;
			else
				byteOrder = MACHINE_BYTE_ORDER_ALIGNED;
			continue;
		}
		if(byteOrder == MACHINE_BYTE_ORDER_ALIGNED) {
			size_t align = getAlignment(*format);
			new_offset = (offset + align - 1) & ~(align - 1);
			offset = new_offset;
		}
		size_t size = getSize(byteOrder, formatC);
		offset += size;
	}
	return(offset);
}
// this doesn't work with record packers like "[p]" which have dynamic length.
bool Record_has_pointers(AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("recordSize needs format string.");
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=' || *format == '@')
			continue;
		switch(*format) {
		case 'p':
		case 'P':
		case 's':
		case 'S':
		case 'z':
		case 'Z':
			return(true);
		}
	}
	return(false);
}
AST::Str* Record_allocate(size_t size, bool bAtomicity) {
	if(size < 1) // TODO
		throw Evaluators::EvaluationException("cannot allocate record with unknown size");
	char* buffer = new (UseGC) char[size];
	AST::Str* result = AST::makeStrRaw(buffer, size, bAtomicity);
	return(result);
}
using namespace Evaluators;
static AST::NodeT pack(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	std::stringstream sst;
	size_t position = 0;
	size_t offset = 0;
	std::vector<size_t> offsets;
	Record_pack(MACHINE_BYTE_ORDER_ALIGNED, position, offset, dynamic_cast<AST::Str*>(a), b, sst, offsets);
	std::string v = sst.str();
	return(AST::makeStrCXX(v));
}
DEFINE_BINARY_OPERATION(RecordPacker, pack)
REGISTER_BUILTIN(RecordPacker, 2, 0, AST::symbolFromStr("packRecord"))
static AST::NodeT unpack(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	// TODO size
	return(Record_unpack(MACHINE_BYTE_ORDER_ALIGNED, dynamic_cast<AST::Str*>(a), dynamic_cast<AST::Box*>(b)));
}
DEFINE_BINARY_OPERATION(RecordUnpacker, unpack)
REGISTER_BUILTIN(RecordUnpacker, 2, 0, AST::symbolFromStr("unpackRecord"))

DEFINE_SIMPLE_OPERATION(RecordSizeCalculator, Numbers::internNative((Numbers::NativeInt) Record_get_size(MACHINE_BYTE_ORDER_ALIGNED, dynamic_cast<AST::Str*>(reduce(argument)))))
REGISTER_BUILTIN(RecordSizeCalculator, 1, 0, AST::symbolFromStr("recordSize"))

static AST::NodeT wrapAllocateRecord(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::Str* format = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	AST::NodeT result = Record_allocate(Record_get_size(MACHINE_BYTE_ORDER_ALIGNED, format), !Record_has_pointers(format));
	return(Evaluators::makeIOMonad(result, world));
}
static AST::NodeT wrapAllocateMemory(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	Numbers::NativeInt size = 0;
	if(!Numbers::toNativeInt(iter->second, size))
		throw Evaluators::EvaluationException("cannot allocate memory with unknown size");
	++iter;
	AST::NodeT world = iter->second;
	AST::NodeT result = Record_allocate(size, false/*TODO atomic as an option*/);
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(RecordAllocator, {
	return(wrapAllocateRecord(fn, argument));
})
DEFINE_FULL_OPERATION(MemoryAllocator, {
	return(wrapAllocateMemory(fn, argument));
})
REGISTER_BUILTIN(RecordAllocator, 2, 0, AST::symbolFromStr("allocateRecord!"))
REGISTER_BUILTIN(MemoryAllocator, (-2), 0, AST::symbolFromStr("allocateMemory!"))
static AST::NodeT wrapDuplicateRecord(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::Str* record = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	AST::Str* result;
	if(record == NULL)
		result = NULL;
	else {
		result = Record_allocate(record->size, false); // since this is monadic, nothing stops the C runtime from putting pointers in there.
		memcpy(result->native, record->native, record->size);
	}
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(RecordDuplicator, {
	return(wrapDuplicateRecord(fn, argument));
})
REGISTER_BUILTIN(RecordDuplicator, 2, 0, AST::symbolFromStr("duplicateRecord!"))

AST::NodeT listFromCharS(const char* text, size_t remainder) {
	if(remainder == 0)
		return(NULL);
	else
		return(makeCons(Numbers::internNative((Numbers::NativeInt) (unsigned char) *text), listFromCharS(text + 1, remainder - 1)));
}
AST::NodeT listFromStr(AST::Str* node) {
	return(listFromCharS((const char*) node->native, node->size));
}
AST::NodeT strFromList(AST::Cons* node) {
	std::stringstream sst;
	Numbers::NativeInt c;
	for(; node; node = evaluateToCons(node->tail)) {
		if(!Numbers::toNativeInt(node->head, c) || c < 0 || c > 255) // oops
			throw Evaluators::EvaluationException("list cannot be represented as a str.");
		sst << (char) c;
	}
	std::string v = sst.str();
	AST::Str* result = Record_allocate(v.length(), false/*chicken*/);
	memcpy(result->native, v.c_str(), result->size);
	return(result);
}
static AST::NodeT substr(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::Str* mBox = dynamic_cast<AST::Str*>(iter->second);
	if(iter->second == NULL)
		return(NULL);
	++iter;
	if(!mBox)
		throw Evaluators::EvaluationException("substr on non-string is undefined");
	int beginning = Evaluators::get_nearest_int(iter->second);
	++iter;
	int end = Evaluators::get_nearest_int(iter->second);
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
	return(AST::makeStrRaw(p, len, mBox->bAtomicity)); // TODO maybe copy.
}
AST::Str* str_until_zero(AST::Box* value) {
	AST::Str* result;
	if(value == NULL)
		return(NULL);
	result = AST::makeStrRaw((char*) value->native, strlen((char*) value->native), true); // TODO copy?
	return(result);
}

DEFINE_SIMPLE_OPERATION(ListFromStrGetter, (argument = reduce(argument), str_P(argument) ? listFromStr(dynamic_cast<AST::Str*>(argument)) : nil_P(argument) ? argument : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrFromListGetter, (argument = reduce(argument), (cons_P(argument) || nil_P(argument)) ? strFromList(dynamic_cast<AST::Cons*>(argument)) : FALLBACK))
DEFINE_FULL_OPERATION(SubstrGetter, return(substr(fn, argument));)
DEFINE_SIMPLE_OPERATION(StrUntilZeroGetter, str_until_zero(dynamic_cast<AST::Box*>(reduce(argument))))
REGISTER_BUILTIN(ListFromStrGetter, 1, 0, AST::symbolFromStr("listFromStr"))
REGISTER_BUILTIN(StrFromListGetter, 1, 0, AST::symbolFromStr("strFromList"))
REGISTER_BUILTIN(SubstrGetter, 3, 0, AST::symbolFromStr("substr"))
REGISTER_BUILTIN(StrUntilZeroGetter, 1, 0, AST::symbolFromStr("strUntilZero"))

}; /* end namespace FFIs */
