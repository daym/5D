
SUBDIRS = Config REPL Bugs WIN32 Tests FFIs Bootstrappers TUI doc lib debian GUI
SUBDIRS2 = $(SUBDIRS) doc/building doc/installation doc/interna doc/library doc/programming doc/programming/manual doc/programming/tutorial Tests/0* lib/Arithmetic lib/Trigonometry lib/Logic lib/Composition lib/List lib/OS lib/IO lib/UI lib/FFI lib/String lib/Reflection lib/Error Examples lib/LinearAlgebra lib/OO lib/Pair lib/Maybe lib/Set lib/Testers lib/Tree
EXECUTABLES = REPL/5DREPL GUI/5D REPL/5DREPL TUI2/5D
GENERATEDS = FFIs/Trampolines FFIs/TrampolineSymbols.cc FFIs/TrampolineSymbols FFIs/Combinations
ASSEMBLERS = Assemblers/X86.o Assemblers/X64.o Assemblers/ARM.o

# -O3 is for tail-call optimization

#LIBGC_LD_WRAP_LDFLAGS = -Wl,--wrap -Wl,read -Wl,--wrap -Wl,dlopen -Wl,--wrap -Wl,pthread_create -Wl,--wrap -Wl,pthread_join -Wl,--wrap -Wl,pthread_detach -Wl,--wrap -Wl,pthread_sigmask -Wl,--wrap -Wl,sleep
LIBGC_LD_WRAP_CFLAGS = -D_REENTRANT -DGC_THREADS -DUSE_LD_WRAP
#CXXFLAGS += -O3 -Wall -I. -fno-strict-overflow -Wno-deprecated -Wno-div-by-zero `pkg-config --cflags glib-2.0 gthread-2.0 libxml-2.0 libffi` $(LIBGC_LD_WRAP_CFLAGS) -D_FILE_OFFSET_BITS=64 -Wno-unused-function -Ilib/Builtins/include
CXXFLAGS += -O1 -g3 -Wall -I. -fno-strict-overflow -Wno-deprecated -Wno-div-by-zero `pkg-config --cflags glib-2.0 gthread-2.0 libxml-2.0 libffi readline bdw-gc` $(LIBGC_LD_WRAP_CFLAGS) -D_FILE_OFFSET_BITS=64 -Wno-unused-function -Ilib/Builtins/include -fsanitize=undefined
ifdef PREFIX
CXXFLAGS +=  -DPREFIX=\"$(PREFIX)\"
endif
PACKAGE = 5D
VERSION = $(shell head -1 debian/changelog |cut -d"(" -f2 |cut -d")" -f1)
DISTDIR = $(PACKAGE)-$(VERSION)

BUILTINS_LIB = lib/Builtins/lib5dbuiltins.so.1
BUILTINS_HEADERS = lib/Builtins/include/5D/Allocators lib/Builtins/include/5D/Evaluators lib/Builtins/include/5D/FFIs lib/Builtins/include/5D/ModuleSystem lib/Builtins/include/5D/Operations lib/Builtins/include/5D/Values lib/Builtins/include/5D/ModuleSystem

#-fwrapv
#-Werror=strict-overflow

ifeq ($(shell ls -1 /usr/lib/libreadline.a 2>/dev/null),/usr/lib/libreadline.a)
LDFLAGS += /usr/lib/libreadline.a /usr/lib/libtinfo.a
else
LDFLAGS += -lreadline
endif
LDFLAGS += -ldl -lgc -lpthread `pkg-config --libs glib-2.0 gthread-2.0 libxml-2.0 libffi readline bdw-gc` $(LIBGC_LD_WRAP_LDFLAGS) -lgccpp -fsanitize=undefined
#dupe -lffi
GUI_CXXFLAGS = $(CXXFLAGS) `pkg-config --cflags gtk+-2.0`
GUI_LDFLAGS = $(LDFLAGS) `pkg-config --libs gtk+-2.0`

NUMBER_OBJECTS = Numbers/Integer.o Numbers/Real.o Numbers/BigUnsigned.o Numbers/Ratio.o

.SUFFIXES:            # Delete the default suffixes
%.o: %.cc
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<


TARGETS = TUI2/5D

#TARGETS += $(shell pkg-config --cflags --libs gtk+-2.0 2>/dev/null |grep -q -- -  && echo GUI/5D )

all: $(TARGETS)
	$(MAKE) -C lib all

$(BUILTINS_LIB):
	$(MAKE) -C lib/Builtins

Config/GTKConfig.o: Config/GTKConfig.cc Config/Config lib/Builtins/FFIs/Allocators lib/Builtins/Values/Values
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

REPL/5DREPL: REPL/main.o REPL/REPL.o REPL/TUIModule.o $(BUILTINS_LIB)
	g++ -o $@ $^ $(LDFLAGS)

TUI/TUI: TUI/main.o TUI/Interrupt.o REPL/REPL.o $(BUILTINS_LIB)
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

TUI/STUI: TUI/main.o TUI/Interrupt.o REPL/REPL.o $(BUILTINS_LIB)
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

TUI2/REPL.o: TUI2/REPL.cc Version $(BUILTINS_HEADERS) REPL/Symbols

TUI2/5D: TUI2/main.o TUI/Interrupt.o $(BUILTINS_LIB) TUI2/TUIModule.o
	g++ -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

TUI2/TUIModule.o: TUI2/TUIModule.cc lib/Builtins/include/5D/FFIs

REPL/main.o: REPL/main.cc REPL/REPL Version $(BUILTINS_HEADERS) REPL/REPLEnvironment
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI/main.o: TUI/main.cc $(BUILTINS_HEADERS) REPL/REPLEnvironment Version TUI/Interrupt Config/Config 
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI2/main.o: TUI2/main.cc $(BUILTINS_HEADERS) REPL/REPLEnvironment Version TUI/Interrupt Config/Config TUI2/REPL REPL/Symbols
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

TUI/Interrupt.o: TUI/Interrupt.cc TUI/Interrupt
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKGUI.o: GUI/GTKGUI.cc Version GUI/GTKREPL GUI/GTKView REPL/REPL $(BUILTINS_HEADERS)
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKREPL.o: GUI/GTKREPL.cc Version Config/Config GUI/UI_definition.UI GUI/GTKLATEXGenerator REPL/REPL GUI/CommonCompleter GUI/GTKCompleter REPL/REPLEnvironment GUI/WindowIcon 
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKCompleter.o: GUI/GTKCompleter.cc GUI/CommonCompleter GUI/GTKCompleter 
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

REPL/REPL.o: REPL/REPL.cc REPL/REPL 
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/GTKLATEXGenerator.o: GUI/GTKLATEXGenerator.cc GUI/GTKLATEXGenerator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
GUI/GTKView.o: GUI/GTKView.cc
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

GUI/5D: GUI/GTKGUI.o GUI/GTKREPL.o GUI/GTKView.o Config/GTKConfig.o GUI/GTKLATEXGenerator.o REPL/REPL.o GUI/GTKCompleter.o GUI/GTKTerminalEmulator.o GUI/GTKUIModule.o REPL/ExtREPL.o $(BUILTINS_LIB)
	g++ -o $@ $^ $(GUI_LDFLAGS) -lutil

GUI/GTKTerminalEmulator.o: GUI/GTKTerminalEmulator.cc GUI/TerminalEmulator
	$(CXX) $(GUI_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

Version: debian/changelog Makefile
	head -1 debian/changelog |awk '{print "#define VERSION \""$$2 "\""}' |tr -d "()" > Version.new && mv Version.new Version
	
REPL/ExtREPL.o: REPL/ExtREPL.cc REPL/ExtREPL REPL/Symbols $(BUILTINS_HEADERS)
REPL/Symbols.o: REPL/Symbols.cc REPL/Symbols $(BUILTINS_HEADERS)

clean:
	rm -f Values/*.o
	rm -f Scanners/*.o
	rm -f Formatters/*.o
	rm -f GUI/*.o
	rm -f Evaluators/*.o
	rm -f FFIs/*.o
	rm -f REPL/*.o
	rm -f Numbers/*.o
	rm -f TUI/*.o
	rm -f Config/*.o
	rm -f lib/Builtins/*/*.o
	rm -f TUI2/*.o
	
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
	install -m 755 TUI2/5D $(DESTDIR)/usr/bin/T5D
	#install -m 755 TUI2/STUI $(DESTDIR)/usr/bin/ST5D
	strip $(DESTDIR)/usr/bin/T5D
	#install -m 755 REPL/5DREPL $(DESTDIR)/usr/bin/5DREPL
	#strip $(DESTDIR)/usr/bin/5DREPL
	install -m 755 -d $(DESTDIR)/usr/share
	install -m 755 -d $(DESTDIR)/usr/share/5D
	install -m 755 ./lib/Builtins/FFIs/find5DExports $(DESTDIR)/usr/share/5D/find5DExports
	install -m 755 ./lib/Builtins/FFIs/extractGNUSymbols $(DESTDIR)/usr/share/5D/extractGNUSymbols
	$(MAKE) -C lib install
test: ./TUI2/5D
	grep "^>" Tests/session |sed 's;^>;;' |./TUI2/5D > Tests/session.result.new && mv Tests/session.result.new Tests/session.result
	diff Tests/session.result Tests/session
	
