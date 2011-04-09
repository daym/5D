
CXXFLAGS += -Wall -I. -g3
LDFLAGS += -lreadline -ldl
GUI_CXXFLAGS = $(CXXFLAGS) `pkg-config --cflags gtk+-2.0`
GUI_LDFLAGS = $(LDFLAGS) `pkg-config --libs gtk+-2.0`

.SUFFIXES:            # Delete the default suffixes
%.o: %.cc
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<


all: REPL/REPL GUI/4D

Linear_Algebra/test-Vector: Linear_Algebra/test-Vector.o
	g++ -o Linear_Algebra/test-Vector Linear_Algebra/test-Vector.o

Linear_Algebra/test-Matrix: Linear_Algebra/test-Matrix.o
	g++ -o Linear_Algebra/test-Matrix Linear_Algebra/test-Matrix.o

Linear_Algebra/test-Tensor: Linear_Algebra/test-Tensor.o
	g++ -o Linear_Algebra/test-Tensor Linear_Algebra/test-Tensor.o

Scanners/test-Scanner: Scanners/test-Scanner.o Scanners/Scanner.o
	g++ -o $@ $^ $(CXXFLAGS)

Scanners/test-MathParser: Scanners/test-MathParser.o Scanners/MathParser.o Scanners/Scanner.o AST/Symbol.o AST/AST.o
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
Formatters/LATEX.o: Formatters/LATEX.cc Formatters/LATEX AST/AST AST/Symbol Scanners/MathParser Formatters/UTFStateMachine
Formatters/UTFStateMachine.o: Formatters/UTFStateMachine.cc Formatters/UTFStateMachine Formatters/UTF-8_to_LATEX_result.h
Formatters/UTF-8_to_LATEX_result.h: Formatters/UTF-8_to_LATEX.table Formatters/generate-state-machine
	Formatters/generate-state-machine Formatters/UTF-8_to_LATEX.table >tmp4711 && mv tmp4711 Formatters/UTF-8_to_LATEX_result.h
AST/AST.o: AST/AST.cc AST/AST
AST/test-AST.o: AST/test-AST.cc AST/AST
AST/test-Symbol.o: AST/test-Symbol.cc AST/Symbol AST/AST
AST/Symbol.o: AST/Symbol.cc AST/Symbol
Scanners/Scanner.o: Scanners/Scanner.cc Scanners/Scanner AST/Symbol
Scanners/test-Scanner.o: Scanners/test-Scanner.cc Scanners/Scanner
Scanners/MathParser.o: Scanners/MathParser.cc Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol
Scanners/test-MathParser.o: Scanners/test-MathParser.cc Scanners/MathParser Scanners/Scanner
Evaluators/Evaluators.o: Evaluators/Evaluators.cc Evaluators/Evaluators AST/AST AST/Symbol
Evaluators/Builtins.o: Evaluators/Builtins.cc Evaluators/Builtins AST/AST AST/Symbol
Evaluators/FFI.o: Evaluators/FFI.cc Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators
FFIs/POSIX.o: FFIs/POSIX.cc FFIs/POSIX Evaluators/FFI AST/AST AST/Symbol Evaluators/Evaluators
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

REPL/REPL: REPL/main.o Scanners/MathParser.o AST/AST.o AST/Symbol.o Scanners/Scanner.o Evaluators/Evaluators.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

GUI/GTKGUI.o: GUI/GTKGUI.cc GUI/GTKREPL GUI/GTKView
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKREPL.o: GUI/GTKREPL.cc Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol Config/Config Formatters/LATEX GUI/UI_definition.UI GUI/GTKLATEXGenerator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKLATEXGenerator.o: GUI/GTKLATEXGenerator.cc GUI/GTKLATEXGenerator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
GUI/GTKView.o: GUI/GTKView.cc
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/4D: GUI/GTKGUI.o GUI/GTKREPL.o Scanners/MathParser.o Scanners/Scanner.o AST/AST.o AST/Symbol.o GUI/GTKView.o Config/GTKConfig.o Evaluators/Evaluators.o Formatters/LATEX.o Formatters/UTFStateMachine.o GUI/GTKLATEXGenerator.o Evaluators/Builtins.o Evaluators/FFI.o FFIs/POSIX.o
	g++ -o $@ $^ $(GUI_LDFLAGS)

clean:
	rm -f Linear_Algebra/*.o
	rm -f AST/*.o
	rm -f Scanners/*.o
	rm -f Formatters/*.o
	
distclean: clean
	rm -f Linear_Algebra/test-Matrix
	rm -f Linear_Algebra/test-Vector
	rm -f Linear_Algebra/test-Tensor
	rm -f AST/test-AST
	rm -f AST/test-Symbol
	rm -f Scanners/test-MathParser
	rm -f Scanners/test-Scanner
	rm -f GUI/4D
	rm -f REPL/REPL

