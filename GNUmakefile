.PHONY: all test clean distclean dist rebase

all: test

dist:
	rm -rf inc META.y*ml
	perl Makefile.PL
	$(MAKE) -f Makefile dist

install distclean tardist: Makefile
	$(MAKE) -f $< $@

test: Makefile
	TEST_RELEASE=1 $(MAKE) -f $< $@

Makefile: Makefile.PL
	perl $<

clean: distclean

reset: clean
	perl Makefile.PL
	$(MAKE) test

rebase:
	rm -rf Search-Kino030
	rsync -av KinoSearch-0.30_01/ Search-Kino030/
	chmod -R +w Search-Kino030/
	./script/rebase