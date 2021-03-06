# Don't compile the ctype glue code, since there is no old non-GNU C library.
inhibit-glue = yes

# Additional Syllable specific routines
ifeq ($(subdir),misc)

# Debugging functions
sysdep_routines += dbprintf
sysdep_headers += sys/debug.h

# System information
sysdep_routines += get_system_info xget_system_info get_system_time get_real_time
sysdep_headers += sys/systime.h

# Semaphore & mutexes
sysdep_routines += unlock_semaphore

# Processes & threads
sysdep_routines += tld

# ELF images & libraries
sysdep_routines += image

# Misc system calls for no specific subsystem
sysdep_routines += reboot libc_lock

# Some stubs
sysdep_routines += fsync ftruncate

# Additional headers we need to install
install-others += $(inst_includedir)/bits/syscall.h

# Generate the list of SYS_* macros for the system calls (__NR_* macros).
$(objpfx)syscall-%.h $(objpfx)syscall-%.d: ../sysdeps/unix/sysv/syllable/sys/syscall.h
	rm -f $(@:.h=.d)-t
	{ \
	 echo '/* Generated at libc build time from kernel syscall list.  */';\
	 echo ''; \
	 echo '#ifndef _SYSCALL_H'; \
	 echo '# error "Never use <bits/syscall.h> directly; include <sys/syscall.h> instead."'; \
	 echo '#endif'; \
	 echo ''; \
	 SUNPRO_DEPENDENCIES='$(@:.h=.d)-t $@' \
	 $(CC) -E -x c $(sysincludes) $< -D_LIBC -dM | \
	 sed -n 's@^#define __NR_\([^ ]*\) .*$$@#define SYS_\1 __NR_\1@p'; \
	} > $(@:.d=.h).new
	mv -f $(@:.d=.h).new $(@:.d=.h)
	sed < $(@:.h=.d)-t > $(@:.h=.d)-t2 \
	    -e 's,$(subst .,\.,$@),$(patsubst $(objpfx)%,$$(objpfx)%,\
					      $(@:.d=.h) $(@:.h=.d)),'
	rm -f $(@:.h=.d)-t
	mv -f $(@:.h=.d)-t2 $(@:.h=.d)

$(inst_includedir)/bits/syscall.h: $(objpfx)syscall-list.h $(+force)
	$(make-target-directory)
	if test -r $@ && cmp -s $< $@; \
        then echo 'bits/syscall.h unchanged'; \
        else $(INSTALL_DATA) $< $@; fi

# endif $(subdir),misc
endif

# Ensure we're thread-safe
ifeq ($(subdir),csu)
sysdep_routines += errno-loc
# endif $(subdir),csu
endif

# Stuff we need for POSIX.1
ifeq ($(subdir),posix)
sysdep_routines += uname
# endif $(subdir),posix
endif

# Directory entries
ifeq ($(subdir),dirent)
sysdep_routines += getdents getdents64 getdirentries getdirentries64
# endif $(subdir),dirent
endif

# Internal ELF mangling function stubs
ifeq ($(subdir),elf)
sysdep_routines += dl-stubs
# endif $(subdir),elf
endif

# Sockets & networks
ifeq ($(subdir),socket)
sysdep_routines += sa_len
# endif $(subdir),socket
endif

# Glibc does not require the dynamic linker when running under Syllable
link-libc = $(common-objpfx)libc.so$(libc.so-version) \
	    $(common-objpfx)$(patsubst %,$(libtype.oS),c) $(gnulib)
# This is how to find at build-time things that will be installed there.
rpath-dirs = math elf dlfcn nss nis rt resolv crypt
