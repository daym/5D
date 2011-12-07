
all: kernel32.5D user32.5D gdi32.5D

kernel32.5D: kernel32.sign
	grep -v "unknown" kernel32.sign > kernel32.5D ; ./find5DExports kernel32.5D >>kernel32.5D
user32.5D: user32.sign
	grep -v "unknown" user32.sign > user32.5D ; ./find5DExports user32.5D >>user32.5D
gdi32.5D: gdi32.sign
	grep -v "unknown" gdi32.sign > gdi32.5D ; ./find5DExports gdi32.5D >>gdi32.5D
kernel32.sign: kernel32.list ax
	./generateSignatures kernel32.list >kernel32.sign.new && mv kernel32.sign.new kernel32.sign
user32.sign: gdi32.list ax
	./generateSignatures user32.list >user32.sign.new && mv user32.sign.new user32.sign
gdi32.sign: gdi32.list ax
	./generateSignatures gdi32.list >gdi32.sign.new && mv gdi32.sign.new gdi32.sign
	

clean:
	rm -f kernel32.5D user32.5D gdi32.5D kernel32.sign user32.sign gdi32.sign

distclean: clean
	