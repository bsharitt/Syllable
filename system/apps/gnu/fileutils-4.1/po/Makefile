# -*- Makefile -*- for program source directory in GNU NLS utilities package.
# Copyright (C) 1995, 1996, 1997, 2000 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file file be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

PACKAGE = fileutils
VERSION = 4.1

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix = ${prefix}
datadir = $(prefix)/share
localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale
gettextsrcdir = $(prefix)/share/gettext/po
subdir = po

INSTALL = /usr/local/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
MKINSTALLDIRS = /source/Syllable/0.3.9/system/apps/gnu/fileutils/mkinstalldirs

CC = gcc
GENCAT = 
GMSGFMT = PATH=../src:$$PATH no
MSGFMT = no
XGETTEXT = PATH=../src:$$PATH :
MSGMERGE = PATH=../src:$$PATH msgmerge

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g -O2
CPPFLAGS = 

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

SOURCES = cat-id-tbl.c
POFILES =  cs.po da.po de.po el.po es.po fr.po gl.po it.po ja.po ko.po nl.po no.po pl.po pt.po pt_BR.po ru.po sk.po sl.po sv.po zh.po
GMOFILES =  cs.gmo da.gmo de.gmo el.gmo es.gmo fr.gmo gl.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sl.gmo sv.gmo zh.gmo
DISTFILES = Makefile.in.in POTFILES.in $(PACKAGE).pot \
stamp-cat-id $(POFILES) $(GMOFILES) $(SOURCES)

POTFILES = \
	../lib/argmatch.c \
	../lib/closeout.c \
	../lib/error.c \
	../lib/getopt.c \
	../lib/human.c \
	../lib/long-options.c \
	../lib/makepath.c \
	../lib/obstack.c \
	../lib/quotearg.c \
	../lib/rpmatch.c \
	../lib/same.c \
	../lib/unicodeio.c \
	../lib/userspec.c \
	../lib/version-etc.c \
	../lib/xmalloc.c \
	../src/chgrp.c \
	../src/chmod.c \
	../src/chown-core.c \
	../src/chown.c \
	../src/copy.c \
	../src/cp.c \
	../src/dircolors.c \
	../src/dd.c \
	../src/df.c \
	../src/du.c \
	../src/install.c \
	../src/ln.c \
	../src/ls.c \
	../src/mkdir.c \
	../src/mkfifo.c \
	../src/mknod.c \
	../src/mv.c \
	../src/remove.c \
	../src/rm.c \
	../src/rmdir.c \
	../src/shred.c \
	../src/sync.c \
	../src/touch.c

CATALOGS =  cs.gmo da.gmo de.gmo el.gmo es.gmo fr.gmo gl.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sl.gmo sv.gmo zh.gmo
CATOBJEXT = .gmo
INSTOBJEXT = .mo

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo .msg .cat

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=$(srcdir)/`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<

.po.cat:
	sed -f ../intl/po2msg.sed < $< > $*.msg \
	  && rm -f $@ && $(GENCAT) $@ $*.msg


all: all-yes

all-yes: cat-id-tbl.c $(CATALOGS)
all-no:

$(srcdir)/$(PACKAGE).pot: $(POTFILES)
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(top_srcdir) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --files-from=$(srcdir)/POTFILES.in \
	&& test ! -f $(PACKAGE).po \
	   || ( rm -f $(srcdir)/$(PACKAGE).pot \
		&& mv $(PACKAGE).po $(srcdir)/$(PACKAGE).pot )

$(srcdir)/cat-id-tbl.c: stamp-cat-id; @:
$(srcdir)/stamp-cat-id: $(PACKAGE).pot
	rm -f cat-id-tbl.tmp
	sed -f ../intl/po2tbl.sed $(srcdir)/$(PACKAGE).pot \
		| sed -e "s/@PACKAGE NAME@/$(PACKAGE)/" > cat-id-tbl.tmp
	if cmp -s cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; then \
	  rm cat-id-tbl.tmp; \
	else \
	  echo cat-id-tbl.c changed; \
	  rm -f $(srcdir)/cat-id-tbl.c; \
	  mv cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; \
	fi
	cd $(srcdir) && rm -f stamp-cat-id && echo timestamp > stamp-cat-id


install: install-exec install-data
install-exec:
install-data: install-data-yes
install-data-no: all
install-data-yes: all
	if test -x "$(MKINSTALLDIRS)"; then \
	  $(MKINSTALLDIRS) $(DESTDIR)$(datadir); \
	else \
	  $(SHELL) $(top_srcdir)/mkinstalldirs $(DESTDIR)$(datadir); \
	fi
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  case "$$cat" in \
	    *.gmo) destdir=$(DESTDIR)$(gnulocaledir);; \
	    *)     destdir=$(DESTDIR)$(localedir);; \
	  esac; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  dir=$$destdir/$$lang/LC_MESSAGES; \
	  if test -r "$(MKINSTALLDIRS)"; then \
	    $(MKINSTALLDIRS) $$dir; \
	  else \
	    $(SHELL) $(top_srcdir)/mkinstalldirs $$dir; \
	  fi; \
	  if test -r $$cat; then \
	    $(INSTALL_DATA) $$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $$cat as $$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $(srcdir)/$$cat as" \
		 "$$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  fi; \
	  if test -r $$cat.m; then \
	    $(INSTALL_DATA) $$cat.m $$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	    echo "installing $$cat.m as $$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	  else \
	    if test -r $(srcdir)/$$cat.m ; then \
	      $(INSTALL_DATA) $(srcdir)/$$cat.m \
		$$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	      echo "installing $(srcdir)/$$cat as" \
		   "$$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	    else \
	      true; \
	    fi; \
	  fi; \
	  timedir=$$destdir/$$lang/LC_TIME; \
	  rm -fr $$timedir; \
	  ln -s LC_MESSAGES $$timedir \
	  || (mkdir $$timedir && ln $$dir/* $$timedir); \
	  echo "installing $$timedir as an alias for $$dir"; \
	done
	if test "$(PACKAGE)" = "gettext"; then \
	  if test -x "$(MKINSTALLDIRS)"; then \
	    $(MKINSTALLDIRS) $(DESTDIR)$(gettextsrcdir); \
	  else \
	    $(SHELL) $(top_srcdir)/mkinstalldirs $(DESTDIR)$(gettextsrcdir); \
	  fi; \
	  $(INSTALL_DATA) $(srcdir)/Makefile.in.in \
			  $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  for dir in $(localedir) $(gnulocaledir); do \
	    for lc in LC_MESSAGES LC_TIME; do \
	      for x in '' .m; do \
		rm -f $(DESTDIR)$$dir/$$lang/$$lc/$(PACKAGE)$(INSTOBJEXT)$$x; \
	      done; \
	    done; \
	  done; \
	done

check: all

cat-id-tbl.o: ../intl/libgettext.h

dvi info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(PACKAGE).po *.old.po cat-id-tbl.tmp
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo *.msg *.cat *.cat.m

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir: update-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  cp -p $(srcdir)/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(PACKAGE).pot
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $(srcdir)/$$lang.po \
	                 $(srcdir)/$(PACKAGE).pot -o $$lang.new.po; then \
	    if cmp $$lang.new.po $(srcdir)/$$lang.po >/dev/null; then \
	      echo "$$lang.po is unchanged"; \
	      rm -f $$lang.new.po; \
	    else \
	      echo "updating $$lang.po"; \
	      rm -f $(srcdir)/$$lang.po; \
	      mv $$lang.new.po $(srcdir)/$$lang.po; \
	    fi; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.new.po; \
	    exit 1; \
	  fi; \
	done

POTFILES: POTFILES.in
	( if test 'x$(srcdir)' != 'x.'; then \
	    posrcprefix='$(top_srcdir)/'; \
	  else \
	    posrcprefix="../"; \
	  fi; \
	  rm -f $@-t $@ \
	    && (sed -e '/^#/d' -e '/^[ 	]*$$/d' \
		    -e "s@.*@	$$posrcprefix& \\\\@" < $(srcdir)/$@.in \
		| sed -e '$$s/\\$$//') > $@-t \
	    && chmod a-w $@-t \
	    && mv $@-t $@ )

Makefile: Makefile.in.in ../config.status POTFILES
	cd .. \
	  && CONFIG_FILES=$(subdir)/$@.in CONFIG_HEADERS= \
	       $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

# Tell version 3.79 and up of GNU make to not build goals in this
# directory in parallel.
.NOTPARALLEL:
