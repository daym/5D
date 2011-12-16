#include <sstream>
#include "Evaluators/Evaluators"
#include "FFIs/Record"

namespace FFIs {

static size_t getSize(char c) {
	switch(c) {
	case 'b': return(1);
	case 'h': return(2);
	case 'i': return(4);
	case 'f': return(4);
#if sizeof(long) == 8
	case 'd': return(8);
	case 'D': return(16);
	case 'l': return(8);
	case 'p': return(8);
	case 'P': return(8);
#else /* 32 bit */
	case 'p': return(4);
	case 'P': return(4);
	case 'l': return(4);
#ifdef WIN32	
	case 'd': return(8);
	case 'D': return(16);
#else
	case 'd': return(8);
	case 'D': return(16);
#endif
#endif
	default: return(0); /* FIXME */
	}
}
static size_t getAlignment(char c) {
	switch(c) {
	case 'b': return(1);
	case 'h': return(2);
	case 'i': return(4);
	case 'f': return(4);
#if sizeof(long) == 8
	case 'd': return(8);
	case 'D': return(16);
	case 'l': return(8);
	case 'p': return(8);
	case 'P': return(8);
#else /* 32 bit */
	case 'p': return(4);
	case 'P': return(4);
	case 'l': return(4);
#ifdef WIN32	
	case 'd': return(8);
	case 'D': return(8); // FIXME
#else
	case 'd': return(4); /* on Linux: 4; except when you specify -malign-double */
	case 'D': return(4);
#endif
#endif
	default: return(0); /* FIXME */
	}
}
// TODO sub-records, arrays, endianness.
AST::Str* Record_pack(AST::Str* formatStr, AST::Node* data) {
	size_t offset = 0;
	size_t padding = 0;
	size_t new_offset = 0;
	std::stringstream sst;
	if(formatStr == NULL)
		return(NULL);
	size_t maxAlign = 0;
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		size_t size = getSize(*format);
		for(; size > 0; --size)
			sst << '\0'; // FIXME the actual value.
		padding = maxAlign - (offset & (maxAlign - 1)) = (-offset) & (maxAlign - 1);
		//new_offset = (offset + align - 1) & ~(align - 1);
		for(; padding > 0; --padding)
			sst << '\0';
	}
	std::string v = sst.str();
	return(AST::makeStr(v.c_str()));
}
AST::Node* Record_unpack(AST::Str* formatStr, AST::Str* dataStr) {
	if(formatStr == NULL)
		throw Evaluators::EvaluationException("Record_unpack needs format string.");
	const char* format = formatStr->text.c_str();
	const char* codedData = data->text.c_str();
	size_t remainderLen = data->text.length();
	size_t maxAlign = 0;
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		size_t align = getAlignment(*format);
		if(align > maxAlign)
			maxAlign = align;
	}
	for(const char* format = formatStr->text.c_str(); *format; ++format) {
		size_t size = getSize(*format);
		if(remainderLen < size)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format.");
		codedData += size; // FIXME the actual value.
		remainderLen -= size;
		padding = maxAlign - (offset & (maxAlign - 1)) = (-offset) & (maxAlign - 1);
		//new_offset = (offset + align - 1) & ~(align - 1);
		if(remainderLen < padding)
			throw Evaluators::EvaluationException("Record_unpack: not enough coded data for format padding.");
		codedData += padding;
		remainderLen -= padding;
	}
	return(NULL); // FIXME
}


}; /* end namespace FFIs */

