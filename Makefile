
SUBDIRS = Config REPL Linear_Algebra Bugs WIN32 Tests FFIs Bootstrappers Compilers AST Evaluators TUI Numbers Formatters doc Runtime debian GUI Scanners
SUBDIRS2 = $(SUBDIRS) doc/building doc/installation doc/interna doc/library doc/programming doc/programming/manual doc/programming/tutorial Tests/0* Runtime/Arithmetic Runtime/Logic Runtime/Composition Runtime/List Runtime/OS Runtime/IO Runtime/UI Runtime/FFI Runtime/String Runtime/Reflection Runtime/Error Examples
EXECUTABLES = REPL/5DREPL GUI/5D TUI/TUI Linear_Algebra/test-Matrix Linear_Algebra/test-Vector Linear_Algebra/test-Tensor AST/test-AST AST/test-Symbol Scanners/test-MathParser Scanners/test-Scanner GUI/5D REPL/5DREPL TUI2/5DTUI
GENERATEDS = FFIs/Trampolines FFIs/TrampolineSymbols.cc FFIs/TrampolineSymbols FFIs/Combinations

# -O3 is for tail-call optimization

#LIBGC_LD_WRAP_LDFLAGS = -Wl,--wrap -Wl,read -Wl,--wrap -Wl,dlopen -Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join -Wl,--wrap -Wl,pthread_detach -Wl,--wrap -Wl,pthread_sigmask -Wl,--wrap -Wl,sleep
LIBGC_LD_WRAP_CFLAGS = -D_REENTRANT -DGC_THREADS -DUSE_LD_WRAP
CXXFLAGS += -Wall -I. -g3 -fno-strict-overflow -Wno-deprecated `pkg-config --cflags glib-2.0 gthread-2.0 libxml-2.0 libffi` $(LIBGC_LD_WRAP_CFLAGS) -pthread -D_FILE_OFFSET_BITS=64 -Wno-unused-function
ifdef PREFIX
CXXFLAGS +=  -DPREFIX=\"$(PREFIX)\"
endif
PACKAGE = 5D
VERSION = $(shell head -1 debian/changelog |cut -d"(" -f2 |cut -d")" -f1)
DISTDIR = $(PACKAGE)-$(VERSION)

#-fwrapv
#-Werror=strict-overflow

LDFLAGS += /usr/lib/libreadline.a /usr/lib/libtinfo.a -ldl -lgc -lpthread `pkg-config --libs glib-2.0 gthread-2.0 libxml-2.0 libffi` $(LIBGC_LD_WRAP) -lffi
GUI_CXXFLAGS = $(CXXFLAGS) `pkg-config --cflags gtk+-2.0`
GUI_LDFLAGS = $(LDFLAGS) `pkg-config --libs gtk+-2.0`
FFIS = FFIs/TrampolineSymbols.o FFIs/Allocators.o

NUMBER_OBJECTS = Numbers/Integer.o Numbers/Real.o Numbers/BigUnsigned.o

.SUFFIXES:            # Delete the default suffixes
%.o: %.cc
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<


TARGETS = REPL/5DREPL TUI/TUI

TARGETS += $(shell pkg-config --cflags --libs gtk+-2.0 2>/dev/null |grep -q -- -  && echo GUI/5D )

all: $(TARGETS)
	$(MAKE) -C Runtime all

Linear_Algebra/test-Vector: Linear_Algebra/test-Vector.o
	g++ -o Linear_Algebra/test-Vector Linear_Algebra/test-Vector.o

Linear_Algebra/test-Matrix: Linear_Algebra/test-Matrix.o
	g++ -o Linear_Algebra/test-Matrix Linear_Algebra/test-Matrix.o

Linear_Algebra/test-Tensor: Linear_Algebra/test-Tensor.o
	g++ -o Linear_Algebra/test-Tensor Linear_Algebra/test-Tensor.o

Scanners/test-Scanner: Scanners/test-Scanner.o Scanners/Scanner.o AST/Symbol.o AST/HashTable.o AST/Symbols.o AST/AST.o AST/Keyword.o $(FFIS)
	g++ -o $@ $^ $(LDFLAGS)

Scanners/test-MathParser: Scanners/test-MathParser.o Scanners/MathParser.o Scanners/Scanner.o AST/Symbol.o AST/HashTable.o AST/Symbols.o AST/AST.o Scanners/OperatorPrecedenceList.o AST/Keyword.o Evaluators/Builtins.o Evaluators/Evaluators.o Numbers/Integer.o Numbers/BigUnsigned.o Numbers/Real.o Evaluators/Operation.o FFIs/Trampolines.o Evaluators/FFI.o $(FFIS) Scanners/ShuntingYardParser.o
	g++ -o $@ $^ $(LDFLAGS)
	
AST/test-AST: AST/test-AST.o
	g++ -o AST/test-AST AST/test-AST.o $(LDFLAGS)

AST/test-Symbol: AST/test-Symbol.o AST/Symbol.o AST/HashTable.o AST/Symbols.o AST/AST.o $(AST_UNCLEAN)
	g++ -o AST/test-Symbol AST/test-Symbol.o AST/HashTable.o AST/Symbol.o AST/Symbols.o AST/AST.o $(AST_UNCLEAN) $(LDFLAGS)

AST/test-Keyword: AST/test-Keyword.o AST/Keyword.o AST/HashTable.o AST/Symbols.o AST/Symbol.o AST/AST.o $(AST_UNCLEAN)
	g++ -o AST/test-Keyword AST/test-Keyword.o AST/HashTable.o AST/Keyword.o AST/Symbols.o AST/Symbol.o AST/AST.o $(AST_UNCLEAN) $(LDFLAGS)

test-AST: test-AST.o
	g++ -o test-AST test-AST.o
	
Linear_Algebra/test-Vector.o: Linear_Algebra/test-Vector.cc Linear_Algebra/Vector
Linear_Algebra/test-Matrix.o: Linear_Algebra/test-Matrix.cc Linear_Algebra/Matrix
Linear_Algebra/test-Tensor.o: Linear_Algebra/test-Tensor.cc Linear_Algebra/Tensor
Formatters/LATEX.o: Formatters/LATEX.cc Formatters/LATEX AST/AST AST/Symbol AST/Symbols Scanners/MathParser Formatters/UTFStateMachine Scanners/OperatorPrecedenceList Evaluators/Builtins Numbers/Integer Numbers/Real Formatters/GenericPrinter
Formatters/SExpression.o: Formatters/SExpression.cc Formatters/SExpression AST/Symbol AST/AST Evaluators/Builtins Numbers/Integer Numbers/Real
Formatters/Math.o: Formatters/Math.cc Formatters/Math AST/Symbol AST/AST Evaluators/Builtins Numbers/Integer Numbers/Real Formatters/GenericPrinter Scanners/OperatorPrecedenceList
Formatters/UTFStateMachine.o: Formatters/UTFStateMachine.cc Formatters/UTFStateMachine Formatters/UTF-8_to_LATEX_result.h
Formatters/UTF-8_to_LATEX_result.h: Formatters/UTF-8_to_LATEX.table Formatters/generate-state-machine
	Formatters/generate-state-machine Formatters/UTF-8_to_LATEX.table >tmp4711 && mv tmp4711 Formatters/UTF-8_to_LATEX_result.h
AST/AST.o: AST/AST.cc AST/AST AST/Symbol
AST/test-AST.o: AST/test-AST.cc AST/AST
AST/test-Symbol.o: AST/test-Symbol.cc FFIs/Allocators AST/Symbol AST/AST AST/HashTable
AST/test-Keyword.o: AST/test-Keyword.cc FFIs/Allocators AST/Keyword AST/AST AST/HashTable
AST/Symbol.o: AST/Symbol.cc AST/Symbol AST/AST AST/HashTable FFIs/Allocators
AST/Symbols.o: AST/Symbols.cc AST/Symbols AST/Symbol AST/AST
AST/Keyword.o: AST/Keyword.cc AST/Keyword AST/HashTable AST/AST FFIs/Allocators
AST/HashTable.o: AST/HashTable.cc AST/HashTable AST/AST AST/Symbol
Scanners/Scanner.o: Scanners/Scanner.cc Scanners/Scanner FFIs/Allocators AST/Symbol AST/AST AST/Keyword Evaluators/Builtins Numbers/Integer Numbers/Real Evaluators/Evaluators AST/AST
Scanners/test-Scanner.o: Scanners/test-Scanner.cc Scanners/Scanner AST/Symbol AST/AST
Scanners/MathParser.o: Scanners/MathParser.cc Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol Scanners/OperatorPrecedenceList Evaluators/Builtins Evaluators/Evaluators
Scanners/ShuntingYardParser.o: Scanners/ShuntingYardParser.cc Scanners/ShuntingYardParser Scanners/Scanner AST/AST AST/Symbol Scanners/OperatorPrecedenceList Evaluators/Builtins Evaluators/Evaluators AST/Symbols
Scanners/SExpressionParser.o: Scanners/SExpressionParser.cc Scanners/SExpressionParser Scanners/Scanner AST/AST AST/Symbol Scanners/OperatorPrecedenceList Evaluators/Builtins Evaluators/Evaluators
Scanners/OperatorPrecedenceList.o: Scanners/OperatorPrecedenceList.cc AST/AST AST/Symbol Evaluators/Operation
Scanners/test-MathParser.o: Scanners/test-MathParser.cc Scanners/MathParser Scanners/Scanner Scanners/OperatorPrecedenceList Evaluators/Builtins
Evaluators/Evaluators.o: Evaluators/Evaluators.cc FFIs/Allocators Evaluators/Evaluators Evaluators/Operation AST/AST AST/Symbol Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/MathParser  Scanners/OperatorPrecedenceList
FFIs/Trampolines.o: FFIs/Trampolines.cc FFIs/RecordPacker Evaluators/Evaluators Numbers/Integer Numbers/Small
Evaluators/Operation.o: Evaluators/Operation.cc Evaluators/Operation Evaluators/Evaluators AST/AST AST/Symbol Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/MathParser  Scanners/OperatorPrecedenceList FFIs/Trampolines FFIs/RecordPacker
	$(CC) -O2 -Wall -I. -fno-strict-overflow -c -o $@ $< 
Evaluators/Builtins.o: Evaluators/Builtins.cc FFIs/Allocators AST/HashTable Scanners/MathParser Evaluators/Builtins Numbers/Integer Numbers/Real AST/AST AST/Symbol AST/Keyword FFIs/FFIs  Scanners/OperatorPrecedenceList Numbers/Small Evaluators/Operation
Evaluators/Backtracker.o: Evaluators/Backtracker.cc Evaluators/Backtracker
Evaluators/FFI.o: Evaluators/FFI.cc FFIs/Allocators Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real Evaluators/Operation
Evaluators/ModuleLoader.o: Evaluators/ModuleLoader.cc Evaluators/ModuleLoader Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real Evaluators/Operation FFIs/FFIs Scanners/MathParser Scanners/ShuntingYardParser Scanners/Scanner FFIs/Allocators FFIs/ProcessInfos
FFIs/POSIX.o: FFIs/POSIX.cc Evaluators/Builtins FFIs/FFIs Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators Numbers/Integer Numbers/Real Evaluators/Operation
Config/GTKConfig.o: Config/GTKConfig.cc Config/Config FFIs/Allocators AST/AST
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

test: Linear_Algebra/test-Vector Linear_Algebra/test-Matrix Linear_Algebra/test-Tensor AST/test-AST AST/test-Symbol AST/test-Keyword Scanners/test-Scanner Scanners/test-MathParser
	./Linear_Algebra/test-Vector
	./Linear_Algebra/test-Matrix
	./Linear_Algebra/test-Tensor
	./AST/test-Symbol
	./AST/test-Keyword
	./AST/test-AST
	./Scanners/test-Scanner
	./Scanners/test-MathParser

REPL/5DREPL: REPL/main.o REPL/REPL.o Scanners/ShuntingYardParser.o Scanners/SExpressionParser.o AST/AST.o AST/Symbol.o AST/HashTable.o AST/Symbols.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o Evaluators/Backtracker.o AST/Keyword.o Formatters/SExpression.o Formatters/Math.o Scanners/OperatorPrecedenceList.o $(NUMBER_OBJECTS) Evaluators/Operation.o FFIs/Trampolines.o FFIs/TUI.o FFIs/RecordPacker.o REPL/BuiltinSelector.o FFIs/POSIXProcessInfos.o $(FFIS) Evaluators/ModuleLoader.o
	g++ -o $@ $^ $(LDFLAGS)

TUI/TUI: TUI/main.o Scanners/ShuntingYardParser.o Scanners/SExpressionParser.o AST/AST.o AST/Symbol.o AST/HashTable.o AST/Symbols.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o Evaluators/Backtracker.o AST/Keyword.o Formatters/SExpression.o Formatters/Math.o Scanners/OperatorPrecedenceList.o TUI/Interrupt.o REPL/REPL.o $(NUMBER_OBJECTS) Evaluators/Operation.o FFIs/Trampolines.o FFIs/TUI.o FFIs/RecordPacker.o REPL/BuiltinSelector.o FFIs/POSIXProcessInfos.o $(FFIS) Evaluators/ModuleLoader.o
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

TUI2/5DTUI: TUI2/main.o Scanners/ShuntingYardParser.o Scanners/SExpressionParser.o AST/AST.o AST/Symbol.o AST/HashTable.o AST/Symbols.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o Evaluators/Backtracker.o AST/Keyword.o Formatters/SExpression.o Formatters/Math.o Scanners/OperatorPrecedenceList.o TUI/Interrupt.o $(NUMBER_OBJECTS) Evaluators/Operation.o FFIs/Trampolines.o REPL/BuiltinSelector.o FFIs/POSIXProcessInfos.o $(FFIS) Evaluators/ModuleLoader.o
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

REPL/main.o: REPL/main.cc REPL/REPL Version FFIs/Allocators REPL/REPLEnvironment Evaluators/ModuleLoader FFIs/FFIs AST/AST AST/Symbol Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators Scanners/MathParser Scanners/SExpressionParser Scanners/Scanner  Scanners/OperatorPrecedenceList Evaluators/Builtins Scanners/MathParser Scanners/Scanner Evaluators/Evaluators Numbers/Small Evaluators/Operation Numbers/Integer Numbers/Real FFIs/ProcessInfos Evaluators/ModuleLoader
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI/main.o: TUI/main.cc FFIs/Allocators REPL/REPLEnvironment Version Evaluators/ModuleLoader FFIs/FFIs AST/AST AST/Symbol Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators Scanners/MathParser Scanners/ShuntingYardParser Scanners/SExpressionParser Scanners/Scanner  Scanners/OperatorPrecedenceList Evaluators/Builtins Numbers/Integer Numbers/Real TUI/Interrupt Config/Config Evaluators/Evaluators Evaluators/Builtins FFIs/ProcessInfos Evaluators/ModuleLoader
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI2/main.o: TUI2/main.cc REPL/REPLEnvironment Version Evaluators/ModuleLoader FFIs/FFIs AST/AST AST/Symbol Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators Scanners/MathParser Scanners/ShuntingYardParser Scanners/SExpressionParser  Scanners/Scanner  Scanners/OperatorPrecedenceList Evaluators/Builtins Numbers/Integer Numbers/Real TUI/Interrupt Config/Config Evaluators/Evaluators Evaluators/Builtins FFIs/ProcessInfos Evaluators/ModuleLoader
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/RecordPacker.o: FFIs/RecordPacker.cc FFIs/RecordPacker FFIs/FFIs AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real 
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/TUI.o: FFIs/TUI.cc FFIs/UI FFIs/FFIs AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real 
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/GTKUI.o: FFIs/GTKUI.cc FFIs/UI FFIs/FFIs AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real 
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/Allocators.o: FFIs/Allocators.c FFIs/Allocators AST/AST
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
FFIs/TrampolineSymbols.o: FFIs/TrampolineSymbols.cc FFIs/TrampolineSymbols AST/Symbol
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/Combinations: FFIs/generateCombinations Makefile
	FFIs/generateCombinations C > FFIs/Combinations.new && mv FFIs/Combinations.new FFIs/Combinations

FFIs/Trampolines: FFIs/generateTrampolines FFIs/Combinations FFIs/TrampolineSymbols
	FFIs/generateTrampolines < FFIs/Combinations > FFIs/Trampolines.new && mv FFIs/Trampolines.new FFIs/Trampolines

FFIs/TrampolineSymbols.cc: FFIs/generateTrampolineSymbols FFIs/Combinations
	FFIs/generateTrampolineSymbols < FFIs/Combinations > FFIs/TrampolineSymbols.cc.new && mv FFIs/TrampolineSymbols.cc.new FFIs/TrampolineSymbols.cc

FFIs/TrampolineSymbols: FFIs/TrampolineSymbols.cc Makefile FFIs/generateTrampolineSymbols
	(echo "#ifndef __TRAMPOLINE_SYMBOLS_H" ; echo "#define __TRAMPOLINE_SYMBOLS_H" ; cat FFIs/TrampolineSymbols.cc | sed 's@^\(.*\) = .*$$@extern \1;@' ; echo "#endif")  > FFIs/TrampolineSymbols.new && mv FFIs/TrampolineSymbols.new FFIs/TrampolineSymbols
	
TUI/Interrupt.o: TUI/Interrupt.cc TUI/Interrupt
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKGUI.o: GUI/GTKGUI.cc Version GUI/GTKREPL GUI/GTKView REPL/REPL FFIs/Allocators Evaluators/ModuleLoader
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKREPL.o: GUI/GTKREPL.cc Scanners/MathParser Version Evaluators/Builtins FFIs/Allocators Scanners/Scanner AST/AST AST/Symbol Config/Config Formatters/LATEX GUI/UI_definition.UI GUI/GTKLATEXGenerator Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators REPL/REPL GUI/CommonCompleter GUI/GTKCompleter REPL/REPLEnvironment FFIs/ProcessInfos GUI/WindowIcon Scanners/OperatorPrecedenceList  Numbers/Small Evaluators/Operation Numbers/Integer Numbers/Real FFIs/ProcessInfos Evaluators/ModuleLoader
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKCompleter.o: GUI/GTKCompleter.cc GUI/CommonCompleter GUI/GTKCompleter FFIs/Allocators AST/AST AST/Symbol Scanners/MathParser Scanners/OperatorPrecedenceList
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

REPL/REPL.o: REPL/REPL.cc REPL/REPL AST/AST AST/Symbol Scanners/Scanner Scanners/MathParser Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/OperatorPrecedenceList Scanners/SExpressionParser Scanners/ShuntingYardParser
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKLATEXGenerator.o: GUI/GTKLATEXGenerator.cc GUI/GTKLATEXGenerator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
GUI/GTKView.o: GUI/GTKView.cc
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

REPL/BuiltinSelectors.o: REPL/BuiltinSelector.cc REPL/BuiltinSelector AST/AST AST/HashTable AST/Symbol AST/Symbols Evaluators/Builtins Evaluators/FFI FFIs/RecordPacker FFIs/UI FFIs/FFIs Evaluators/Evaluators Numbers/Integer Numbers/Small FFIs/ProcessInfos
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

FFIs/POSIXProcessInfos.o: FFIs/POSIXProcessInfos.cc FFIs/ProcessInfos AST/AST Evaluators/Evaluators AST/HashTable AST/Symbol Evaluators/FFI Evaluators/Builtins FFIs/Allocators
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/5D: GUI/GTKGUI.o GUI/GTKREPL.o Scanners/MathParser.o Scanners/ShuntingYardParser.o Scanners/SExpressionParser.o Scanners/Scanner.o AST/AST.o AST/Symbol.o AST/HashTable.o AST/Symbols.o GUI/GTKView.o Config/GTKConfig.o Evaluators/Evaluators.o Formatters/LATEX.o Formatters/UTFStateMachine.o GUI/GTKLATEXGenerator.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o Formatters/SExpression.o REPL/REPL.o GUI/GTKCompleter.o Evaluators/Backtracker.o AST/Keyword.o GUI/GTKTerminalEmulator.o Scanners/OperatorPrecedenceList.o Formatters/Math.o $(NUMBER_OBJECTS) Evaluators/Operation.o FFIs/Trampolines.o FFIs/GTKUI.o FFIs/RecordPacker.o REPL/BuiltinSelector.o FFIs/POSIXProcessInfos.o $(FFIS) Evaluators/ModuleLoader.o
	g++ -o $@ $^ $(GUI_LDFLAGS) -lutil

GUI/GTKTerminalEmulator.o: GUI/GTKTerminalEmulator.cc GUI/TerminalEmulator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

Version: debian/changelog Makefile
	head -1 debian/changelog |awk '{print "#define VERSION \""$$2 "\""}' |tr -d "()" > Version.new && mv Version.new Version
	
Numbers/Integer.o: Numbers/Integer.cc Numbers/Integer Numbers/Small Evaluators/Operation Evaluators/Builtins Numbers/BigUnsigned Evaluators/Evaluators
Numbers/Real.o: Numbers/Real.cc Numbers/Real Numbers/Small Evaluators/Operation Evaluators/Builtins Evaluators/Evaluators
Numbers/BigUnsigned.o: Numbers/BigUnsigned.cc Numbers/BigUnsigned

clean:
	rm -f Linear_Algebra/*.o
	rm -f AST/*.o
	rm -f Scanners/*.o
	rm -f Formatters/*.o
	rm -f GUI/*.o
	rm -f Evaluators/*.o
	rm -f FFIs/*.o
	rm -f REPL/*.o
	rm -f Numbers/*.o
	rm -f TUI/*.o
	rm -f Config/*.o
	
distclean: clean
	rm -f $(EXECUTABLES)
	rm -f $(GENERATEDS)

dist: all
	rm -rf "$(DISTDIR)"
	mkdir "$(DISTDIR)"
	cp Makefile "$(DISTDIR)"/Makefile
	cp COPYING "$(DISTDIR)"/COPYING
	cp TODO "$(DISTDIR)"/TODO
	for s in $(SUBDIRS2) ; do mkdir "$(DISTDIR)"/"$$s" && cp "$$s"/* "$(DISTDIR)"/"$$s"/ ; rm -f "$(DISTDIR)"/"$$s"/*.o ; done
	for s in $(EXECUTABLES) ; do rm -f "$(DISTDIR)"/"$$s" ; done
	tar zcf "$(DISTDIR).tar.gz" "$(DISTDIR)/"* 
	rm -rf "$(DISTDIR)"

installgui:
	install -m 755 -d $(DESTDIR)/usr
	install -m 755 -d $(DESTDIR)/usr/bin
	install -m 755 GUI/5D $(DESTDIR)/usr/bin/5D
	install -m 755 -d $(DESTDIR)/usr/share
	install -m 755 -d $(DESTDIR)/usr/share/5D
	install -m 644 doc/tips $(DESTDIR)/usr/share/5D/tips

install: $(shell pkg-config --cflags --libs gtk+-2.0 2>/dev/null |grep -q -- -  && echo installgui )
	install -m 755 -d $(DESTDIR)/usr
	install -m 755 -d $(DESTDIR)/usr/bin
	install -m 755 TUI/TUI $(DESTDIR)/usr/bin/T5D
	strip $(DESTDIR)/usr/bin/T5D
	install -m 755 REPL/5DREPL $(DESTDIR)/usr/bin/5DREPL
	strip $(DESTDIR)/usr/bin/5DREPL
	install -m 755 -d $(DESTDIR)/usr/share
	install -m 755 -d $(DESTDIR)/usr/share/5D
	install -m 755 FFIs/find5DExports $(DESTDIR)/usr/share/5D/find5DExports
	install -m 755 FFIs/extractGNUSymbols $(DESTDIR)/usr/share/5D/extractGNUSymbols
	$(MAKE) -C Runtime install
