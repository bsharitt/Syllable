# Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# Makefile for installing locale data source files.

subdir := localedata

all: # Make this the default target; it will be defined in Rules.

# List with all available character set descriptions.
charmaps := $(filter-out $(addprefix charmaps/, CVS RCS SCCS %~), \
				     $(wildcard charmaps/[A-I]*) \
				     $(wildcard charmaps/[J-Z]*))

# List with all available character set descriptions.
locales := $(filter-out $(addprefix locales/, CVS RCS SCCS %~), \
				    $(wildcard locales/*))

# List of repertoire maps.
repertoiremaps := $(filter-out $(addprefix repertoiremaps/, CVS RCS SCCS %~), \
					   $(wildcard repertoiremaps/*))

test-srcs := collate-test xfrm-test tst-fmon tst-rpmatch
test-input := de_DE.ISO-8859-1 da_DK.ISO-8859-1 fr_CA,2.13.ISO-8859-1 \
	      hr_HR.ISO-8859-2 # once it is fixed: cs_CZ.ISO-8859-2
test-input-data = $(addsuffix .in, $(basename $(test-input)))
test-output := $(foreach s, .out .xout, \
			 $(addsuffix $s, $(basename $(test-input))))
ld-test-names := test1 test2 test3 test4 test5
ld-test-srcs := $(addprefix tests/,$(addsuffix .cm,$(ld-test-names)) \
				   $(addsuffix .def,$(ld-test-names)))

generated := $(test-input) $(test-output)
generated-dirs := $(basename $(test-input)) en_US $(ld-test-names)

distribute := CHECKSUMS README SUPPORTED ChangeLog			\
	      $(charmaps) $(locales) $(repertoiremaps)			\
	      tst-rpmatch.sh tst-locale.sh tst-fmon.sh sort-test.sh	\
	      tst-fmon.data $(test-input-data) $(ld-test-srcs)		\
	      th_TH.in cs_CZ.in

# Get $(inst_i18ndir) defined.
include ../Makeconfig

# Files to install.
install-others := $(addprefix $(inst_i18ndir)/, $(charmaps) $(locales) \
						$(repertoiremaps))

include ../Rules

# Install the source files in the appropriate directories.
$(inst_i18ndir)/charmaps/%: charmaps/% $(+force); $(do-install)
$(inst_i18ndir)/locales/%: locales/% $(+force); $(do-install)
$(inst_i18ndir)/repertoiremaps/%: repertoiremaps/% $(+force); $(do-install)


ifeq (no,$(cross-compiling))
ifeq (yes,$(build-shared))
.PHONY: do-collate-test do-tst-fmon do-tst-locale do-tst-rpmatch
tests: do-collate-test do-tst-fmon do-tst-locale do-tst-rpmatch
do-collate-test: sort-test.sh $(objpfx)collate-test $(objpfx)xfrm-test \
		 $(test-input-data)
	$(SHELL) -e $< $(common-objpfx) $(test-input)
do-tst-fmon: tst-fmon.sh $(objpfx)tst-fmon tst-fmon.data  do-collate-test
	$(SHELL) -e $< $(common-objpfx) tst-fmon.data
do-tst-locale: tst-locale.sh $(ld-test-srcs)
	$(SHELL) -e $< $(common-objpfx)
do-tst-rpmatch: tst-rpmatch.sh $(objpfx)tst-rpmatch do-tst-fmon
	$(SHELL) -e $< $(common-objpfx)
endif
endif

# Sometimes the whole collection of locale files should be installed.
LOCALEDEF=I18NPATH=. $(common-objpfx)elf/ld.so --library-path $(rpath-link) $(common-objpfx)locale/localedef
install-locales:
	while read locale charset; do \
	   case $$locale in \#*) continue;; esac; \
	   $(LOCALEDEF) -i locales/`echo $$locale | sed 's/\([^.]*\).*/\1/'` \
			-c -f charmaps/$$charset \
			-u repertoiremaps/mnemonic.ds \
			$(inst_localedir)/$$locale; \
	done < SUPPORTED
