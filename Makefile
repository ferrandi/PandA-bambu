
J ?= $(shell grep -c ^processor /proc/cpuinfo)
BUILD_DIR ?= build
PKG_DIR ?= panda_dist/distdir
SCAN_BUILD_REPORT_DIR ?= scanbuild-report
CONFIGURE_FLAGS ?= --enable-opt --enable-release

ifdef SCAN_BUILD_VERSION
override SCAN_BUILD_VERSION := -$(SCAN_BUILD_VERSION)
endif

all: build

.PHONY: all fix_revision update-submodules configure run-configure build install appimage-setup appimage scan-build clean distclean

fix_revision:
	@if command -v git > /dev/null; then if git status 2>&1 > /dev/null; then \
		echo "\"`git rev-parse HEAD`\"" > dist_revision_info; \
		echo "\"`git rev-parse --abbrev-ref HEAD`\"" >> dist_revision_info; \
	fi; fi

update-submodules:
	@if command -v git > /dev/null; then git submodule init; git submodule update; fi

autoreconf.log: configure.ac etc/macros/*
	@autoreconf -m -iv --warning=none 2>&1 | tee autoreconf.log

configure: autoreconf.log

run-configure: fix_revision update-submodules configure
	@if [ "x$(shell cat $(BUILD_DIR)/current_flags 2>/dev/null)" != "x$(CONFIGURE_FLAGS)" ]; then \
		mkdir -p $(BUILD_DIR) $(PKG_DIR); \
		cd $(BUILD_DIR) && ../configure -C --prefix=/usr $(CONFIGURE_FLAGS) && echo -n "$(CONFIGURE_FLAGS)" > current_flags; \
	fi

build: run-configure
	@make --directory=$(BUILD_DIR) -j$(J)

install: build
	@make --directory=$(BUILD_DIR) install


### AppImage targets
appimage-setup: $(PKG_DIR)/AppRun

$(PKG_DIR)/AppRun: .devcontainer/library-scripts/appimage-setup.sh
	@bash -e .devcontainer/library-scripts/appimage-setup.sh $(PKG_DIR)

appimage: appimage-setup build
	@make --directory=$(BUILD_DIR) DESTDIR=$(shell readlink -e $(PKG_DIR)) install
	@ARCH=x86_64 appimagetool $(PKG_DIR) $(PKG_DIR)/../bambu.AppImage

### Scan build
scan-build: run-configure
	@make --directory=$(BUILD_DIR)/ext -j$(J)
	@scan-build$(SCAN_BUILD_VERSION) -v -v --use-cc=clang$(SCAN_BUILD_VERSION) --use-c++=clang++$(SCAN_BUILD_VERSION) --use-analyzer=clang$(SCAN_BUILD_VERSION) -o $(SCAN_BUILD_REPORT_DIR) make --directory=build/src -j$(J)

clean:
	@if [ -e $(BUILD_DIR)/Makefile ]; then make --directory=$(BUILD_DIR) clean; fi
	@rm -rf config.* lconfig.* configure.scan depcomp install-sh ltmain.sh missing mkinstalldirs autom4te.cache auxdir autoscan.log autoreconf.log aclocal.m4 configure ylwrap
	@find . -iname Makefile.in -delete
	@find . -iname "*.orig" -delete
	@find . -iname "*~" -delete

checkCompile :
	make -f etc/scripts/Makeall all CHECKS_FILE=./check_compilation

dist :
	make -f etc/scripts/Makeall dist CONFIGURE_OPTIONS="$(CONFIGURE_FLAGS)"

distclean :
	@find . -name autom4te.cache -delete
	@find . -name test-driver -delete
	@find src -name Makefile.in -delete
	@find ext -name aclocal.m4 -delete
	@rm -rf ar-lib autoreconf_log compile {,l}config.* configure depcomp install-sh ltmain.sh missing ylwrap
	@rm -rf etc/autoreconf_log etc/config.h.in etc/config.h.in~ etc/configure
	@rm -rf etc/macros/libtool.m4 etc/macros/lt~obsolete.m4 etc/macros/ltoptions.m4 etc/macros/ltsugar.m4 etc/macros/ltversion.m4
	@rm -rf etc/scripts/.test_panda.py.swp
	@rm -rf ext/autoreconf_log ext/config.h.in ext/config.h.in~ ext/configure
	@rm -rf ext/abseil-cpp/configure
	@rm -rf ext/Coin-Cbc/missing
	@rm -rf ext/flopoco/config.h.in ext/flopoco/config.h.in~ ext/flopoco/configure ext/flopoco/configure.ac ext/flopoco/m4 ext/flopoco/Makefile.am ext/flopoco/src/FPExpressions/ExpressionParser.h ext/flopoco/src/FPExpressions/FlopocoScanner.h ext/flopoco/src/Makefile.am
	@rm -rf ext/libfplll-4.0.3/config.h.in ext/libfplll-4.0.3/config.h.in~ ext/libfplll-4.0.3/configure
	@rm -rf ext/sollya/config.h.in ext/sollya/config.h.in~ ext/sollya/configure ext/sollya/m4/l*
