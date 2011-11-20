
CXXFLAGS += -Wall -I. -g3 -fno-strict-overflow
#-fwrapv
#-Werror=strict-overflow

LDFLAGS += -lreadline -ldl
GUI_CXXFLAGS = $(CXXFLAGS) `pkg-config --cflags gtk+-2.0`
GUI_LDFLAGS = $(LDFLAGS) `pkg-config --libs gtk+-2.0`

NUMBER_OBJECTS = Numbers/Integer.o Numbers/Real.o

.SUFFIXES:            # Delete the default suffixes
%.o: %.cc
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<


all: REPL/5DREPL GUI/5D TUI/TUI

Linear_Algebra/test-Vector: Linear_Algebra/test-Vector.o
	g++ -o Linear_Algebra/test-Vector Linear_Algebra/test-Vector.o

Linear_Algebra/test-Matrix: Linear_Algebra/test-Matrix.o
	g++ -o Linear_Algebra/test-Matrix Linear_Algebra/test-Matrix.o

Linear_Algebra/test-Tensor: Linear_Algebra/test-Tensor.o
	g++ -o Linear_Algebra/test-Tensor Linear_Algebra/test-Tensor.o

Scanners/test-Scanner: Scanners/test-Scanner.o Scanners/Scanner.o
	g++ -o $@ $^ $(CXXFLAGS)

Scanners/test-MathParser: Scanners/test-MathParser.o Scanners/MathParser.o Scanners/Scanner.o AST/Symbol.o AST/AST.o Scanners/OperatorPrecedenceList.o AST/Keyword.o
	g++ -o $@ $^ $(CXXFLAGS)
	
AST/test-AST: AST/test-AST.o
	g++ -o AST/test-AST AST/test-AST.o

AST/test-Symbol: AST/test-Symbol.o AST/Symbol.o AST/AST.o
	g++ -o AST/test-Symbol AST/test-Symbol.o AST/Symbol.o AST/AST.o

test-AST: test-AST.o
	g++ -o test-AST test-AST.o
	
Linear_Algebra/test-Vector.o: Linear_Algebra/test-Vector.cc Linear_Algebra/Vector
Linear_Algebra/test-Matrix.o: Linear_Algebra/test-Matrix.cc Linear_Algebra/Matrix
Linear_Algebra/test-Tensor.o: Linear_Algebra/test-Tensor.cc Linear_Algebra/Tensor
Formatters/LATEX.o: Formatters/LATEX.cc Formatters/LATEX AST/AST AST/Symbol Scanners/MathParser Formatters/UTFStateMachine Scanners/OperatorPrecedenceList
Formatters/SExpression.o: Formatters/SExpression.cc Formatters/SExpression AST/Symbol AST/AST
Formatters/Math.o: Formatters/Math.cc Formatters/Math AST/Symbol AST/AST
Formatters/UTFStateMachine.o: Formatters/UTFStateMachine.cc Formatters/UTFStateMachine Formatters/UTF-8_to_LATEX_result.h
Formatters/UTF-8_to_LATEX_result.h: Formatters/UTF-8_to_LATEX.table Formatters/generate-state-machine
	Formatters/generate-state-machine Formatters/UTF-8_to_LATEX.table >tmp4711 && mv tmp4711 Formatters/UTF-8_to_LATEX_result.h
AST/AST.o: AST/AST.cc AST/AST AST/Symbol
AST/test-AST.o: AST/test-AST.cc AST/AST
AST/test-Symbol.o: AST/test-Symbol.cc AST/Symbol AST/AST
AST/Symbol.o: AST/Symbol.cc AST/Symbol AST/AST
AST/Keyword.o: AST/Keyword.cc AST/Keyword AST/AST
Scanners/Scanner.o: Scanners/Scanner.cc Scanners/Scanner AST/Symbol AST/AST AST/Keyword
Scanners/test-Scanner.o: Scanners/test-Scanner.cc Scanners/Scanner
Scanners/MathParser.o: Scanners/MathParser.cc Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol Scanners/OperatorPrecedenceList
Scanners/OperatorPrecedenceList.o: Scanners/OperatorPrecedenceList.cc AST/AST AST/Symbol
Scanners/test-MathParser.o: Scanners/test-MathParser.cc Scanners/MathParser Scanners/Scanner Scanners/OperatorPrecedenceList
Evaluators/Evaluators.o: Evaluators/Evaluators.cc Evaluators/Evaluators AST/AST AST/Symbol Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/MathParser  Scanners/OperatorPrecedenceList
Evaluators/Builtins.o: Evaluators/Builtins.cc Scanners/MathParser Evaluators/Builtins Numbers/Integer Numbers/Real AST/AST AST/Symbol AST/Keyword FFIs/ResultMarshaller FFIs/FFIs  Scanners/OperatorPrecedenceList
Evaluators/Backtracker.o: Evaluators/Backtracker.cc Evaluators/Backtracker
Evaluators/FFI.o: Evaluators/FFI.cc Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real
FFIs/POSIX.o: FFIs/POSIX.cc FFIs/FFIs Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators Numbers/Integer Numbers/Real
FFIs/ArgumentMarshaller.o: FFIs/ArgumentMarshaller.cc FFIs/ArgumentMarshaller Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators FFIs/CallMarshaller
FFIs/ResultMarshaller.o: FFIs/ResultMarshaller.cc FFIs/ResultMarshaller Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators FFIs/ArgumentMarshaller
FFIs/CallMarshaller.o: FFIs/CallMarshaller.cc FFIs/CallMarshaller Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators
Config/GTKConfig.o: Config/GTKConfig.cc Config/Config
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

test: Linear_Algebra/test-Vector Linear_Algebra/test-Matrix Linear_Algebra/test-Tensor AST/test-AST AST/test-Symbol Scanners/test-Scanner Scanners/test-MathParser
	./Linear_Algebra/test-Vector
	./Linear_Algebra/test-Matrix
	./Linear_Algebra/test-Tensor
	./AST/test-Symbol
	./AST/test-AST
	./Scanners/test-Scanner
	./Scanners/test-MathParser

REPL/5DREPL: REPL/main.o REPL/REPL.o Scanners/MathParser.o AST/AST.o AST/Symbol.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o FFIs/ResultMarshaller.o FFIs/ArgumentMarshaller.o FFIs/CallMarshaller.o Evaluators/Backtracker.o AST/Keyword.o Formatters/SExpression.o Formatters/Math.o Scanners/OperatorPrecedenceList.o $(NUMBER_OBJECTS)
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

TUI/TUI: TUI/main.o Scanners/MathParser.o AST/AST.o AST/Symbol.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o FFIs/ResultMarshaller.o FFIs/ArgumentMarshaller.o FFIs/CallMarshaller.o Evaluators/Backtracker.o AST/Keyword.o Formatters/SExpression.o Formatters/Math.o Scanners/OperatorPrecedenceList.o TUI/Interrupt.o REPL/REPL.o $(NUMBER_OBJECTS)
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

REPL/main.o: REPL/main.cc REPL/REPL REPL/REPLEnvironment FFIs/FFIs AST/AST AST/Symbol Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators FFIs/ResultMarshaller Scanners/MathParser Scanners/Scanner  Scanners/OperatorPrecedenceList Evaluators/Builtins Scanners/MathParser Scanners/Scanner Evaluators/Evaluators
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI/main.o: TUI/main.cc REPL/REPLEnvironment FFIs/FFIs AST/AST AST/Symbol Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators FFIs/ResultMarshaller Scanners/MathParser Scanners/Scanner  Scanners/OperatorPrecedenceList Evaluators/Builtins Numbers/Integer Numbers/Real TUI/Interrupt Config/Config Evaluators/Evaluators
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI/Interrupt.o: TUI/Interrupt.cc TUI/Interrupt
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKGUI.o: GUI/GTKGUI.cc GUI/GTKREPL GUI/GTKView REPL/REPL
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKREPL.o: GUI/GTKREPL.cc Scanners/MathParser Evaluators/Builtins Scanners/Scanner AST/AST AST/Symbol Config/Config Formatters/LATEX GUI/UI_definition.UI GUI/GTKLATEXGenerator Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators REPL/REPL GUI/CommonCompleter GUI/GTKCompleter FFIs/ResultMarshaller REPL/REPLEnvironment GUI/WindowIcon Scanners/OperatorPrecedenceList
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKCompleter.o: GUI/GTKCompleter.cc GUI/CommonCompleter GUI/GTKCompleter AST/AST AST/Symbol Scanners/MathParser Scanners/OperatorPrecedenceList
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

REPL/REPL.o: REPL/REPL.cc REPL/REPL AST/AST AST/Symbol Scanners/Scanner Scanners/MathParser Formatters/SExpression Formatters/Math Evaluators/FFI Evaluators/Evaluators Evaluators/Builtins Numbers/Integer Numbers/Real Scanners/OperatorPrecedenceList
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKLATEXGenerator.o: GUI/GTKLATEXGenerator.cc GUI/GTKLATEXGenerator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
GUI/GTKView.o: GUI/GTKView.cc
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/5D: GUI/GTKGUI.o GUI/GTKREPL.o Scanners/MathParser.o Scanners/Scanner.o AST/AST.o AST/Symbol.o GUI/GTKView.o Config/GTKConfig.o Evaluators/Evaluators.o Formatters/LATEX.o Formatters/UTFStateMachine.o GUI/GTKLATEXGenerator.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o Formatters/SExpression.o REPL/REPL.o FFIs/ArgumentMarshaller.o FFIs/ResultMarshaller.o FFIs/CallMarshaller.o FFIs/ArgumentMarshaller.o GUI/GTKCompleter.o Evaluators/Backtracker.o AST/Keyword.o GUI/GTKTerminalEmulator.o Scanners/OperatorPrecedenceList.o Formatters/Math.o $(NUMBER_OBJECTS)
	g++ -o $@ $^ $(GUI_LDFLAGS) -lutil

GUI/GTKTerminalEmulator.o: GUI/GTKTerminalEmulator.cc GUI/TerminalEmulator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

Numbers/Integer.o: Numbers/Integer.cc Numbers/Integer Numbers/Small Evaluators/Builtins
Numbers/Real.o: Numbers/Real.cc Numbers/Real Numbers/Small Evaluators/Builtins

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
	
distclean: clean
	rm -f Linear_Algebra/test-Matrix
	rm -f Linear_Algebra/test-Vector
	rm -f Linear_Algebra/test-Tensor
	rm -f AST/test-AST
	rm -f AST/test-Symbol
	rm -f Scanners/test-MathParser
	rm -f Scanners/test-Scanner
	rm -f GUI/5D
	rm -f REPL/5DREPL

