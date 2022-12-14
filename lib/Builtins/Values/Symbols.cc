#include <5D/Values>

namespace Symbols {
using namespace Values;

NodeT Spercenteax = symbolFromStr("%eax");
NodeT Sright = symbolFromStr("right");
NodeT SlistFromStr = symbolFromStr("listFromStr");
NodeT Slessequal = symbolFromStr("<=");
NodeT Sinline = symbolFromStr("inline");
NodeT Shead = symbolFromStr("head");
NodeT SREPLV1 = symbolFromStr("REPLV1");
NodeT Snot = symbolFromStr("¬");
NodeT Scircumflex = symbolFromStr("^");
NodeT SfloatP = symbolFromStr("float?");
NodeT SsymbolFromStr = symbolFromStr("symbolFromStr");
NodeT Sdivmod0 = symbolFromStr("divmod0");
NodeT Sin = symbolFromStr("in");
NodeT Sapply = symbolFromStr("apply");
NodeT Sgreater = symbolFromStr(">");
NodeT Sspace  = symbolFromStr(" ");
NodeT SstrP = symbolFromStr("str?");
NodeT Scrossproduct = symbolFromStr("⨯");
NodeT Squote = symbolFromStr("'");
NodeT Srightbracket = symbolFromStr("]");
NodeT Srightparen = symbolFromStr(")");
NodeT Sautorightparen = symbolFromStr("auto)");
NodeT SkeywordFromStr = symbolFromStr("keywordFromStr");
NodeT Splus = symbolFromStr("+");
NodeT Sasterisk = symbolFromStr("*");
NodeT Sdash = symbolFromStr("-");
NodeT SintegerP = symbolFromStr("integer?");
NodeT Sslash = symbolFromStr("/");
NodeT Senvironment = symbolFromStr("environment");
NodeT Szero = symbolFromStr("0");
NodeT Stail = symbolFromStr("tail");
NodeT SnilP = symbolFromStr("nil?");
NodeT Slesssymbolgreater = symbolFromStr("<symbol>");
NodeT Srec = symbolFromStr("rec");
NodeT Sapprox = symbolFromStr("≈");
NodeT Scolon = symbolFromStr(":");
NodeT SstrFromKeyword = symbolFromStr("strFromKeyword");
NodeT SstrFromList = symbolFromStr("strFromList");
NodeT Sless = symbolFromStr("<");
NodeT Stips5DV1 = symbolFromStr("tips5DV1");
NodeT Sdefine = symbolFromStr("define");
NodeT Sleftangle = symbolFromStr("⟨");
NodeT Srightangle = symbolFromStr("⟩");
NodeT SfromLibrary = symbolFromStr("fromLibrary");
NodeT Spercentecx = symbolFromStr("%ecx");
NodeT Snil = symbolFromStr("nil");
NodeT Sbackslash = symbolFromStr("\\");
NodeT StextBufferText = symbolFromStr("textBufferText");
NodeT SsymbolsEqualP = symbolFromStr("symbolsEqual?");
NodeT Shashf = symbolFromStr("#f");
NodeT SrunIO = symbolFromStr("runIO");
NodeT Spercentedx = symbolFromStr("%edx");
NodeT SintegerSucc = symbolFromStr("integerSucc");
NodeT Slet = symbolFromStr("let");
NodeT Sletexclam = symbolFromStr("let!");
NodeT Sintzero = symbolFromStr("int0");
NodeT Sasteriskasterisk = symbolFromStr("**");
NodeT Sleftbracket = symbolFromStr("[");
NodeT SaddrsLEP = symbolFromStr("addrsLE?");
NodeT SintP = symbolFromStr("int?");
NodeT Sdefrec = symbolFromStr("defrec");
NodeT SsymbolP = symbolFromStr("symbol?");
NodeT Sleftcurly = symbolFromStr("{");
NodeT Sunderscore = symbolFromStr("_");
NodeT Shasht = symbolFromStr("#t");
NodeT Snone = symbolFromStr("none");
NodeT Stilde = symbolFromStr("~");
NodeT StranslateFFI = symbolFromStr("translateFFI");
NodeT Slessstringgreater = symbolFromStr("<string>");
NodeT Sequal = symbolFromStr("=");
NodeT Slessequalunicode = symbolFromStr("≤");
NodeT Sgreaterequalunicode = symbolFromStr("≥");
NodeT SconsP = symbolFromStr("cons?");
NodeT Slessoperatorgreater = symbolFromStr("<operator>");
NodeT SlessEOFgreater = symbolFromStr("<EOF>");
NodeT Sslashequal = symbolFromStr("/=");
NodeT Sleftparen = symbolFromStr("(");
NodeT Sautoleftparen = symbolFromStr("auto(");
NodeT SkeywordP = symbolFromStr("keyword?");
NodeT SintSucc = symbolFromStr("intSucc");
NodeT SloadFromLibrary = symbolFromStr("loadFromLibrary");
NodeT Srightcurly = symbolFromStr("}");
NodeT Shello = symbolFromStr("hello");
NodeT Sdef = symbolFromStr("def");
NodeT Sleft = symbolFromStr("left");
NodeT Stab = symbolFromStr("tab");
NodeT Snewline = symbolFromStr("newline");
NodeT Sbackspace = symbolFromStr("backspace");
NodeT Sescape = symbolFromStr("escape");
NodeT Sinfo = symbolFromStr("info");
NodeT Simport = symbolFromStr("import");
NodeT Sdescribe = symbolFromStr("describe");
NodeT SrequireModule = symbolFromStr("requireModule");
NodeT Sunderline = symbolFromStr("_");
NodeT SmoduleKeys = symbolFromStr("moduleKeys");
NodeT Sexports = symbolFromStr("exports");
NodeT Sdispatch = symbolFromStr("dispatch");
NodeT Sprefix = symbolFromStr("prefix");
NodeT Spostfix = symbolFromStr("postfix");
NodeT Sdot = symbolFromStr(".");
NodeT Sunarydash = symbolFromStr("‒"); /* I'm so sorry */
NodeT Ssubstr = symbolFromStr("substr");
NodeT SstrUntilZero = symbolFromStr("strUntilZero");
NodeT Sintegral = symbolFromStr("∫");
NodeT Sroot = symbolFromStr("√");
NodeT Sa = symbolFromStr("a");
NodeT Sb = symbolFromStr("b");
NodeT SBuiltins = symbolFromStr("Builtins");
NodeT Scolonequal = symbolFromStr(":=");
NodeT SgetOperatorPrecedenceListexclam = symbolFromStr("getOperatorPrecedenceList!");
NodeT Sdescribeexclam = symbolFromStr("describe!");
NodeT Sdefineexcam = symbolFromStr("define!");
NodeT Simportexclam = symbolFromStr("import!");
NodeT Spurgeexclam = symbolFromStr("purge!");
NodeT Sexecuteexclam = symbolFromStr("execute!");
NodeT Sreturnexclam = symbolFromStr("return!");
NodeT Shashexports = symbolFromStr("#exports");
NodeT Scomma = symbolFromStr(",");
NodeT Sfst = symbolFromStr("fst");
NodeT Ssnd = symbolFromStr("snd");
NodeT SpairP = symbolFromStr("pair?");
NodeT Sfrom = symbolFromStr("from");
NodeT Shashexclam = symbolFromStr("#!");
NodeT SAX = symbolFromStr("AX");
NodeT SCX = symbolFromStr("CX");
NodeT SDX = symbolFromStr("DX");
NodeT SBX = symbolFromStr("BX");
NodeT SSP = symbolFromStr("SP");
NodeT SBP = symbolFromStr("BP");
NodeT SSI = symbolFromStr("SI");
NodeT SDI = symbolFromStr("DI");
NodeT SloadValReg = symbolFromStr("loadValReg");
NodeT SloadRegReg = symbolFromStr("loadRegReg");
NodeT SpopReg = symbolFromStr("popReg");
NodeT SpushReg = symbolFromStr("pushReg");
NodeT SpopRegs = symbolFromStr("popRegs");
NodeT SpushRegs = symbolFromStr("pushRegs");
NodeT SaddRegReg = symbolFromStr("addRegReg");
NodeT SaddValReg = symbolFromStr("addValReg");
NodeT SsubRegReg = symbolFromStr("subRegReg");
NodeT SsubValReg = symbolFromStr("subValReg");
NodeT SmulRegReg = symbolFromStr("mulRegReg");
NodeT SmulValRegReg = symbolFromStr("mulValRegReg");
NodeT SidivVal = symbolFromStr("idivVal");
NodeT Sret = symbolFromStr("ret");
NodeT SclearCarry = symbolFromStr("clearCarry");
NodeT SsetCarry = symbolFromStr("setCarry");
NodeT Sadc = symbolFromStr("adc");
NodeT Ssbb = symbolFromStr("sbb");
NodeT SR0 = symbolFromStr("R0");
NodeT SR1 = symbolFromStr("R1");
NodeT SR2 = symbolFromStr("R2");
NodeT SR3 = symbolFromStr("R3");
NodeT SR4 = symbolFromStr("R4");
NodeT SR5 = symbolFromStr("R5");
NodeT SR6 = symbolFromStr("R6");
NodeT SR7 = symbolFromStr("R7");
NodeT SR8 = symbolFromStr("R8");
NodeT SR9 = symbolFromStr("R9");
NodeT SSL = symbolFromStr("SL");
NodeT SFP = symbolFromStr("FP");
NodeT SIP = symbolFromStr("IP");
NodeT SLR = symbolFromStr("LR");
NodeT SPC = symbolFromStr("PC");
NodeT Ssemicolon = symbolFromStr(";");
NodeT SfreeVariables = symbolFromStr("freeVariables");
NodeT SdynamicBuiltin = symbolFromStr("dynamicBuiltin");
NodeT Swrap = symbolFromStr("wrap");
NodeT Stable = symbolFromStr("table");
NodeT Sfilename = symbolFromStr("filename");
void initSymbols2(void) {
	Spercenteax = symbolFromStr("%eax");
	Sright = symbolFromStr("right");
	SlistFromStr = symbolFromStr("listFromStr");
	Slessequal = symbolFromStr("<=");
	Sinline = symbolFromStr("inline");
	Shead = symbolFromStr("head");
	SREPLV1 = symbolFromStr("REPLV1");
	Snot = symbolFromStr("¬");
	Scircumflex = symbolFromStr("^");
	SfloatP = symbolFromStr("float?");
	SsymbolFromStr = symbolFromStr("symbolFromStr");
	Sdivmod0 = symbolFromStr("divmod0");
	Sin = symbolFromStr("in");
	Sapply = symbolFromStr("apply");
	Sgreater = symbolFromStr(">");
	Sspace  = symbolFromStr(" ");
	SstrP = symbolFromStr("str?");
	Scrossproduct = symbolFromStr("⨯");
	Squote = symbolFromStr("'");
	Srightbracket = symbolFromStr("]");
	Srightparen = symbolFromStr(")");
	Sautorightparen = symbolFromStr("auto)");
	SkeywordFromStr = symbolFromStr("keywordFromStr");
	Splus = symbolFromStr("+");
	Sasterisk = symbolFromStr("*");
	Sdash = symbolFromStr("-");
	SintegerP = symbolFromStr("integer?");
	Sslash = symbolFromStr("/");
	Senvironment = symbolFromStr("environment");
	Szero = symbolFromStr("0");
	Stail = symbolFromStr("tail");
	SnilP = symbolFromStr("nil?");
	Slesssymbolgreater = symbolFromStr("<symbol>");
	Srec = symbolFromStr("rec");
	Sapprox = symbolFromStr("≈");
	Scolon = symbolFromStr(":");
	SstrFromKeyword = symbolFromStr("strFromKeyword");
	SstrFromList = symbolFromStr("strFromList");
	Sless = symbolFromStr("<");
	Stips5DV1 = symbolFromStr("tips5DV1");
	Sdefine = symbolFromStr("define");
	Sleftangle = symbolFromStr("⟨");
	Srightangle = symbolFromStr("⟩");
	SfromLibrary = symbolFromStr("fromLibrary");
	Spercentecx = symbolFromStr("%ecx");
	Snil = symbolFromStr("nil");
	Sbackslash = symbolFromStr("\\");
	StextBufferText = symbolFromStr("textBufferText");
	SsymbolsEqualP = symbolFromStr("symbolsEqual?");
	Shashf = symbolFromStr("#f");
	SrunIO = symbolFromStr("runIO");
	Spercentedx = symbolFromStr("%edx");
	SintegerSucc = symbolFromStr("integerSucc");
	Slet = symbolFromStr("let");
	Sletexclam = symbolFromStr("let!");
	Sintzero = symbolFromStr("int0");
	Sasteriskasterisk = symbolFromStr("**");
	Sleftbracket = symbolFromStr("[");
	SaddrsLEP = symbolFromStr("addrsLE?");
	SintP = symbolFromStr("int?");
	Sdefrec = symbolFromStr("defrec");
	SsymbolP = symbolFromStr("symbol?");
	Sleftcurly = symbolFromStr("{");
	Sunderscore = symbolFromStr("_");
	Shasht = symbolFromStr("#t");
	Snone = symbolFromStr("none");
	Stilde = symbolFromStr("~");
	StranslateFFI = symbolFromStr("translateFFI");
	Slessstringgreater = symbolFromStr("<string>");
	Sequal = symbolFromStr("=");
	Slessequalunicode = symbolFromStr("≤");
	Sgreaterequalunicode = symbolFromStr("≥");
	SconsP = symbolFromStr("cons?");
	Slessoperatorgreater = symbolFromStr("<operator>");
	SlessEOFgreater = symbolFromStr("<EOF>");
	Sslashequal = symbolFromStr("/=");
	Sleftparen = symbolFromStr("(");
	Sautoleftparen = symbolFromStr("auto(");
	SkeywordP = symbolFromStr("keyword?");
	SintSucc = symbolFromStr("intSucc");
	SloadFromLibrary = symbolFromStr("loadFromLibrary");
	Srightcurly = symbolFromStr("}");
	Shello = symbolFromStr("hello");
	Sdef = symbolFromStr("def");
	Sleft = symbolFromStr("left");
	Stab = symbolFromStr("tab");
	Snewline = symbolFromStr("newline");
	Sbackspace = symbolFromStr("backspace");
	Sescape = symbolFromStr("escape");
	Sinfo = symbolFromStr("info");
	Simport = symbolFromStr("import");
	Sdescribe = symbolFromStr("describe");
	SrequireModule = symbolFromStr("requireModule");
	Sunderline = symbolFromStr("_");
	SmoduleKeys = symbolFromStr("moduleKeys");
	Sexports = symbolFromStr("exports");
	Sdispatch = symbolFromStr("dispatch");
	Sprefix = symbolFromStr("prefix");
	Spostfix = symbolFromStr("postfix");
	Sdot = symbolFromStr(".");
	Sunarydash = symbolFromStr("‒"); /* I'm so sorry */
	Ssubstr = symbolFromStr("substr");
	SstrUntilZero = symbolFromStr("strUntilZero");
	Sintegral = symbolFromStr("∫");
	Sroot = symbolFromStr("√");
	Sa = symbolFromStr("a");
	Sb = symbolFromStr("b");
	SBuiltins = symbolFromStr("Builtins");
	Scolonequal = symbolFromStr(":=");
	SgetOperatorPrecedenceListexclam = symbolFromStr("getOperatorPrecedenceList!");
	Sdescribeexclam = symbolFromStr("describe!");
	Sdefineexcam = symbolFromStr("define!");
	Simportexclam = symbolFromStr("import!");
	Spurgeexclam = symbolFromStr("purge!");
	Sexecuteexclam = symbolFromStr("execute!");
	Sreturnexclam = symbolFromStr("return!");
	Shashexports = symbolFromStr("#exports");
	Scomma = symbolFromStr(",");
	Sfst = symbolFromStr("fst");
	Ssnd = symbolFromStr("snd");
	SpairP = symbolFromStr("pair?");
	Sfrom = symbolFromStr("from");
	Shashexclam = symbolFromStr("#!");
	SAX = symbolFromStr("AX");
	SCX = symbolFromStr("CX");
	SDX = symbolFromStr("DX");
	SBX = symbolFromStr("BX");
	SSP = symbolFromStr("SP");
	SBP = symbolFromStr("BP");
	SSI = symbolFromStr("SI");
	SDI = symbolFromStr("DI");
	SloadValReg = symbolFromStr("loadValReg");
	SloadRegReg = symbolFromStr("loadRegReg");
	SpopReg = symbolFromStr("popReg");
	SpushReg = symbolFromStr("pushReg");
	SpopRegs = symbolFromStr("popRegs");
	SpushRegs = symbolFromStr("pushRegs");
	SaddRegReg = symbolFromStr("addRegReg");
	SaddValReg = symbolFromStr("addValReg");
	SsubRegReg = symbolFromStr("subRegReg");
	SsubValReg = symbolFromStr("subValReg");
	SmulRegReg = symbolFromStr("mulRegReg");
	SmulValRegReg = symbolFromStr("mulValRegReg");
	SidivVal = symbolFromStr("idivVal");
	Sret = symbolFromStr("ret");
	SclearCarry = symbolFromStr("clearCarry");
	SsetCarry = symbolFromStr("setCarry");
	Sadc = symbolFromStr("adc");
	Ssbb = symbolFromStr("sbb");
	SR0 = symbolFromStr("R0");
	SR1 = symbolFromStr("R1");
	SR2 = symbolFromStr("R2");
	SR3 = symbolFromStr("R3");
	SR4 = symbolFromStr("R4");
	SR5 = symbolFromStr("R5");
	SR6 = symbolFromStr("R6");
	SR7 = symbolFromStr("R7");
	SR8 = symbolFromStr("R8");
	SR9 = symbolFromStr("R9");
	SSL = symbolFromStr("SL");
	SFP = symbolFromStr("FP");
	SIP = symbolFromStr("IP");
	SLR = symbolFromStr("LR");
	SPC = symbolFromStr("PC");
	Ssemicolon = symbolFromStr(";");
	SfreeVariables = symbolFromStr("freeVariables");
	SdynamicBuiltin = symbolFromStr("dynamicBuiltin");
	Swrap = symbolFromStr("wrap");
	Stable = symbolFromStr("table");
	Sfilename = symbolFromStr("filename");
}

};
