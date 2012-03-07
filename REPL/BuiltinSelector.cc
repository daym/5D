#include <stdio.h>
#ifdef WIN32
#define PATH_MAX 4096
#else
#include <limits.h>
#endif
#include "REPL/BuiltinSelector"
#include "AST/AST"
#include "AST/HashTable"
#include "AST/Symbol"
#include "AST/Symbols"
#include "Evaluators/Builtins"
#include "Evaluators/FFI"
#include "FFIs/RecordPacker"
#include "FFIs/UI"
#include "FFIs/FFIs"
#include "Evaluators/Evaluators"
#include "Numbers/Integer"
#include "FFIs/ProcessInfos"

// for now, these are kept all in the main executable. After ensuring that GC actually works across DLL boundaries, we can also extract stuff into their own extension modules.
namespace REPLB {

static AST::HashTable self;

static void REPL_add_static_builtin_binding(AST::HashTable& self, AST::Symbol* name, AST::Node* value) {
	// TODO maybe dup name. Not really from the looks of it.
	self[name->name] = value;
}
static void REPL_add_builtin_method(AST::HashTable& self, AST::Symbol* name, AST::Node* value) {
	REPL_add_static_builtin_binding(self, name, value);
}

static AST::Cons* consFromKeys(AST::HashTable::const_iterator begin, AST::HashTable::const_iterator end) {
	if(begin == end)
		return(NULL);
	else {
		AST::Symbol* name = AST::symbolFromStr(begin->first);
		++begin;
		return(AST::makeCons(name, consFromKeys(begin, end)));
	}
}

static int didInit = 0;
void BuiltinSelector_init(void) {
	if(didInit)
		return;
	didInit = 1;
	REPL_add_static_builtin_binding(self, Symbols::Squote, &Evaluators::Quoter); /* keep at the beginning */
	// TODO REPL_add_static_builtin_binding(self, Symbols::SrequireModule, uncurried(&RModuleLoader, self));
	// no REPL_add_builtin_method(self, Symbols::Sdescribe, uncurried(&RInformant, self));
	REPL_add_static_builtin_binding(self, Symbols::Snil, NULL);
	REPL_add_builtin_method(self, Symbols::Scolon, &Evaluators::Conser);
	REPL_add_builtin_method(self, Symbols::SconsP, &Evaluators::ConsP);
	REPL_add_builtin_method(self, Symbols::SnilP, &Evaluators::NilP);
	REPL_add_builtin_method(self, Symbols::Shead, &Evaluators::HeadGetter);
	REPL_add_builtin_method(self, Symbols::Stail, &Evaluators::TailGetter);
	REPL_add_builtin_method(self, Symbols::SintP, &Numbers::IntP);
	REPL_add_static_builtin_binding(self, Symbols::Sintzero, Numbers::internNative((Numbers::NativeInt) 0));
	REPL_add_builtin_method(self, Symbols::SintSucc, &Numbers::IntSucc);
	REPL_add_builtin_method(self, Symbols::SintegerP, &Numbers::IntegerP);
	REPL_add_builtin_method(self, Symbols::SintegerSucc, &Numbers::IntegerSucc);
	REPL_add_builtin_method(self, Symbols::Splus, &Evaluators::Adder);
	REPL_add_builtin_method(self, Symbols::Sdash, &Evaluators::Subtractor);
	REPL_add_builtin_method(self, Symbols::Sasterisk, &Evaluators::Multiplicator);
	REPL_add_builtin_method(self, Symbols::Sslash, &Evaluators::Divider);
	REPL_add_builtin_method(self, Symbols::Sdivrem, &Evaluators::QModulator2);
	REPL_add_builtin_method(self, Symbols::Slessequal, &Evaluators::LEComparer);
	REPL_add_builtin_method(self, Symbols::SfloatP, &Numbers::FloatP);
	REPL_add_builtin_method(self, Symbols::SstrP, &Evaluators::StrP);
	REPL_add_builtin_method(self, Symbols::SsymbolP, &Evaluators::SymbolP);
	REPL_add_builtin_method(self, Symbols::SaddrsLEP, &Evaluators::AddrLEComparer);
	REPL_add_builtin_method(self, Symbols::SsymbolsEqualP, &Evaluators::SymbolEqualityChecker);
	REPL_add_builtin_method(self, Symbols::SkeywordP, &Evaluators::KeywordP);
	REPL_add_builtin_method(self, Symbols::SsymbolFromStr, &Evaluators::SymbolFromStrGetter);
	REPL_add_builtin_method(self, Symbols::SkeywordFromStr, &Evaluators::KeywordFromStrGetter);
	REPL_add_builtin_method(self, Symbols::SstrFromKeyword, &Evaluators::KeywordStr);
	REPL_add_builtin_method(self, Symbols::SlistFromStr, &FFIs::ListFromStrGetter);
	REPL_add_builtin_method(self, Symbols::SstrFromList, &FFIs::StrFromListGetter);
	REPL_add_builtin_method(self, Symbols::Ssubstr, &FFIs::SubstrGetter);
	REPL_add_builtin_method(self, Symbols::SstrUntilZero, &FFIs::StrUntilZeroGetter);
	REPL_add_builtin_method(self, Symbols::SrunIO, &Evaluators::IORunner);
	REPL_add_static_builtin_binding(self, Symbols::Shasht, Evaluators::churchTrue);
	REPL_add_static_builtin_binding(self, Symbols::Shashf, Evaluators::churchFalse);
	REPL_add_static_builtin_binding(self, AST::symbolFromStr("stdin"), AST::makeBox(stdin, AST::symbolFromStr("stdin")));
	REPL_add_static_builtin_binding(self, AST::symbolFromStr("stdout"), AST::makeBox(stdout, AST::symbolFromStr("stdout")));
	REPL_add_static_builtin_binding(self, AST::symbolFromStr("stderr"), AST::makeBox(stderr, AST::symbolFromStr("stderr")));
	REPL_add_static_builtin_binding(self, AST::symbolFromStr("PATHMAX"), Numbers::internNative((Numbers::NativeInt) PATH_MAX));
	REPL_add_builtin_method(self, AST::symbolFromStr("write!"), &Evaluators::Writer);
	REPL_add_builtin_method(self, AST::symbolFromStr("flush!"), &Evaluators::Flusher);
	REPL_add_builtin_method(self, AST::symbolFromStr("readline!"), &Evaluators::LineReader);
	REPL_add_builtin_method(self, AST::symbolFromStr("messageBox!"), &FFIs::MessageBoxDisplayer);
	REPL_add_builtin_method(self, AST::symbolFromStr("requireSharedLibrary"), &FFIs::SharedLibraryLoader);
	REPL_add_builtin_method(self, AST::symbolFromStr("absolutePath!"), &FFIs::AbsolutePathGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("absolutePath?"), &FFIs::AbsolutePathP);
	REPL_add_builtin_method(self, AST::symbolFromStr("packRecord"), &FFIs::RecordPacker);
	REPL_add_builtin_method(self, AST::symbolFromStr("unpackRecord"), &FFIs::RecordUnpacker);
	REPL_add_builtin_method(self, AST::symbolFromStr("recordSize"), &FFIs::RecordSizeCalculator);
	REPL_add_builtin_method(self, AST::symbolFromStr("allocateMemory!"), &FFIs::MemoryAllocator);
	REPL_add_builtin_method(self, AST::symbolFromStr("allocateRecord!"), &FFIs::RecordAllocator); // this is actually (compose allocateMemory recordSize)
	REPL_add_builtin_method(self, AST::symbolFromStr("duplicateRecord!"), &FFIs::RecordDuplicator);
	REPL_add_builtin_method(self, AST::symbolFromStr("archDepLibName"), &FFIs::ArchDepLibNameGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("errno!"), &FFIs::ErrnoGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("environ!"), &FFIs::EnvironGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("listFromEnviron"), &FFIs::EnvironInterner);
	REPL_add_builtin_method(self, AST::symbolFromStr("environFromList"), &FFIs::EnvironFromList);
	REPL_add_builtin_method(self, AST::symbolFromStr("makeApp"), &Evaluators::ApplicationMaker);
	REPL_add_builtin_method(self, AST::symbolFromStr("app?"), &Evaluators::ApplicationP);
	REPL_add_builtin_method(self, AST::symbolFromStr("appOperator"), &Evaluators::ApplicationOperatorGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("appOperand"), &Evaluators::ApplicationOperandGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("makeFn"), &Evaluators::AbstractionMaker);
	REPL_add_builtin_method(self, AST::symbolFromStr("fn?"), &Evaluators::AbstractionP);
	REPL_add_builtin_method(self, AST::symbolFromStr("fnParam"), &Evaluators::AbstractionParameterGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("fnBody"), &Evaluators::AbstractionBodyGetter);
	REPL_add_builtin_method(self, AST::symbolFromStr("parseMath!"), &Evaluators::RFileMathParser);
	REPL_add_builtin_method(self, AST::symbolFromStr("parseMathStr"), &Evaluators::RStrMathParser);
	self[Symbols::Sexports->name] = consFromKeys(self.begin(), self.end());
}

static AST::Node* getEntry(AST::Symbol* name) {
	if(!name || !name->name)
		return(NULL);
	AST::HashTable::const_iterator iter = self.find(name->name);
	if(iter == self.end())
		return(NULL);
	return(iter->second);
}

// FIXME error checking
DEFINE_SIMPLE_OPERATION(BuiltinGetter, getEntry(dynamic_cast<AST::Symbol*>(Evaluators::reduce(argument))))
REGISTER_BUILTIN(BuiltinGetter, 1, 0, Symbols::SBuiltins)

};
