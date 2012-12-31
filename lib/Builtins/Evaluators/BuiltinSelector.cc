#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#ifdef WIN32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <limits.h>
#endif
#include "Evaluators/BuiltinSelector"
#include "Values/Values"
#include "Values/Symbols"
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
#include "Evaluators/ModuleLoader"
#include "Evaluators/Logic"
#include "Scanners/Scanner"
#include "Scanners/MathParser"
#include "ModuleSystem/Modules"
#include "Formatters/SExpression"
#include "Formatters/Math"
#include "Formatters/LATEX"
#include <5D/ModuleSystem>

namespace Evaluators {
using namespace Values;

static HashTable self;

static void add_static_builtin_binding(HashTable& self, NodeT name, NodeT value) {
	// TODO maybe dup name. Not really from the looks of it.
	const char* n = get_symbol_name(name);
	// TODO non-symbols?
	self[n] = value;
}
static void add_builtin_method(HashTable& self, NodeT name, NodeT value) {
	add_static_builtin_binding(self, name, value);
}
static NodeT consFromKeys(HashTable::const_iterator begin, HashTable::const_iterator end) {
	if(begin == end)
		return(NULL);
	else {
		NodeT name = symbolFromStr(begin->first);
		++begin;
		return(makeCons(name, consFromKeys(begin, end)));
	}
}

NodeT getBuiltinEntry(NodeT sym) {
	if(nil_P(sym)) { /* ?!?!?! */
		return(NULL);
	} else {
		const char* name = get_symbol1_name(sym);
		HashTable::const_iterator iter = self.find(name);
		if(iter == self.end())
			return(NULL);
		return(iter->second);
	}
}

static int didInit = 0;
void BuiltinSelector_init(void) {
	if(didInit)
		return;
	didInit = 1;
	add_static_builtin_binding(self, Symbols::Squote, &Evaluators::Quoter); /* keep at the beginning */
	add_static_builtin_binding(self, Symbols::SrequireModule, &ModuleLoader);
	add_static_builtin_binding(self, Symbols::Snil, NULL);
	add_builtin_method(self, Symbols::SdynamicBuiltin, &Evaluators::DynamicBuiltinGetter);
	add_builtin_method(self, Symbols::SfreeVariables, &Evaluators::FreeVariablesGetter);
	add_builtin_method(self, Symbols::Scolon, &Evaluators::Conser);
	add_builtin_method(self, Symbols::SconsP, &Evaluators::ConsP);
	add_builtin_method(self, Symbols::SpairP, &Evaluators::PairP);
	add_builtin_method(self, Symbols::SnilP, &Evaluators::NilP);
	add_builtin_method(self, Symbols::Shead, &Evaluators::HeadGetter);
	add_builtin_method(self, Symbols::Stail, &Evaluators::TailGetter);
	add_builtin_method(self, Symbols::Sfst, &Evaluators::FstGetter);
	add_builtin_method(self, Symbols::Ssnd, &Evaluators::SndGetter);
#ifdef STRICT_BUILTIN_SELECTOR 
	add_builtin_method(self, symbolFromStr(";"), &Evaluators::Sequencer);
	add_builtin_method(self, symbolFromStr("&&"), &Evaluators::Ander);
	add_builtin_method(self, symbolFromStr("||"), &Evaluators::Orer);
	add_builtin_method(self, symbolFromStr("not"), &Evaluators::Noter);
	add_builtin_method(self, symbolFromStr("if"), &Evaluators::Ifer);
	add_builtin_method(self, symbolFromStr("elif"), &Evaluators::Elifer);
	add_builtin_method(self, symbolFromStr("else"), &Evaluators::Elser);
#else
	add_builtin_method(self, symbolFromStr(";"), Evaluators::Sequencer);
	add_builtin_method(self, symbolFromStr("&&"), Evaluators::Ander);
	add_builtin_method(self, symbolFromStr("||"), Evaluators::Orer);
	add_builtin_method(self, symbolFromStr("not"), Evaluators::Noter);
	add_builtin_method(self, symbolFromStr("if"), Evaluators::Ifer);
	add_builtin_method(self, symbolFromStr("elif"), Evaluators::Elifer);
	add_builtin_method(self, symbolFromStr("else"), Evaluators::Elser);
#endif
	add_static_builtin_binding(self, Symbols::Shasht, Evaluators::aTrue);
	add_static_builtin_binding(self, Symbols::Shashf, Evaluators::aFalse);
	add_builtin_method(self, Symbols::SintP, &Numbers::IntP);
	add_static_builtin_binding(self, Symbols::Sintzero, Numbers::internNative((Numbers::NativeInt) 0));
	add_builtin_method(self, Symbols::SintSucc, &Numbers::IntSucc);
	add_builtin_method(self, Symbols::SintegerP, &Numbers::IntegerP);
	add_builtin_method(self, Symbols::SintegerSucc, &Numbers::IntegerSucc);
	add_builtin_method(self, Symbols::Splus, &Evaluators::Adder);
	add_builtin_method(self, Symbols::Sdash, &Evaluators::Subtractor);
	add_builtin_method(self, Symbols::Sasterisk, &Evaluators::Multiplicator);
	add_builtin_method(self, Symbols::Sslash, &Evaluators::Divider);
	add_builtin_method(self, Symbols::Sdivmod0, &Evaluators::QModulator2);
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
	add_static_builtin_binding(self, symbolFromStr("stdin"), makeBox(stdin, symbolFromStr("stdin")));
	add_static_builtin_binding(self, symbolFromStr("stdout"), makeBox(stdout, symbolFromStr("stdout")));
	add_static_builtin_binding(self, symbolFromStr("stderr"), makeBox(stderr, symbolFromStr("stderr")));
	add_static_builtin_binding(self, symbolFromStr("PATHMAX"), Numbers::internNative((Numbers::NativeInt) PATH_MAX));
	add_builtin_method(self, symbolFromStr("write!"), &Evaluators::wrapWrite);
	add_builtin_method(self, symbolFromStr("flush!"), &Evaluators::wrapFlush);
	add_builtin_method(self, symbolFromStr("readline!"), &Evaluators::wrapReadline);
	add_builtin_method(self, symbolFromStr("messageBox!"), &FFIs::MessageBoxDisplayer);
	add_builtin_method(self, symbolFromStr("requireSharedLibrary"), &FFIs::SharedLibraryLoader);
	add_builtin_method(self, symbolFromStr("absolutePath!"), &FFIs::AbsolutePathGetter);
	add_builtin_method(self, symbolFromStr("absolutePath?"), &FFIs::AbsolutePathP);
	add_builtin_method(self, symbolFromStr("packRecord"), &FFIs::RecordPacker);
	add_builtin_method(self, symbolFromStr("unpackRecord"), &FFIs::RecordUnpacker);
	add_builtin_method(self, symbolFromStr("recordSize"), &FFIs::RecordSizeCalculator);
	add_builtin_method(self, symbolFromStr("allocateMemory!"), &FFIs::MemoryAllocator);
	add_builtin_method(self, symbolFromStr("allocateRecord!"), &FFIs::RecordAllocator); // this is actually (compose allocateMemory recordSize)
	add_builtin_method(self, symbolFromStr("duplicateRecord!"), &FFIs::RecordDuplicator);
	add_builtin_method(self, symbolFromStr("archDepLibName"), &FFIs::ArchDepLibNameGetter);
	add_builtin_method(self, symbolFromStr("getErrno!"), &Evaluators::wrapGetErrno);
	add_builtin_method(self, symbolFromStr("environ!"), &FFIs::EnvironGetter);
	add_builtin_method(self, symbolFromStr("listFromEnviron"), &FFIs::EnvironInterner);
	add_builtin_method(self, symbolFromStr("environFromList"), &FFIs::EnvironFromList);
	add_builtin_method(self, symbolFromStr("makeApp"), &Evaluators::ApplicationMaker);
	add_builtin_method(self, symbolFromStr("app?"), &Evaluators::ApplicationP);
	add_builtin_method(self, symbolFromStr("appOperator"), &Evaluators::ApplicationOperatorGetter);
	add_builtin_method(self, symbolFromStr("appOperand"), &Evaluators::ApplicationOperandGetter);
	add_builtin_method(self, symbolFromStr("makeFn"), &Evaluators::AbstractionMaker);
	add_builtin_method(self, symbolFromStr("fn?"), &Evaluators::AbstractionP);
	add_builtin_method(self, symbolFromStr("fnParam"), &Evaluators::AbstractionParameterGetter);
	add_builtin_method(self, symbolFromStr("fnBody"), &Evaluators::AbstractionBodyGetter);
	add_builtin_method(self, symbolFromStr("makeRatio"), &Numbers::RatioMaker);
	add_builtin_method(self, symbolFromStr("ratio?"), &Numbers::RatioP);
	add_builtin_method(self, symbolFromStr("ratioNum"), &Numbers::RatioNumeratorGetter);
	add_builtin_method(self, symbolFromStr("ratioDenom"), &Numbers::RatioDenominatorGetter);
	add_builtin_method(self, symbolFromStr("parseMath!"), &Evaluators::FileMathParser);
	add_builtin_method(self, symbolFromStr("parseMathStr"), &Evaluators::StrMathParser);
	add_builtin_method(self, symbolFromStr("parseParenStr"), &Evaluators::StrParenParser);
	add_builtin_method(self, symbolFromStr("freeVariables"), &Evaluators::FreeVariablesGetter);
	add_builtin_method(self, symbolFromStr("dispatchModule"), &ModuleSystem::ModuleDispatcher);
	add_builtin_method(self, symbolFromStr("makeModuleBox"), &Evaluators::ModuleBoxMaker);
	//add_builtin_method(self, symbolFromStr("REPLMethods"), &REPLX::REPLMethodGetter);
	add_builtin_method(self, symbolFromStr(","), &Evaluators::Pairer);
	add_builtin_method(self, symbolFromStr("#exports"), &ModuleSystem::HashExporter);
	add_builtin_method(self, symbolFromStr("mkgmtime!"), &Evaluators::GmtimeMaker);
	add_builtin_method(self, symbolFromStr("mktime!"), &Evaluators::TimeMaker);
	add_builtin_method(self, symbolFromStr("infinite?"), &Evaluators::InfinityChecker);
	add_builtin_method(self, symbolFromStr("nan?"), &Evaluators::NanChecker);
#ifdef WIN32
	add_builtin_method(self, symbolFromStr("getWin32FindDataWSize!"), &Evaluators::Win32FindDataWSizeGetter);
	add_builtin_method(self, symbolFromStr("unpackWin32FindDataW!"), &Evaluators::Win32FindDataWUnpacker);
#else
	add_builtin_method(self, symbolFromStr("getDirentSize!"), &Evaluators::DirentSizeGetter);
	add_builtin_method(self, symbolFromStr("unpackDirent!"), &Evaluators::DirentUnpacker);
#endif
	add_builtin_method(self, symbolFromStr("makeScanner!"), &Scanners::makeScanner);
	add_builtin_method(self, symbolFromStr("makeMathParser!"), &Scanners::makeShuntingYardParser);
	add_builtin_method(self, symbolFromStr("eval"), &Evaluators::Reducer);
	add_builtin_method(self, symbolFromStr("printMath!"), &Formatters::Math::printMath);
	add_builtin_method(self, symbolFromStr("printLATEX!"), &Formatters::LATEX::printLATEX);
	add_builtin_method(self, symbolFromStr("printS!"), &Formatters::printS);
	// TODO quote?
	add_static_builtin_binding(self, symbolFromStr("nan"), Numbers::nan());
	add_static_builtin_binding(self, symbolFromStr("infinity"), Numbers::infinity());
	self["exports"] = consFromKeys(self.begin(), self.end()); // see Sexports
}


// FIXME error checking
DEFINE_SIMPLE_STRICT_OPERATION(BuiltinGetter, getBuiltinEntry(argument))
REGISTER_BUILTIN(BuiltinGetter, 1, 0, Symbols::SBuiltins)

};
