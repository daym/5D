
all: DBus.5D

DBus.5D: DBus.sign
	../../FFIs/find5DExports DBus.sign >DBus.5D.new && mv DBus.5D.new DBus.5D
DBus.sign: DBus.symbols prototypes
	../generateSignatures $< /usr/lib/x86_64-linux-gnu/libdbus-glib-1.so.2 >DBus.sign.new && mv DBus.sign.new DBus.sign
DBus.symbols: /usr/lib/x86_64-linux-gnu/libdbus-glib-1.so.2
	../../FFIs/extractGNUSymbols $< > DBus.symbols.new && mv DBus.symbols.new DBus.symbols

clean:
	rm -f DBus.5D DBus.sign

distclean: clean
