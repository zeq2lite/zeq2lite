VERSION=1.35_SVN
RELEASE=0
ARCH=x86

all: zeq2-$(VERSION)-$(RELEASE).$(ARCH).exe

zeq2lite.nsi: zeq2lite.nsi.in
	sed 's/XXXVERSIONXXX/$(VERSION)/;s/XXXRELEASEXXX/$(RELEASE)/;s/x86/$(ARCH)/g' < $< > $@

zeq2lite-$(VERSION)-$(RELEASE).$(ARCH).exe: zeq2lite.nsi
	makensis zeq2lite.nsi

clean:
	rm -rf *.exe zeq2lite.nsi

.PHONY: all clean
