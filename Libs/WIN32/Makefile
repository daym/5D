
all: kernel32.5D user32.5D gdi32.5D

kernel32.5D: kernel32.sign
	../../FFIs/find5DExports kernel32.sign >kernel32.5D.new && mv kernel32.5D.new kernel32.5D
user32.5D: user32.sign
	../../FFIs/find5DExports user32.sign >user32.5D.new && mv user32.5D.new user32.5D
gdi32.5D: gdi32.sign
	../../FFIs/find5DExports gdi32.sign >>gdi32.5D.new && mv gdi32.5D.new gdi32.5D
kernel32.sign: kernel32.symbols prototypes
	../../FFIs/generateSignatures $< >kernel32.sign.new && mv kernel32.sign.new kernel32.sign
user32.sign: user32.symbols prototypes
	../../FFIs/generateSignatures $< >user32.sign.new && mv user32.sign.new user32.sign
gdi32.sign: gdi32.symbols prototypes
	../../FFIs/generateSignatures gdi32.symbols >gdi32.sign.new && mv gdi32.sign.new gdi32.sign
	
clean:
	rm -f kernel32.5D user32.5D gdi32.5D kernel32.sign user32.sign gdi32.sign

distclean: clean
	