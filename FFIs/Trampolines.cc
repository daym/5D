#include <stdio.h>
#include <stdlib.h>
#include <ffi.h>
#include <ffitarget.h>
#include <alloca.h>
#include <assert.h>
#include <sstream>
#include <vector>
#include "FFIs/RecordPacker"
#include "Evaluators/Evaluators"
#include "Numbers/Integer"

namespace Trampolines {

static inline ffi_type* ffiTypeFromChar(char t) {
	return t == 'i' ? &ffi_type_sint : 
	      t == 'I' ? &ffi_type_uint :
	      t == 'l' ? &ffi_type_slong :
	      t == 'L' ? &ffi_type_ulong : 
	      t == 'b' ? &ffi_type_sint8 : 
	      t == 'B' ? &ffi_type_uint8 :
	      t == 'h' ? &ffi_type_sshort :
	      t == 'H' ? &ffi_type_ushort :
	      t == 'q' ? &ffi_type_sint64 :
	      t == 'Q' ? &ffi_type_uint64 /* TODO long long */ :
	      t == 'f' ? &ffi_type_float :
	      t == 'd' ? &ffi_type_double : 
	      t == 'p' ? &ffi_type_pointer :
	      t == 'P' ? &ffi_type_pointer :
	      t == 's' ? &ffi_type_pointer :
	      t == 'S' ? &ffi_type_pointer :
	      t == 'z' ? &ffi_type_pointer :
	      t == 'Z' ? &ffi_type_pointer :
	      t == 'g' ? &ffi_type_longdouble : 
	      &ffi_type_void;
}
static AST::NodeT buildList(Evaluators::CXXArguments::const_iterator& iter, Evaluators::CXXArguments::const_iterator& endIter) {
	if(iter == endIter)
		return(NULL);
	else {
		AST::NodeT v = iter->second;
		++iter;
		return(AST::makeCons(v, buildList(iter, endIter)));
	}
}
/* note that the caller did (--endIter) so it now points to the World. */
AST::NodeT jumpFFI(Evaluators::CProcedure* proc, Evaluators::CXXArguments::const_iterator& iter, Evaluators::CXXArguments::const_iterator& endIter, AST::NodeT options, AST::NodeT world) {
	ffi_cif cif;
	ffi_abi abi = FFI_DEFAULT_ABI;
	ffi_type** argTypes;
	const char* sig = AST::get_symbol1_name(proc->fSignature);
	//abi = (*sig == 'P') ? FFI_STDCALL : FFI_DEFAULT_API; TODO
	if(*sig)
		++sig;// skip calling convention
	size_t argCount = strlen(sig); // Note: includes return value
	if(argCount == 0)
		return Evaluators::makeIOMonad(NULL, endIter->second);
	AST::Str* formatString = AST::makeStr(sig);
	std::string dataStd;
	size_t returnValueSize;
	void** args = (void**) alloca(argCount * sizeof(void*));
	{
		size_t position = 0;
		size_t offset = 0;
		std::stringstream sst;
		std::vector<size_t> offsets;
		AST::NodeT returnValue = (ffiTypeFromChar(sig[0]) == &ffi_type_pointer) ? NULL : Numbers::internNative(0);
		Record_pack(FFIs::MACHINE_BYTE_ORDER_ALIGNED, position, offset, formatString, AST::makeCons(returnValue, buildList(iter, endIter)), sst, offsets);
		assert(offsets.size() == argCount);
		dataStd = sst.str();
		returnValueSize = (argCount < 2) ? dataStd.length() : offsets[1]; // actually this could in principle be too much. Since it's the first entry, it's correct.
		char* data = (char*) dataStd.c_str();
		argTypes = (ffi_type**) GC_MALLOC(argCount * sizeof(ffi_type*));
		for(size_t i = 0; i < argCount; ++i) {
			argTypes[i] = ffiTypeFromChar(sig[i]);
			args[i] = data + offsets[i];
			if(argTypes[i] == &ffi_type_void) {
				// error
				argCount = 0;
				break;
			}
		}
	}
	if(argCount > 0 && ffi_prep_cif(&cif, abi, argCount - 1, argTypes[0], argTypes + 1) == FFI_OK) {
		ffi_call(&cif, (void (*)(void)) proc->native, args[0], args + 1);
		AST::NodeT results;
		char returnSig[2] = {*sig, 0};
		AST::NodeT rep = AST::makeStrCXX(dataStd.substr(0, returnValueSize));
		results = Record_unpack(FFIs::MACHINE_BYTE_ORDER_ALIGNED, AST::makeStr(returnSig), AST::makeBox(args[0], rep));
		AST::NodeT result = (pair_P(results) ? Evaluators::get_pair_first(results) : NULL);
		return Evaluators::makeIOMonad(result, endIter->second);
	}
	fprintf(stderr, "warning: could not find marshaller for %s\n", sig);
	return Evaluators::makeIOMonad(NULL, endIter->second);
}

};
