
all: kernel32.5D user32.5D gdi32.5D

kernel32.5D: kernel32.sign
	grep -v "unknown" kernel32.sign |grep -v "?" > kernel32.5D ; ./find5DExports kernel32.5D >>kernel32.5D
user32.5D: user32.sign
	grep -v "unknown" user32.sign |grep -v "?" > user32.5D ; ./find5DExports user32.5D >>user32.5D
gdi32.5D: gdi32.sign
	grep -v "unknown" gdi32.sign |grep -v "?" > gdi32.5D ; ./find5DExports gdi32.5D >>gdi32.5D
kernel32.sign: kernel32.symbols prototypes
	./generateSignatures $< >kernel32.sign.new && mv kernel32.sign.new kernel32.sign
user32.sign: user32.symbols prototypes
	./generateSignatures $< >user32.sign.new && mv user32.sign.new user32.sign
gdi32.sign: gdi32.symbols prototypes
	./generateSignatures gdi32.symbols >gdi32.sign.new && mv gdi32.sign.new gdi32.sign
	
clean:
	rm -f kernel32.5D user32.5D gdi32.5D kernel32.sign user32.sign gdi32.sign

distclean: clean
	