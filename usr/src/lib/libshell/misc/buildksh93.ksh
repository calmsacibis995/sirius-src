#!/bin/ksh

#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"
#

#
# buildksh93.ksh - ast-ksh standalone build script for the 
# OpenSolaris ksh93-integration project
#

# ksh93s+ beta sources can be downloaded like this from the AT&T site:
# wget --http-user="I accept www.opensource.org/licenses/cpl" --http-passwd="." 'http://www.research.att.com/sw/download/beta/INIT.2007-04-18.tgz'
# wget --http-user="I accept www.opensource.org/licenses/cpl" --http-passwd="." 'http://www.research.att.com/sw/download/beta/ast-ksh.2007-04-18.tgz'

function fatal_error
{
    printf "${0}: %s\n" "${1}" >&2
    exit 1
}

buildmode="$1"

if [ "${buildmode}" = "" ] ; then
    fatal_error "buildmode required."
fi

set -e -x

# make sure we use the C locale during building to avoid any unintended
# side-effects
export LANG=C
export LC_ALL=$LANG LC_MONETARY=$LANG LC_NUMERIC=$LANG LC_MESSAGES=$LANG LC_COLLATE=$LANG LC_CTYPE=$LANG

function print_solaris_builtin_header
{
cat <<ENDOFTEXT
/* POSIX compatible commands */
#ifdef _NOT_YET
#define XPG6CMDLIST(f)   { "/usr/xpg6/bin/" #f, NV_BLTIN|NV_NOFREE, bltin(f) },
#define XPG4CMDLIST(f)   { "/usr/xpg4/bin/" #f, NV_BLTIN|NV_NOFREE, bltin(f) },
#else
#define XPG6CMDLIST(f)
#define XPG4CMDLIST(f)
#endif /* NOT_YET */
/* Commands which are 100% compatible with native Solaris versions (/bin is
 * a softlink to ./usr/bin so both need to be listed here) */
#define BINCMDLIST(f)	 { "/bin/" #f,  	NV_BLTIN|NV_NOFREE, bltin(f) },
/* Make all ksh93 builtins accessible when /usr/ast/bin was added to ${PATH} */
#define ASTCMDLIST(f)	 { "/usr/ast/bin/" #f,  NV_BLTIN|NV_NOFREE, bltin(f) },

/* undo ast_map.h #defines to avoid collision */
#undef basename
#undef dirname

/* Generated data, do not edit. */
XPG4CMDLIST(basename)
ASTCMDLIST(basename)
BINCMDLIST(cat)
ASTCMDLIST(cat)
XPG4CMDLIST(chgrp)
ASTCMDLIST(chgrp)
ASTCMDLIST(chmod)
XPG4CMDLIST(chown)
BINCMDLIST(chown)
ASTCMDLIST(chown)
ASTCMDLIST(cmp)
ASTCMDLIST(comm)
XPG4CMDLIST(cp)
ASTCMDLIST(cp)
ASTCMDLIST(cut)
XPG4CMDLIST(date)
ASTCMDLIST(date)
ASTCMDLIST(dirname)
XPG4CMDLIST(expr)
ASTCMDLIST(expr)
ASTCMDLIST(fds)
ASTCMDLIST(fmt)
ASTCMDLIST(fold)
BINCMDLIST(head)
ASTCMDLIST(head)
XPG4CMDLIST(id)
ASTCMDLIST(id)
ASTCMDLIST(join)
XPG4CMDLIST(ln)
ASTCMDLIST(ln)
ASTCMDLIST(logname)
BINCMDLIST(mkdir)
ASTCMDLIST(mkdir)
ASTCMDLIST(mkfifo)
XPG4CMDLIST(mv)
ASTCMDLIST(mv)
ASTCMDLIST(paste)
ASTCMDLIST(pathchk)
ASTCMDLIST(rev)
XPG4CMDLIST(rm)
ASTCMDLIST(rm)
BINCMDLIST(rmdir)
ASTCMDLIST(rmdir)
XPG4CMDLIST(stty)
ASTCMDLIST(stty)
XPG4CMDLIST(tail)
ASTCMDLIST(tail)
BINCMDLIST(tee)
ASTCMDLIST(tee)
ASTCMDLIST(tty)
ASTCMDLIST(uname)
BINCMDLIST(uniq)
ASTCMDLIST(uniq)
BINCMDLIST(wc)
ASTCMDLIST(wc)

/* Mandatory for ksh93 test suite and AST scripts */
BINCMDLIST(getconf)

ENDOFTEXT
}

function build_shell
{
    set -e -x

    # OS.cputype.XXbit.compiler
    case "${buildmode}" in
        *.linux.*)
	    # ksh93+AST config flags
	    BAST_FLAGS="-DSHOPT_CMDLIB_BLTIN=0 -DSH_CMDLIB_DIR=\\\"/usr/ast/bin\\\" -DSHOPT_SYSRC -D_map_libc=1"
	    
            # gcc flags
	    BGCC99="gcc -std=gnu99"
            BGCC_CCFLAGS="${BON_FLAGS} ${BAST_FLAGS}"

            case "${buildmode}" in
                # Linux i386
                *.i386.32bit.gcc*)  HOSTTYPE="linux.i386" CC="${BGCC99} -fPIC" CC_SHAREDLIB="-shared" CCFLAGS="${BGCC_CCFLAGS}"
                    ;;
                *)
                    fatal_error "build_shell: Illegal Linux type/compiler build mode \"${buildmode}\"."
                    ;;
            esac
            ;;
        *.solaris.*)
            # Notes:
	    # 1. Do not remove/modify these flags or their order before either
	    # asking the project leads at
	    # http://www.opensolaris.org/os/project/ksh93-integration/
	    # These flags all have a purpose, even if they look
	    # weird/redundant/etc. at the first look.
	    #
	    # 2. We use -KPIC here since -Kpic is too small on 64bit sparc and
            # on 32bit it's close to the barrier so we use it for both 32bit and
            # 64bit to avoid later suprises when people update libast in the
            # future
	    #
            # 3. "-D_map_libc=1" is needed to force map.c to add a "_ast_" prefix to all
            # AST symbol names which may otherwise collide with Solaris/Linux libc
            #
            # 4. "-DSHOPT_SYSRC" enables /etc/ksh.kshrc support (AST default is currently
            # to enable it if /etc/ksh.kshrc or /etc/bash.bashrc are available on the
            # build machine).
            #
	    # 5. -D_lib_socket=1 -lsocket -lnsl" was added to make sure ksh93 is compiled
            # with networking support enabled, the current AST build infratructure has
            # problems with detecting networking support in Solaris.
	    #
	    # 6. "-xc99=%all -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1" is used to force
	    # the compiler into C99 mode. Otherwise ksh93 will be much slower and lacks
	    # lots of arithmethic functions.
	    #
	    # 7. "-D_TS_ERRNO -D_REENTRANT" are flags taken from the default OS/Net
	    # build system.
            #
            # 8. "-xpagesize_stack=64K -xpagesize_heap=64K" is used on SPARC to
            # enhance the performace
	    #
	    # 9. -DSHOPT_CMDLIB_BLTIN=0 -DSH_CMDLIB_DIR=\\\"/usr/ast/bin\\\" -DSHOPT_CMDLIB_HDR=\\\"/home/test001/ksh93/ast_ksh_20070322/solaris_cmdlist.h\\\"
	    # is used to bind all ksh93 builtins to a "virtual" directory
	    # called "/usr/ast/bin/" and to adjust the list of builtins
	    # enabled by default to those defined by PSARC 2006/550
	    
	    solaris_builtin_header="$PWD/tmp_solaris_builtin_header.h"
	    print_solaris_builtin_header >"${solaris_builtin_header}"
	    
	    # OS/Net build flags
	    BON_FLAGS="-D_TS_ERRNO -D_REENTRANT"
	    
	    # ksh93+AST config flags
	    BAST_FLAGS="-DSHOPT_CMDLIB_BLTIN=0 -DSH_CMDLIB_DIR=\\\"/usr/ast/bin\\\" -DSHOPT_CMDLIB_HDR=\\\"${solaris_builtin_header}\\\" -DSHOPT_SYSRC -D_map_libc=1"
	    
	    # Sun Studio flags
	    BSUNCC99="/opt/SUNWspro/bin/cc -xc99=%all -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1"
            BSUNCC_APP_CCFLAGS_SPARC="-xpagesize_stack=64K -xpagesize_heap=64K" # use BSUNCC_APP_CCFLAGS_SPARC only for final executables
            BSUNCC_CCFLAGS="${BON_FLAGS} -KPIC -g -xs -xspace -Xa -xstrconst -z combreloc -xildoff -errtags=yes ${BAST_FLAGS} -D_lib_socket=1 -lsocket -lnsl"

            # gcc flags
	    BGCC99="/usr/sfw/bin/gcc -std=gnu99 -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1"
            BGCC_CCFLAGS="${BON_FLAGS} ${BAST_FLAGS} -D_lib_socket=1 -lsocket -lnsl"
 

            case "${buildmode}" in
                *.i386.32bit.suncc*)  HOSTTYPE="sol11.i386" CC="${BSUNCC99}"                    CC_SHAREDLIB="-G" CCFLAGS="${BSUNCC_CCFLAGS}"  ;;
                *.i386.64bit.suncc*)  HOSTTYPE="sol11.i386" CC="${BSUNCC99} -xarch=amd64 -KPIC" CC_SHAREDLIB="-G" CCFLAGS="${BSUNCC_CCFLAGS}"  ;;
                *.sparc.32bit.suncc*) HOSTTYPE="sol11.sun4" CC="${BSUNCC99}"                    CC_SHAREDLIB="-G" CCFLAGS="${BSUNCC_CCFLAGS}" BSUNCC_APP_CCFLAGS="${BSUNCC_APP_CCFLAGS_SPARC}" ;;
                *.sparc.64bit.suncc*) HOSTTYPE="sol11.sun4" CC="${BSUNCC99} -xarch=v9 -KPIC"    CC_SHAREDLIB="-G" CCFLAGS="${BSUNCC_CCFLAGS}" BSUNCC_APP_CCFLAGS="${BSUNCC_APP_CCFLAGS_SPARC}" ;;

                *.i386.32bit.gcc*)  HOSTTYPE="sol11.i386" CC="${BGCC99} -fPIC"                                            CC_SHAREDLIB="-shared" CCFLAGS="${BGCC_CCFLAGS}"  ;;
                *.i386.64bit.gcc*)  HOSTTYPE="sol11.i386" CC="${BGCC99} -m64 -mtune=opteron -Ui386 -U__i386 -fPIC"        CC_SHAREDLIB="-shared" CCFLAGS="${BGCC_CCFLAGS}"  ;;
                *.sparc.32bit.gcc*) HOSTTYPE="sol11.sun4" CC="${BGCC99} -m32 -mcpu=v8 -fPIC "                             CC_SHAREDLIB="-shared" CCFLAGS="${BGCC_CCFLAGS}"  ;;
                *.sparc.64bit.gcc*) HOSTTYPE="sol11.sun4" CC="${BGCC99} -m64 -mcpu=v9 -fPIC"                              CC_SHAREDLIB="-shared" CCFLAGS="${BGCC_CCFLAGS}"  ;;

                *)
                    fatal_error "build_shell: Illegal Solaris type/compiler build mode \"${buildmode}\"."
                    ;;
            esac
            ;;
        *)
            fatal_error "Illegal OS build mode \"${buildmode}\"."
            ;;
    esac

    # some prechecks
    [ -z "${CCFLAGS}"  ] && fatal_error "build_shell: CCFLAGS is empty."
    [ -z "${CC}"       ] && fatal_error "build_shell: CC is empty."
    [ -z "${HOSTTYPE}" ] && fatal_error "build_shell: HOSTTYPE is empty."
    [ ! -f "bin/package" ] && fatal_error "build_shell: bin/package missing."
    [ ! -x "bin/package" ] && fatal_error "build_shell: bin/package not executable."

    export CCFLAGS CC HOSTTYPE

    # build ksh93
    bin/package make CCFLAGS="${CCFLAGS}" CC="${CC}" HOSTTYPE="${HOSTTYPE}"

    root="${PWD}/arch/${HOSTTYPE}"
    test -d "$root" || fatal_error "build_shell: directory ${root} not found."
    log="${root}/lib/package/gen/make.out"

    test -s $log || fatal_error "build_shell: no make.out log found."

    if [[ "${buildmode}" != *.staticshell* ]] ; then
        # libcmd causes some trouble since there is a squatter in solaris
	# This has been fixed in Solaris 11/B48 but may require adjustments
	# for older Solaris releases
        for lib in libast libdll libcmd libshell ; do
            test $? -eq 0 || exit 1
            case "$lib" in
            libshell)
                base="lib/"
                vers=1
                link="-L${root}/lib/ -lcmd -ldll -last -lm"
                ;;
            libdll)
                base="src/lib/${lib}"
                vers=1
                link="-ldl"
                ;;
            libast)
                base="src/lib/${lib}"
                vers=1
                link="-lm"
                ;;
            *)
                base="src/lib/${lib}"
                vers=1
                link="-L${root}/lib/ -last -lm"
                ;;
            esac

            (
            cd "${root}/${base}"
            if [[ "${buildmode}" = *solaris* ]] ; then
                ${CC} ${CC_SHAREDLIB} ${CCFLAGS} -Wl,-zallextract -Wl,-zmuldefs -o "${root}/lib/${lib}.so.${vers}" "${lib}.a"  $link
            else
                ${CC} ${CC_SHAREDLIB} ${CCFLAGS} -Wl,--whole-archive -Wl,-zmuldefs "${lib}.a" -Wl,--no-whole-archive -o "${root}/lib/${lib}.so.${vers}" $link
	    fi
	   
            #rm ${lib}.a
            mv "${lib}.a" "disabled_${lib}.a_"

            cd "${root}/lib"
            ln -sf "${lib}.so.${vers}" "${lib}.so"
            )
        done

        (
          base=src/cmd/ksh93
          cd ${root}/${base}
          rm -f ${root}/lib/libshell.a
          rm -f ${root}/lib/libdll.a
          rm -f ${root}/lib/libast.a

          if [[ "${buildmode}" = *solaris* ]] ; then
              ${CC} ${CCFLAGS} ${BSUNCC_APP_CCFLAGS} -L${root}/lib/ -o ksh pmain.o -lshell -Bstatic -lcmd -Bdynamic -ldll -last -lm -lsecdb
          else
              ${CC} ${CCFLAGS} ${BSUNCC_APP_CCFLAGS} -L${root}/lib/ -o ksh pmain.o -lshell -lcmd -ldll -last -lm
	  fi
	  
          ldd ksh
        )
    fi
}

function test_shell
{
    set -e -x

    export SHELL="$(ls -1 $PWD/arch/*/src/cmd/ksh93/ksh)"
    export LD_LIBRARY_PATH="$(ls -1ad $PWD/arch/*/lib):${LD_LIBRARY_PATH}"
    export LD_LIBRARY_PATH_32="$(ls -1ad $PWD/arch/*/lib):${LD_LIBRARY_PATH_32}"
    export LD_LIBRARY_PATH_64="$(ls -1ad $PWD/arch/*/lib):${LD_LIBRARY_PATH_64}"
    print "## SHELL is |${SHELL}|"
    print "## LD_LIBRARY_PATH is |${LD_LIBRARY_PATH}|"
    
    [ ! -f "${SHELL}" ] && fatal_error "test_shell: |${SHELL}| is not a file."
    [ ! -x "${SHELL}" ] && fatal_error "test_shell: |${SHELL}| is not executable."

    case "${buildmode}" in
            testshell.bcheck*)
        	for i in ./src/cmd/ksh93/tests/*.sh ; do 
		    bc_logfile="$(basename "$i").$$.bcheck"
		    rm -f "${bc_logfile}"
                    /opt/SUNWspro/bin/bcheck -q -access -o "${bc_logfile}" ${SHELL} ./src/cmd/ksh93/tests/shtests \
		        LD_LIBRARY_PATH_64="$LD_LIBRARY_PATH_64" \
			LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
			LD_LIBRARY_PATH_32="$LD_LIBRARY_PATH_32"\
			LANG=C LC_ALL=C \
			"$i"
                    cat "${bc_logfile}"
        	done
	        ;;
	    testshell)
        	for i in ./src/cmd/ksh93/tests/*.sh ; do 
                    ${SHELL} ./src/cmd/ksh93/tests/shtests \
		        LD_LIBRARY_PATH_64="$LD_LIBRARY_PATH_64" \
			LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
			LD_LIBRARY_PATH_32="$LD_LIBRARY_PATH_32"\
			LANG=C LC_ALL=C \
			"$i"
        	done
		;;
    esac
}

# main
case "${buildmode}" in
        build.*) build_shell ;;
        testshell*)  test_shell  ;;
        *) fatal_error "Illegal build mode \"${buildmode}\"." ;;
esac
# EOF.
