
all: Cairo.5D

Cairo.5D: Cairo.sign
	../../FFIs/find5DExports Cairo.sign >Cairo.5D.new && mv Cairo.5D.new Cairo.5D
Cairo.sign: Cairo.symbols prototypes
	../../FFIs/generateSignatures $< /usr/lib/x86_64-linux-gnu/libpango-1.0.so.0 >Cairo.sign.new && mv Cairo.sign.new Cairo.sign
Cairo.symbols: /usr/lib/x86_64-linux-gnu/libcairo.so.2
	../../FFIs/extractGNUSymbols $< > Cairo.symbols.new && mv Cairo.symbols.new Cairo.symbols

clean:
	rm -f GTK.5D GTK.sign
	rm -f Cairo.5D Cairo.sign

distclean: clean

# GDK
	