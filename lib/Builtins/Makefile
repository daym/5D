
OBJECTS = ./FFIs/TrampolineSymbols.o \
FFIs/Allocators.o \
FFIs/Trampolines.o \
FFIs/POSIX.o \
FFIs/POSIXProcessInfos.o \
FFIs/RecordPacker.o \
Evaluators/BuiltinSelector.o \
Evaluators/Builtins.o \
Evaluators/ModuleLoader.o \
Evaluators/Logic.o \
Evaluators/Evaluators.o \
Evaluators/FFI.o \
Evaluators/Backtracker.o \
Evaluators/Operation.o \
Numbers/Integer.o \
Numbers/Real.o \
Numbers/BigUnsigned.o \
Numbers/Ratio.o \
Formatters/Math.o \
Formatters/UTFStateMachine.o \
Formatters/SExpression.o \
Formatters/LATEX.o \
ModuleSystem/Exports.o \
ModuleSystem/Modules.o \
Scanners/LOperatorPrecedenceList.o \
Scanners/MathParser.o \
Scanners/SExpressionParser.o \
Scanners/Scanner.o \
Scanners/ShuntingYardParser.o \
Values/Values.o \
Values/Symbols.o \
Values/HashTable.o \
Values/Keyword.o \
Values/Symbol.o \
BuiltinsModule.o

LIBGC_LD_WRAP_CFLAGS = -D_REENTRANT -DGC_THREADS -DUSE_LD_WRAP
#CXXFLAGS += -fPIC -O3 -Wall -I. -fno-strict-overflow -Wno-deprecated -Wno-div-by-zero `pkg-config --cflags glib-2.0 gthread-2.0 libxml-2.0 libffi` $(LIBGC_LD_WRAP_CFLAGS) -D_FILE_OFFSET_BITS=64 -Wno-unused-function -Iinclude
CXXFLAGS += -fPIC -O1 -g3 -Wall -I. -fno-strict-overflow -Wno-deprecated -Wno-div-by-zero `pkg-config --cflags glib-2.0 gthread-2.0 libxml-2.0 libffi` $(LIBGC_LD_WRAP_CFLAGS) -D_FILE_OFFSET_BITS=64 -Wno-unused-function -Iinclude
ifdef PREFIX
CXXFLAGS +=  -DPREFIX=\"$(PREFIX)\"
endif

LIB = lib5dbuiltins.so.1
STATIC_LIB = lib5dbuiltins1.a
.SUFFIXES:            # Delete the default suffixes
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

all: $(LIB) $(STATIC_LIB)
$(LIB): $(OBJECTS)
	$(CXX) -shared -o $(LIB) $(OBJECTS) $(LDFLAGS)
$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $(STATIC_LIB) $(OBJECTS)
#./FFIs/VariantPacker.o \
#FFIs/COMWrapper.o \
#FFIs/WIN32.o \
#FFIs/WIN32UI.o \
#FFIs/WIN32ProcessInfos.o \
#Compilers/ARM/CodeGen.o \
#Compilers/CallGen.o \
#Compilers/X86/BorlandFastCallGen.o \
#Compilers/X86/PascalCallGen.o \
#Compilers/X86/CDECLCallGen.o \
#Compilers/X86/CodeGen.o \
#Compilers/X86/StdCallGen.o \
#Compilers/CodeGen.o \
#Scanners/ParenAdder.o \

# FIXME Evaluators/StrictBuiltins.o \
# FIXME Evaluators/StrictBuiltinSelector.o \
# FIXME Evaluators/StrictEvaluators.o \
# FIXME Evaluators/StrictLogic.o \

clean:
	rm -f $(OBJECTS)
distclean: clean
	rm -f $(LIB) $(STATIC_LIB)
Formatters/UTF-8_to_LATEX_result.h: Formatters/UTF-8_to_LATEX.table Formatters/generate-state-machine
	Formatters/generate-state-machine Formatters/UTF-8_to_LATEX.table >tmp4711 && mv tmp4711 Formatters/UTF-8_to_LATEX_result.h
FFIs/TrampolineSymbols.o: FFIs/TrampolineSymbols.cc Values/Values Numbers/BigUnsigned
FFIs/WIN32ProcessInfos.o: FFIs/WIN32ProcessInfos.cc Numbers/BigUnsigned Numbers/Real FFIs/ProcessInfos
FFIs/POSIXProcessInfos.o: FFIs/POSIXProcessInfos.cc Numbers/Small Numbers/Integer Numbers/BigUnsigned Numbers/Real Evaluators/FFI Numbers/Small Numbers/BigUnsigned Evaluators/Builtins Numbers/Real
Evaluators/StrictLogic.o: Evaluators/StrictLogic.cc Values/Values Values/Keyword Values/HashTable Scanners/MathParser Scanners/Scanner Values/Symbols Scanners/ShuntingYardParser FFIs/FFIs Formatters/Math
Evaluators/StrictBuiltins.o: Evaluators/StrictBuiltins.cc Evaluators/Builtins.cc Values/Keyword Values/HashTable Scanners/MathParser Scanners/Scanner Values/Symbols Scanners/ShuntingYardParser FFIs/FFIs Formatters/Math
Evaluators/BuiltinSelector.o: Evaluators/BuiltinSelector.cc Evaluators/BuiltinSelector include/5D/Operations FFIs/RecordPacker FFIs/UI FFIs/FFIs FFIs/ProcessInfos Numbers/Ratio Evaluators/Logic Scanners/Scanner Scanners/MathParser Formatters/SExpression Formatters/LATEX Formatters/Math
Evaluators/StrictBuiltinSelector.o: Evaluators/StrictBuiltinSelector.cc Evaluators/BuiltinSelector include/5D/Operations FFIs/RecordPacker FFIs/UI FFIs/FFIs FFIs/ProcessInfos Numbers/Ratio Evaluators/Logic Numbers/Small Numbers/Integer Numbers/BigUnsigned Values/Symbols FFIs/FFIs Evaluators/FFI Scanners/MathParser Scanners/Scanner Scanners/ShuntingYardParser FFIs/ProcessInfos Evaluators/BuiltinSelector Numbers/BigUnsigned Numbers/Real Values/Symbols Scanners/MathParser Scanners/Scanner Scanners/ShuntingYardParser
Evaluators/Backtracker.o: Evaluators/Backtracker.cc Evaluators/Backtracker Evaluators/Builtins Numbers/Small Numbers/Integer Numbers/BigUnsigned FFIs/TrampolineSymbols Evaluators/FFI
Evaluators/StrictEvaluators.o: Evaluators/StrictEvaluators.cc Numbers/BigUnsigned Numbers/Real Values/Symbols Scanners/MathParser Scanners/Scanner Scanners/ShuntingYardParser
Evaluators/Operation.o: Evaluators/Operation.cc include/5D/Operations Evaluators/Evaluators Values/Values Values/Symbol Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/MathParser FFIs/Trampolines FFIs/RecordPacker Numbers/Ratio
	$(CC) -fPIC -O2 -Wall -I. -fno-strict-overflow -c -o $@ $< -Iinclude 
Numbers/BigUnsigned.o: Numbers/BigUnsigned.cc Evaluators/Evaluators Values/Keyword Values/HashTable Values/Symbols Evaluators/Builtins Numbers/Small Numbers/Integer Formatters/GenericPrinter Formatters/UTFStateMachine Formatters/UTF-8_to_LATEX_result.h Evaluators/Builtins Numbers/Small Numbers/Integer Numbers/BigUnsigned Formatters/UTFStateMachine Numbers/Ratio Formatters/GenericPrinter Evaluators/FFI Evaluators/Evaluators Evaluators/FFI Evaluators/Evaluators Evaluators/FFI Evaluators/Evaluators 
Scanners/LOperatorPrecedenceList.o: Scanners/LOperatorPrecedenceList.cc Values/Keyword Values/HashTable Scanners/LOperatorPrecedenceList include/5D/Operations
Scanners/MathParser.o: Scanners/MathParser.cc
Scanners/test-Scanner.o: Scanners/test-Scanner.cc Values/Values Values/Keyword Values/HashTable Values/Symbols Evaluators/Builtins Numbers/Small Numbers/Integer Numbers/BigUnsigned include/5D/Operations Numbers/Real Evaluators/Builtins Numbers/Small Numbers/Integer Numbers/BigUnsigned Numbers/Real Values/Keyword Values/HashTable Evaluators/Builtins Numbers/Small include/5D/Operations Numbers/Real
Values/HashTable.o: Values/HashTable.cc Values/Values 
Scanners/Scanner.o: Scanners/Scanner.cc Scanners/Scanner include/5D/Operations
Formatters/Math.o: Formatters/Math.cc Formatters/Math Formatters/GenericPrinter Scanners/LOperatorPrecedenceList
Formatters/LATEX.o: Formatters/LATEX.cc Formatters/LATEX Formatters/GenericPrinter Scanners/LOperatorPrecedenceList

ModuleSystem/Exports.o: ModuleSystem/Exports.cc include/5D/Values Values/Values Values/Symbols
ModuleSystem/Modules.o: ModuleSystem/Modules.cc ModuleSystem/Modules Evaluators/Evaluators Values/Symbols Evaluators/Builtins Evaluators/BuiltinSelector

FFIs/POSIX.o: FFIs/POSIX.cc FFIs/FFIs

Values/test-Values: Values/test-Values.o $(BUILTINS_LIB)
	g++ -o Values/test-Values Values/test-Values.o $(LDFLAGS)

Values/test-Symbol: Values/test-Symbol.o Values/Symbol.o Values/HashTable.o Values/Symbols.o Values/Values.o $(AST_UNCLEAN) $(BUILTINS_LIB)
	g++ -o Values/test-Symbol Values/test-Symbol.o Values/HashTable.o Values/Symbol.o Values/Symbols.o Values/Values.o $(AST_UNCLEAN) $(LDFLAGS)

Values/test-Keyword: Values/test-Keyword.o Values/Keyword.o Values/HashTable.o Values/Symbols.o Values/Symbol.o Values/Values.o $(AST_UNCLEAN) $(BUILTINS_LIB)
	g++ -o Values/test-Keyword Values/test-Keyword.o Values/HashTable.o Values/Keyword.o Values/Symbols.o Values/Symbol.o Values/Values.o $(AST_UNCLEAN) $(LDFLAGS)

FFIs/Combinations: FFIs/generateCombinations Makefile
	# put "PC" here for Windows.
	FFIs/generateCombinations "" > FFIs/Combinations.new && mv FFIs/Combinations.new FFIs/Combinations

FFIs/Trampolines: FFIs/generateTrampolines FFIs/Combinations FFIs/TrampolineSymbols
	FFIs/generateTrampolines < FFIs/Combinations > FFIs/Trampolines.new && mv FFIs/Trampolines.new FFIs/Trampolines

FFIs/TrampolineSymbols.cc: FFIs/generateTrampolineSymbols FFIs/Combinations
	FFIs/generateTrampolineSymbols < FFIs/Combinations > FFIs/TrampolineSymbols.cc.new && mv FFIs/TrampolineSymbols.cc.new FFIs/TrampolineSymbols.cc

FFIs/TrampolineSymbols: FFIs/TrampolineSymbols.cc Makefile FFIs/generateTrampolineSymbols
	(echo "#ifndef __TRAMPOLINE_SYMBOLS_H" ; echo "#define __TRAMPOLINE_SYMBOLS_H" ; cat FFIs/TrampolineSymbols.cc | sed 's@^\(.*\) = .*$$@extern \1;@' ; echo "#endif")  > FFIs/TrampolineSymbols.new && mv FFIs/TrampolineSymbols.new FFIs/TrampolineSymbols
	
BuiltinsModule.o: BuiltinsModule.cc include/5D/Operations include/5D/Values

install:
	install -m 755 -d $(DESTDIR)/usr/share/5D/Builtins
	install -m 755 $(LIB) $(DESTDIR)/usr/share/5D/Builtins/$(LIB)
	install -m 755 $(STATIC_LIB) $(DESTDIR)/usr/share/5D/Builtins/$(STATIC_LIB)