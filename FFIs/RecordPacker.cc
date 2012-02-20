#include <stdio.h>
#include <sstream>
#include <assert.h>
#ifdef WIN32
//typedef short uint16_t;
#else
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
	case 'z': return(sizeof(char*));
	case 'Z': return(sizeof(char*));
	case 's': return(sizeof(char*));
	case 'S': return(sizeof(char*));
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
	case 'z': return(sizeof(long) == 8 ? 8 : 4);
	case 'Z': return(sizeof(long) == 8 ? 8 : 4);
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

#define BIG_ENDIAN_BYTE_ORDER_P(type) (byteOrder == BIG_ENDIAN_BYTE_ORDER || (byteOrder == MACHINE_BYTE_ORDER && machine ## type ## BigEndianP()))

#define PACK_BUF(type, buffer) \
	assert(sizeof(buffer) == size); \
	if(BIG_ENDIAN_BYTE_ORDER_P(type) != machine ## type ## BigEndianP()) { \
		char* b = (char*) &buffer; \
		b += size; \
		for(--b; size > 0; --size, --b) \
			sst << *b; \
	} else { \
		char* b = (char*) &buffer; \
		for(; size > 0; --size, ++b) \
			sst << *b; \
	}


static inline size_t pack_atom_value(enum ByteOrder byteOrder, char formatC, AST::Node* headNode, std::stringstream& sst) {
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
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'd': {
			double result = Evaluators::get_native_double(headNode);
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'D': {
			long double result = Evaluators::get_native_long_double(headNode);
			PACK_BUF(FloatingPoint, result)
			return(size);
		}
		case 'z':
		case 'p': {
			if(headNode == NULL)
				throw Evaluators::EvaluationException("packRecord: that field cannot be nil");
			void* result = Evaluators::get_native_pointer(headNode);
			PACK_BUF(None, result)
			return(size);
		}
		case 'Z':
		case 'P': {
			void* result = headNode ? Evaluators::get_native_pointer(headNode) : NULL;
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
void Record_pack(enum ByteOrder byteOrder, size_t& position /* in format Str */, size_t& offset /* in output */, AST::Str* formatStr, AST::Node* data, std::stringstream& sst, std::vector<size_t>& offsets) {
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
		if(formatC == '<' || formatC == '>' || formatC == '=') {
			if(formatC == '<')
				byteOrder = LITTLE_ENDIAN_BYTE_ORDER;
			else if(formatC == '>')
				byteOrder = BIG_ENDIAN_BYTE_ORDER;
			else
				byteOrder = MACHINE_BYTE_ORDER;
			continue;
		}
		if(consNode == NULL)
			throw Evaluators::EvaluationException("packRecord: not enough data for format.");
		AST::Node* headNode = Evaluators::reduce(consNode->head);
		consNode = Evaluators::evaluateToCons(consNode->tail);
		if(formatC == '[') {
			++position;
			size_t subPosition;
			subPosition = position;
			for(AST::Cons* consNode = Evaluators::evaluateToCons(headNode); consNode; consNode = Evaluators::evaluateToCons(Evaluators::reduce(consNode->tail))) {
				subPosition = position;
				AST::Node* headNode = Evaluators::reduce(consNode->head);
				Record_pack(byteOrder, subPosition, offset, formatStr, headNode, sst, offsets);
			}
			position = subPosition;
			format = ((const char*) formatStr->native) + position;
		} else {
			size_t align = getAlignment(formatC); 
			new_offset = (offset + align - 1) & ~(align - 1);
			for(; offset < new_offset; ++offset)
				sst << '\0';
			offsets.push_back(offset);
			size_t size = pack_atom_value(byteOrder, formatC, headNode, sst);
			offset += size;
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

static AST::Node* skipApplications(AST::Node* app, size_t count) {
	for(; count > 0; --count) {
		if(application_P(app))
			app = get_application_operand(app);
		else {
			throw Evaluators::EvaluationException("skipApplications cannot handle non-applications");
		}
	}
	return(app);
}
static inline AST::Node* decode(enum ByteOrder byteOrder, AST::Node* repr, size_t reprOffset, char formatC, const unsigned char* codedData, size_t size) {
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
			return(Numbers::internNative((Numbers::NativeInt) value));
		}
	case 'q':
		{
			long long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value)); // FIXME larger
		}
	case 'Q':
		{
			unsigned long long value;
			DECODE_BUF(Integer, value)
			return(Numbers::internNative((Numbers::NativeInt) value)); // FIXME larger
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
			return(Numbers::internNative((Numbers::NativeFloat) value)); // FIXME larger
		}
	case 'D':
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
			AST::Node* r = AST::makeApplication(AST::symbolFromStr("head"), skipApplications(repr, reprOffset));
			return(AST::makeBox(value, r)); // this could also be made to reuse existing wrappers.
		}
	case 'P':
		{
			void* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				return(NULL);
			else {
				AST::Node* r = AST::makeApplication(AST::symbolFromStr("head"), skipApplications(repr, reprOffset));
				return(AST::makeBox(value, r));
			}
		}
	case 'z':
		{
			char* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				throw Evaluators::EvaluationException("unpack: cannot decode NULL pointer (maybe use \"P\" ?)");
			return(AST::makeStr(value));
		}
	case 'Z':
		{
			char* value;
			DECODE_BUF(None, value)
			if(value == NULL)
				return(NULL);
			else {
				return(AST::makeStr(value));
			}
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
static AST::Node* tailtailtail(AST::Node* suffix, size_t count) {
	if(count == 0)
		return(suffix);
	else
		return(AST::makeApplication(AST::symbolFromStr("tail"), tailtailtail(suffix, count - 1)));
}

// TODO record unpacker for "[p]"
AST::Node* Record_unpack(enum ByteOrder byteOrder, AST::Str* formatStr, AST::Box* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("unpackRecord needs format string.");
	AST::Node* repr = AST::makeApplication(AST::makeApplication(&RecordUnpacker, formatStr), dataStr);
	size_t resultOffset = 0;
	size_t resultCount = 0;
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		char formatC = *format;
		if(formatC == '<' || formatC == '>' || formatC == '=')
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
		if(formatC == '<' || formatC == '>' || formatC == '=') {
			if(formatC == '<')
				byteOrder = LITTLE_ENDIAN_BYTE_ORDER;
			else if(formatC == '>')
				byteOrder = BIG_ENDIAN_BYTE_ORDER;
			else
				byteOrder = MACHINE_BYTE_ORDER;
			continue;
		}
		size_t size = getSize(formatC);
		size_t align = getAlignment(formatC);
		new_offset = (offset + align - 1) & ~(align - 1);
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
size_t Record_get_size(AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("recordSize needs format string.");
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
// this doesn't work with record packers like "[p]" which have dynamic length.
bool Record_has_pointers(AST::Str* formatStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("recordSize needs format string.");
	size_t position = 0; // in format
	for(const char* format = (const char*) formatStr->native; position < formatStr->size; ++format, ++position) {
		if(*format == '<' || *format == '>' || *format == '=')
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
static AST::Node* pack(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	std::stringstream sst;
	size_t position = 0;
	size_t offset = 0;
	std::vector<size_t> offsets;
	Record_pack(MACHINE_BYTE_ORDER, position, offset, dynamic_cast<AST::Str*>(a), b, sst, offsets);
	std::string v = sst.str();
	return(AST::makeStrCXX(v));
}
DEFINE_BINARY_OPERATION(RecordPacker, pack)
REGISTER_BUILTIN(RecordPacker, 2, 0, AST::symbolFromStr("packRecord"))
static AST::Node* unpack(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	// TODO size
	return(Record_unpack(MACHINE_BYTE_ORDER, dynamic_cast<AST::Str*>(a), dynamic_cast<AST::Box*>(b)));
}
DEFINE_BINARY_OPERATION(RecordUnpacker, unpack)
REGISTER_BUILTIN(RecordUnpacker, 2, 0, AST::symbolFromStr("unpackRecord"))

DEFINE_SIMPLE_OPERATION(RecordSizeCalculator, Numbers::internNative((Numbers::NativeInt) Record_get_size(dynamic_cast<AST::Str*>(reduce(argument)))))
REGISTER_BUILTIN(RecordSizeCalculator, 1, 0, AST::symbolFromStr("recordSize"))

static AST::Node* wrapAllocateRecord(AST::Node* options, AST::Node* argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::Str* format = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	AST::Node* result = Record_allocate(Record_get_size(format), !Record_has_pointers(format));
	return(Evaluators::makeIOMonad(result, world));
}
static AST::Node* wrapAllocateMemory(AST::Node* options, AST::Node* argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	bool B_ok = false;
	Numbers::NativeInt size = Numbers::toNativeInt(iter->second, B_ok);
	if(!B_ok)
		throw Evaluators::EvaluationException("cannot allocate memory with unknown size");
	++iter;
	AST::Node* world = iter->second;
	AST::Node* result = Record_allocate(size, false/*TODO atomic as an option*/);
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(RecordAllocator, {
	return(wrapAllocateRecord(fn, argument));
})
DEFINE_FULL_OPERATION(MemoryAllocator, {
	return(wrapAllocateMemory(fn, argument));
})
REGISTER_BUILTIN(RecordAllocator, 2, 0, AST::symbolFromStr("allocateRecord"))
REGISTER_BUILTIN(MemoryAllocator, (-2), 0, AST::symbolFromStr("allocateMemory"))
static AST::Node* wrapDuplicateRecord(AST::Node* options, AST::Node* argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::Str* record = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
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
	AST::Str* result = Record_allocate(v.length(), false/*chicken*/);
	memcpy(result->native, v.c_str(), result->size);
	return(result);
}
static AST::Node* substr(AST::Node* options, AST::Node* argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
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
	return(AST::makeStrRaw(p, len, mBox->bAtomicity)); // TODO maybe copy.
}
AST::Str* str_until_zero(AST::Box* value) {
	AST::Str* result;
	if(value == NULL)
		return(NULL);
	result = AST::makeStrRaw((char*) value->native, strlen((char*) value->native), true); // TODO copy?
	return(result);
}

DEFINE_SIMPLE_OPERATION(ListFromStrGetter, (argument = reduce(argument), str_P(argument) ? listFromStr(dynamic_cast<AST::Str*>(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrFromListGetter, (argument = reduce(argument), (cons_P(argument) || nil_P(argument)) ? strFromList(dynamic_cast<AST::Cons*>(argument)) : FALLBACK))
DEFINE_FULL_OPERATION(SubstrGetter, return(substr(fn, argument));)
DEFINE_SIMPLE_OPERATION(StrUntilZeroGetter, str_until_zero(dynamic_cast<AST::Box*>(reduce(argument))))
REGISTER_BUILTIN(ListFromStrGetter, 1, 0, AST::symbolFromStr("listFromStr"))
REGISTER_BUILTIN(StrFromListGetter, 1, 0, AST::symbolFromStr("strFromList"))
REGISTER_BUILTIN(SubstrGetter, 3, 0, AST::symbolFromStr("substr"))
REGISTER_BUILTIN(StrUntilZeroGetter, 1, 0, AST::symbolFromStr("strUntilZero"))

}; /* end namespace FFIs */
