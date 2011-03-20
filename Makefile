
CXXFLAGS += -Wall -I. -g3
LDFLAGS += -lreadline
GUI_CXXFLAGS = $(CXXFLAGS) `pkg-config --cflags gtk+-2.0`
GUI_LDFLAGS = $(LDFLAGS) `pkg-config --libs gtk+-2.0`

.SUFFIXES:            # Delete the default suffixes
%.o: %.C
%.o: %.C
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<


all: REPL/REPL GUI/GTKGUI

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
	
Linear_Algebra/test-Vector.o: Linear_Algebra/test-Vector.C Linear_Algebra/Vector
Linear_Algebra/test-Matrix.o: Linear_Algebra/test-Matrix.C Linear_Algebra/Matrix
Linear_Algebra/test-Tensor.o: Linear_Algebra/test-Tensor.C Linear_Algebra/Tensor
Formatters/LATEX.o: Formatters/LATEX.C Formatters/LATEX AST/AST AST/Symbol
AST/AST.o: AST/AST.C AST/AST
AST/test-AST.o: AST/test-AST.C AST/AST
AST/test-Symbol.o: AST/test-Symbol.C AST/Symbol AST/AST
AST/Symbol.o: AST/Symbol.C AST/Symbol
Scanners/Scanner.o: Scanners/Scanner.C Scanners/Scanner AST/Symbol
Scanners/test-Scanner.o: Scanners/test-Scanner.C Scanners/Scanner
Scanners/MathParser.o: Scanners/MathParser.C Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol
Scanners/test-MathParser.o: Scanners/test-MathParser.C Scanners/MathParser Scanners/Scanner
test: Linear_Algebra/test-Vector Linear_Algebra/test-Matrix Linear_Algebra/test-Tensor AST/test-AST AST/test-Symbol Scanners/test-Scanner Scanners/test-MathParser
	./Linear_Algebra/test-Vector
	./Linear_Algebra/test-Matrix
	./Linear_Algebra/test-Tensor
	./AST/test-Symbol
	./AST/test-AST
	./Scanners/test-Scanner
	./Scanners/test-MathParser

REPL/REPL: REPL/main.o Scanners/MathParser.o AST/AST.o AST/Symbol.o Scanners/Scanner.o
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

GUI/GTKGUI.o: GUI/GTKGUI.C GUI/GTKREPL
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKREPL.o: GUI/GTKREPL.C Scanners/MathParser Scanners/Scanner AST/AST AST/Symbol
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKView.o: GUI/GTKView.C
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKGUI: GUI/GTKGUI.o GUI/GTKREPL.o Scanners/MathParser.o Scanners/Scanner.o AST/AST.o AST/Symbol.o GUI/GTKView.o
	g++ -o $@ $^ $(GUI_LDFLAGS)

clean:
	rm -f Linear_Algebra/*.o
	rm -f AST/*.o
	rm -f Scanners/*.o
	
distclean: clean
	rm -f Linear_Algebra/test-Matrix
	rm -f Linear_Algebra/test-Vector
	rm -f Linear_Algebra/test-Tensor
	rm -f AST/test-AST
	rm -f AST/test-Symbol
	rm -f Scanners/test-MathParser
	rm -f Scanners/test-Scanner
