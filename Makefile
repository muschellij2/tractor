SHELL=/bin/bash
export SHELL

R=R
ENV=/usr/bin/env
ECHO=/bin/echo
ECHO_N=/bin/echo -n
GIT=git
MD5=md5
DOCKER=docker
INSTALL=$(ENV) R=$(R) bin/extra/tractor_Rinstall

default: build post-build-info

post-build-info:
	@$(ECHO) 'Run "make install" to install packages'

build:
	@$(ECHO_N) "Building tractor executable... "
	@cd src && $(R) CMD make >build.log 2>&1 && $(ECHO) "OK" || ( $(ECHO) "FAIL"; exit 0 )

post-install-info:
	@$(ECHO)
	@$(ECHO) "Installation complete. You may wish to add the following lines"
	@$(ECHO) "to your ~/.bashrc file (or equivalent for other shells):"
	@$(ECHO)
	@$(ECHO) "  export TRACTOR_HOME=`pwd`"
	@$(ECHO) '  export PATH=$${TRACTOR_HOME}/bin:$${PATH}'
	@$(ECHO) '  export MANPATH=$${TRACTOR_HOME}/share/man:$${MANPATH}'
	@$(ECHO)
	@$(ECHO) "The ~/.bashrc file can be created if it does not already exist."

lib/.timestamp: lib/ore lib/reportr lib/corpcor lib/loder lib/shades lib/yaml lib/jsonlite lib/Rcpp lib/RcppEigen lib/mmand lib/RNifti lib/divest lib/RNiftyReg
	@$(INSTALL) $? && touch lib/.timestamp

install-libs: lib/.timestamp

check-and-install-libs: install-libs

lib/R/testthat:
	@$(INSTALL) -c testthat

lib/R/oro.nifti:
	@$(INSTALL) -c oro.nifti

lib/R/igraph:
	@$(INSTALL) -c igraph

install-base:
	@$(INSTALL) tractor.base

install-utils:
	@$(INSTALL) tractor.utils

install-reg:
	@$(INSTALL) tractor.reg

install-session:
	@$(INSTALL) tractor.session

install-nt:
	@$(INSTALL) tractor.nt

install-track:
	@$(INSTALL) tractor.track

install-graph:
	@$(INSTALL) tractor.graph

install: build
	@rm -f install.log
	@$(MAKE) check-and-install-libs install-base install-utils install-reg install-session install-track install-nt install-graph post-install-info

install-local: install

install-global:
	@$(MAKE) install INSTALL="$(INSTALL) -g"

install-all: install

uninstall:
	@rm -rf lib/R

uninstall-local: uninstall

uninstall-global:
	$(R) CMD REMOVE tractor.graph tractor.track tractor.nt tractor.session tractor.reg tractor.utils tractor.base

clean:
	@cd tests && $(MAKE) clean

distclean: clean
	@rm -f lib/.timestamp
	@rm -f bin/exec/tractor src/tractor.o src/build.log install.log
	@rm -f tractor.track/config.log tractor.track/config.status tractor.track/src/Makevars tractor.track/src/config.h

test:
	@cd tests && $(MAKE) run-tests R=$(R)

dtest:
	@cd tests && $(MAKE) debug-tests R=$(R)

utest: lib/R/testthat lib/R/oro.nifti lib/R/igraph
	@$(ENV) TRACTOR_HOME=. bin/tractor -i -v0 tests/scripts/unit-test tractor.base
	@$(ENV) TRACTOR_HOME=. bin/tractor -i -v0 tests/scripts/unit-test tractor.graph

deeptest: utest test

create-md5:
	@$(GIT) ls-files | grep -v -e '^lib/' -e '^md5.txt' -e '\.git' | xargs $(MD5) -r >md5.txt

check-md5:
	@mkdir -p tmp
	@$(ECHO_N) "Checking MD5 checksums... "
	@bin/tractor -q -z -i tests/scripts/check-md5 md5.txt >tmp/md5.txt && $(ECHO) "OK" || ( $(ECHO) "FAIL"; sed '$$ d' tmp/md5.txt )
	@rm -f tmp/md5.txt

docker: uninstall distclean
	@$(GIT) clean -Xf
	@$(GIT) submodule foreach $(GIT) clean -Xf
	@minor_version=`cat VERSION | sed 's/\..$$//'` && $(DOCKER) build -t "tractor:$$minor_version" .
