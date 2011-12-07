
all: GTK.5D

GTK.5D: GTK.sign
	../../FFIs/find5DExports GTK.sign >GTK.5D.new && mv GTK.5D.new GTK.5D
GTK.sign: GTK.symbols prototypes
	../generateSignatures $< libgtk-3.so.0 >GTK.sign.new && mv GTK.sign.new GTK.sign
GTK.symbols: /usr/lib/libgtk-3.so 
	../../FFIs/extractGNUSymbols $< > GTK.symbols.new && mv GTK.symbols.new GTK.symbols

clean:
	rm -f GTK.5D GTK.sign

distclean: clean

# GDK
	