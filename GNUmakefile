.PHONY: all test clean distclean dist repackage repackage-only repackage-build

PACKAGE := Search::Kino03
PACKAGE_dir := Search-Kino03

# all: test

# dist:
#     rm -rf inc META.y*ml
#     perl Makefile.PL
#     $(MAKE) -f Makefile dist

# install distclean tardist: Makefile
#     $(MAKE) -f $< $@

# test: Makefile
#     TEST_RELEASE=1 $(MAKE) -f $< $@

# Makefile: Makefile.PL
#     perl $<

# clean: distclean

# reset: clean
#     perl Makefile.PL
#     $(MAKE) test

repackage: repackage-only
	rsync -av $(PACKAGE_dir)/ ./
	rm -f _Build.PL
	rm -rf sample devel

repackage-only:
	rm -rf $(PACKAGE_dir)
	rsync -av KinoSearch-0.30_01/ $(PACKAGE_dir)
	cd $(PACKAGE_dir) && mv Build.PL _Build.PL
	chmod -R +w $(PACKAGE_dir)/
	./repackage/script/repackage $(PACKAGE)
#     cd $(PACKAGE_dir) && cp _Build.PL Build.PL

repackage-build: repackage-only
	cd $(PACKAGE_dir) && perl Build.PL && ./Build
