
all: Pango.5D

Pango.5D: Pango.sign
	../../FFIs/find5DExports Pango.sign >Pango.5D.new && mv Pango.5D.new Pango.5D
Pango.sign: Pango.symbols prototypes
	../../FFIs/generateSignatures $< /usr/lib/x86_64-linux-gnu/libpango-1.0.so.0 >Pango.sign.new && mv Pango.sign.new Pango.sign
Pango.symbols: /usr/lib/x86_64-linux-gnu/libpango-1.0.so.0
	../../FFIs/extractGNUSymbols $< > Pango.symbols.new && mv Pango.symbols.new Pango.symbols

clean:
	rm -f GTK.5D GTK.sign
	rm -f Pango.5D Pango.sign

distclean: clean

# GDK
	