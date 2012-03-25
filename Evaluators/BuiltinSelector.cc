#include <stdio.h>
#ifdef WIN32
#define PATH_MAX 4096
#else
#include <limits.h>
#endif
#include "Evaluators/BuiltinSelector"
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
#include "Numbers/Real"
#include "Numbers/Ratio"

// for now, these are kept all in the main executable. After ensuring that GC actually works across DLL boundaries, we can also extract stuff into their own extension modules.
namespace Evaluators {

static AST::HashTable self;

static void add_static_builtin_binding(AST::HashTable& self, AST::Symbol* name, AST::Node* value) {
	// TODO maybe dup name. Not really from the looks of it.
	self[name->name] = value;
}
static void add_builtin_method(AST::HashTable& self, AST::Symbol* name, AST::Node* value) {
	add_static_builtin_binding(self, name, value);
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
	add_static_builtin_binding(self, Symbols::Squote, &Evaluators::Quoter); /* keep at the beginning */
	// TODO add_static_builtin_binding(self, Symbols::SrequireModule, uncurried(&RModuleLoader, self));
	// no add_builtin_method(self, Symbols::Sdescribe, uncurried(&RInformant, self));
	add_static_builtin_binding(self, Symbols::Snil, NULL);
	add_builtin_method(self, Symbols::Scolon, &Evaluators::Conser);
	add_builtin_method(self, Symbols::SconsP, &Evaluators::ConsP);
	add_builtin_method(self, Symbols::SnilP, &Evaluators::NilP);
	add_builtin_method(self, Symbols::Shead, &Evaluators::HeadGetter);
	add_builtin_method(self, Symbols::Stail, &Evaluators::TailGetter);
	add_builtin_method(self, Symbols::SintP, &Numbers::IntP);
	add_static_builtin_binding(self, Symbols::Sintzero, Numbers::internNative((Numbers::NativeInt) 0));
	add_builtin_method(self, Symbols::SintSucc, &Numbers::IntSucc);
	add_builtin_method(self, Symbols::SintegerP, &Numbers::IntegerP);
	add_builtin_method(self, Symbols::SintegerSucc, &Numbers::IntegerSucc);
	add_builtin_method(self, Symbols::Splus, &Evaluators::Adder);
	add_builtin_method(self, Symbols::Sdash, &Evaluators::Subtractor);
	add_builtin_method(self, Symbols::Sasterisk, &Evaluators::Multiplicator);
	add_builtin_method(self, Symbols::Sslash, &Evaluators::Divider);
	add_builtin_method(self, Symbols::Sdivrem, &Evaluators::QModulator2);
	add_builtin_method(self, Symbols::Slessequal, &Evaluators::LEComparer);
	add_builtin_method(self, Symbols::SfloatP, &Numbers::FloatP);
	add_builtin_method(self, Symbols::SstrP, &Evaluators::StrP);
	add_builtin_method(self, Symbols::SsymbolP, &Evaluators::SymbolP);
	add_builtin_method(self, Symbols::SaddrsLEP, &Evaluators::AddrLEComparer);
	add_builtin_method(self, Symbols::SsymbolsEqualP, &Evaluators::SymbolEqualityChecker);
	add_builtin_method(self, Symbols::SkeywordP, &Evaluators::KeywordP);
	add_builtin_method(self, Symbols::SsymbolFromStr, &Evaluators::SymbolFromStrGetter);
	add_builtin_method(self, Symbols::SkeywordFromStr, &Evaluators::KeywordFromStrGetter);
	add_builtin_method(self, Symbols::SstrFromKeyword, &Evaluators::KeywordStr);
	add_builtin_method(self, Symbols::SlistFromStr, &FFIs::ListFromStrGetter);
	add_builtin_method(self, Symbols::SstrFromList, &FFIs::StrFromListGetter);
	add_builtin_method(self, Symbols::Ssubstr, &FFIs::SubstrGetter);
	add_builtin_method(self, Symbols::SstrUntilZero, &FFIs::StrUntilZeroGetter);
	add_builtin_method(self, Symbols::SrunIO, &Evaluators::IORunner);
	add_static_builtin_binding(self, Symbols::Shasht, Evaluators::churchTrue);
	add_static_builtin_binding(self, Symbols::Shashf, Evaluators::churchFalse);
	add_static_builtin_binding(self, AST::symbolFromStr("stdin"), AST::makeBox(stdin, AST::symbolFromStr("stdin")));
	add_static_builtin_binding(self, AST::symbolFromStr("stdout"), AST::makeBox(stdout, AST::symbolFromStr("stdout")));
	add_static_builtin_binding(self, AST::symbolFromStr("stderr"), AST::makeBox(stderr, AST::symbolFromStr("stderr")));
	add_static_builtin_binding(self, AST::symbolFromStr("PATHMAX"), Numbers::internNative((Numbers::NativeInt) PATH_MAX));
	add_builtin_method(self, AST::symbolFromStr("write!"), &Evaluators::Writer);
	add_builtin_method(self, AST::symbolFromStr("flush!"), &Evaluators::Flusher);
	add_builtin_method(self, AST::symbolFromStr("readline!"), &Evaluators::LineReader);
	add_builtin_method(self, AST::symbolFromStr("messageBox!"), &FFIs::MessageBoxDisplayer);
	add_builtin_method(self, AST::symbolFromStr("requireSharedLibrary"), &FFIs::SharedLibraryLoader);
	add_builtin_method(self, AST::symbolFromStr("absolutePath!"), &FFIs::AbsolutePathGetter);
	add_builtin_method(self, AST::symbolFromStr("absolutePath?"), &FFIs::AbsolutePathP);
	add_builtin_method(self, AST::symbolFromStr("packRecord"), &FFIs::RecordPacker);
	add_builtin_method(self, AST::symbolFromStr("unpackRecord"), &FFIs::RecordUnpacker);
	add_builtin_method(self, AST::symbolFromStr("recordSize"), &FFIs::RecordSizeCalculator);
	add_builtin_method(self, AST::symbolFromStr("allocateMemory!"), &FFIs::MemoryAllocator);
	add_builtin_method(self, AST::symbolFromStr("allocateRecord!"), &FFIs::RecordAllocator); // this is actually (compose allocateMemory recordSize)
	add_builtin_method(self, AST::symbolFromStr("duplicateRecord!"), &FFIs::RecordDuplicator);
	add_builtin_method(self, AST::symbolFromStr("archDepLibName"), &FFIs::ArchDepLibNameGetter);
	add_builtin_method(self, AST::symbolFromStr("errno!"), &Evaluators::ErrnoGetter);
	add_builtin_method(self, AST::symbolFromStr("environ!"), &FFIs::EnvironGetter);
	add_builtin_method(self, AST::symbolFromStr("listFromEnviron"), &FFIs::EnvironInterner);
	add_builtin_method(self, AST::symbolFromStr("environFromList"), &FFIs::EnvironFromList);
	add_builtin_method(self, AST::symbolFromStr("makeApp"), &Evaluators::ApplicationMaker);
	add_builtin_method(self, AST::symbolFromStr("app?"), &Evaluators::ApplicationP);
	add_builtin_method(self, AST::symbolFromStr("appOperator"), &Evaluators::ApplicationOperatorGetter);
	add_builtin_method(self, AST::symbolFromStr("appOperand"), &Evaluators::ApplicationOperandGetter);
	add_builtin_method(self, AST::symbolFromStr("makeFn"), &Evaluators::AbstractionMaker);
	add_builtin_method(self, AST::symbolFromStr("fn?"), &Evaluators::AbstractionP);
	add_builtin_method(self, AST::symbolFromStr("fnParam"), &Evaluators::AbstractionParameterGetter);
	add_builtin_method(self, AST::symbolFromStr("fnBody"), &Evaluators::AbstractionBodyGetter);
	add_builtin_method(self, AST::symbolFromStr("makeRatio"), &Numbers::RatioMaker);
	add_builtin_method(self, AST::symbolFromStr("ratio?"), &Numbers::RatioP);
	add_builtin_method(self, AST::symbolFromStr("ratioNum"), &Numbers::RatioNumeratorGetter);
	add_builtin_method(self, AST::symbolFromStr("ratioDenom"), &Numbers::RatioDenominatorGetter);
	add_builtin_method(self, AST::symbolFromStr("parseMath!"), &Evaluators::RFileMathParser);
	add_builtin_method(self, AST::symbolFromStr("parseMathStr"), &Evaluators::RStrMathParser);
	add_builtin_method(self, AST::symbolFromStr("dispatchModule"), &Evaluators::ModuleDispatcher);
	add_builtin_method(self, AST::symbolFromStr("makeModuleBox"), &Evaluators::ModuleBoxMaker);
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
