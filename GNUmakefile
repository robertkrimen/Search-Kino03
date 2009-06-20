.PHONY: all test clean distclean dist repackage repackage-only repackage-build

PACKAGE := Search::Kino03
PACKAGE_dir := Search-Kino03

all: test

dist:
	rm -rf inc META.y*ml
	perl Build.PL
	./Build dist

# install distclean tardist: Build
#     $(MAKE) -f $< $@

Build: Build.PL
	perl $<

test: Build
	./Build test

 clean: distclean

reset: clean
	perl Build.PL
	./Build test

repackage: repackage-only
	rsync -av $(PACKAGE_dir)/ ./
	rm -f _Build.PL _MANIFEST
	rm -rf devel

repackage-only:
	rm -rf $(PACKAGE_dir)
	rsync -av KinoSearch-0.30_01/ $(PACKAGE_dir)
	cd $(PACKAGE_dir) && mv Build.PL _Build.PL
	cd $(PACKAGE_dir) && mv MANIFEST _MANIFEST
	chmod -R +w $(PACKAGE_dir)/
	./repackage/script/repackage $(PACKAGE)
#     cd $(PACKAGE_dir) && cp _Build.PL Build.PL

repackage-build: repackage-only
	cd $(PACKAGE_dir) && perl Build.PL && ./Build
