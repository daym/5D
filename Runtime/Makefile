
SUBDIRS2 = Arithmetic Logic Composition List OS IO UI FFI String Reflection Error
#PACKAGE = 5D
#VERSION = $(shell head -1 debian/changelog |cut -d"(" -f2 |cut -d")" -f1)
#DISTDIR = $(PACKAGE)-$(VERSION)

all:
	$(MAKE) -C Arithmetic all
	$(MAKE) -C Logic all
	$(MAKE) -C Composition all
	$(MAKE) -C List all
	$(MAKE) -C OS all
	$(MAKE) -C IO all
	$(MAKE) -C UI all
	$(MAKE) -C FFI all
	$(MAKE) -C String all
	$(MAKE) -C Reflection all
	$(MAKE) -C Error all
	$(MAKE) -C LinearAlgebra all
	$(MAKE) -C OO all

clean:
	$(MAKE) -C Arithmetic clean
	$(MAKE) -C Logic clean
	$(MAKE) -C Composition clean
	$(MAKE) -C List clean
	$(MAKE) -C OS clean
	$(MAKE) -C IO clean
	$(MAKE) -C UI clean
	$(MAKE) -C FFI clean
	$(MAKE) -C String clean
	$(MAKE) -C Reflection clean
	$(MAKE) -C Error clean
	$(MAKE) -C LinearAlgebra clean
	$(MAKE) -C OO clean

distclean:
	$(MAKE) -C Arithmetic distclean
	$(MAKE) -C Logic distclean
	$(MAKE) -C Composition distclean
	$(MAKE) -C List distclean
	$(MAKE) -C OS distclean
	$(MAKE) -C IO distclean
	$(MAKE) -C UI distclean
	$(MAKE) -C FFI distclean
	$(MAKE) -C String distclean
	$(MAKE) -C Reflection distclean
	$(MAKE) -C Error distclean
	$(MAKE) -C LinearAlgebra distclean
	$(MAKE) -C OO distclean

install:
	$(MAKE) -C Arithmetic install
	$(MAKE) -C Logic install
	$(MAKE) -C Composition install
	$(MAKE) -C List install
	$(MAKE) -C OS install
	$(MAKE) -C IO install
	$(MAKE) -C UI install
	$(MAKE) -C FFI install
	$(MAKE) -C String install
	$(MAKE) -C Reflection install
	$(MAKE) -C Error install
	$(MAKE) -C LinearAlgebra install
	$(MAKE) -C OO install

dist:
