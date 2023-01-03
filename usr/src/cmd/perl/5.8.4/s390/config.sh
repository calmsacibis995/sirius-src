#!/bin/sh
#
# This file was produced by running the Configure script. It holds all the
# definitions figured out by Configure. Should you modify one of these values,
# do not forget to propagate your changes by running "Configure -der". You may
# instead choose to run each of the .SH files by yourself, or "Configure -S".
#

# Package name      : perl5
# Source directory  : .
# Configuration time: 
# Configured by     : perl-bugs
# Target system     : sunos localhost 5.11 sun4u s390x SUNW,Ultra-2

Author=''
Date='$Date'
Header=''
Id='$Id'
Locker=''
Log='$Log'
Mcc='Mcc'
RCSfile='$RCSfile'
Revision='$Revision'
Source=''
State=''
_a='.a'
_exe=''
_o='.o'
afs='false'
afsroot='/afs'
alignbytes='8'
ansi2knr=''
aphostname='/usr/bin/hostname'
api_revision='5'
api_subversion='0'
api_version='8'
api_versionstring='5.8.0'
ar='ar'
archlib='/usr/perl5/5.8.4/lib/s390x-solaris-64int'
archlibexp='/usr/perl5/5.8.4/lib/s390x-solaris-64int'
archname64='64int'
archname='s390x-solaris-64int'
archobjs=''
asctime_r_proto='0'
awk='awk'
baserev='5.0'
bash=''
bin='/usr/perl5/5.8.4/bin'
binexp='/usr/perl5/5.8.4/bin'
bison='bison'
byacc='byacc'
byteorder='87654321'
c='\c'
castflags='0'
cat='cat'
cc='cc'
cccdlflags='-KPIC'
ccdlflags='-R /usr/perl5/5.8.4/lib/s390x-solaris-64int/CORE'
ccflags='-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -xarch=v8 -D_TS_ERRNO'
ccflags_uselargefiles='-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64'
ccname='cc'
ccsymbols=''
ccversion='Sun WorkShop'
cf_by='perl-bugs'
cf_email='perl-bugs@sun.com'
cf_time=''
charsize='1'
chgrp=''
chmod='chmod'
chown=''
clocktype='clock_t'
comm='comm'
compress=''
contains='grep'
cp='cp'
cpio=''
cpp='cpp'
cpp_stuff='42'
cppccsymbols=''
cppflags=''
cpplast='-'
cppminus='-'
cpprun='cc -E'
cppstdin='cc -E'
cppsymbols='_BIG_ENDIAN=1 _FILE_OFFSET_BITS=64 _ILP32=1 _LARGEFILE64_SOURCE=1 _LARGEFILE_SOURCE=1 s390x=1 __s390x=1 __STDC__=1 sun=1 __sun=1 __SVR4=1 unix=1 __unix=1'
crypt_r_proto='0'
cryptlib=''
csh='csh'
ctermid_r_proto='0'
ctime_r_proto='0'
d_Gconvert='gconvert((x),(n),(t),(b))'
d_PRIEUldbl='define'
d_PRIFUldbl='define'
d_PRIGUldbl='define'
d_PRIXU64='define'
d_PRId64='define'
d_PRIeldbl='define'
d_PRIfldbl='define'
d_PRIgldbl='define'
d_PRIi64='define'
d_PRIo64='define'
d_PRIu64='define'
d_PRIx64='define'
d_SCNfldbl='define'
d__fwalk='undef'
d_access='define'
d_accessx='undef'
d_aintl='undef'
d_alarm='define'
d_archlib='define'
d_asctime_r='undef'
d_atolf='undef'
d_atoll='define'
d_attribut='undef'
d_bcmp='define'
d_bcopy='define'
d_bsd='undef'
d_bsdgetpgrp='undef'
d_bsdsetpgrp='undef'
d_bzero='define'
d_casti32='define'
d_castneg='define'
d_charvspr='undef'
d_chown='define'
d_chroot='define'
d_chsize='undef'
d_class='undef'
d_closedir='define'
d_cmsghdr_s='define'
d_const='define'
d_copysignl='define'
d_crypt='define'
d_crypt_r='undef'
d_csh='define'
d_ctermid_r='undef'
d_ctime_r='undef'
d_cuserid='define'
d_dbl_dig='define'
d_dbminitproto='undef'
d_difftime='define'
d_dirfd='undef'
d_dirnamlen='undef'
d_dlerror='define'
d_dlopen='define'
d_dlsymun='undef'
d_dosuid='undef'
d_drand48_r='undef'
d_drand48proto='define'
d_dup2='define'
d_eaccess='undef'
d_endgrent='define'
d_endgrent_r='undef'
d_endhent='define'
d_endhostent_r='undef'
d_endnent='define'
d_endnetent_r='undef'
d_endpent='define'
d_endprotoent_r='undef'
d_endpwent='define'
d_endpwent_r='undef'
d_endsent='define'
d_endservent_r='undef'
d_eofnblk='define'
d_eunice='undef'
d_faststdio='define'
d_fchdir='define'
d_fchmod='define'
d_fchown='define'
d_fcntl='define'
d_fcntl_can_lock='define'
d_fd_macros='define'
d_fd_set='define'
d_fds_bits='define'
d_fgetpos='define'
d_finite='define'
d_finitel='undef'
d_flexfnam='define'
d_flock='undef'
d_flockproto='undef'
d_fork='define'
d_fp_class='undef'
d_fpathconf='define'
d_fpclass='define'
d_fpclassify='undef'
d_fpclassl='undef'
d_fpos64_t='define'
d_frexpl='define'
d_fs_data_s='undef'
d_fseeko='define'
d_fsetpos='define'
d_fstatfs='define'
d_fstatvfs='define'
d_fsync='define'
d_ftello='define'
d_ftime='undef'
d_getcwd='define'
d_getespwnam='undef'
d_getfsstat='undef'
d_getgrent='define'
d_getgrent_r='undef'
d_getgrgid_r='undef'
d_getgrnam_r='undef'
d_getgrps='define'
d_gethbyaddr='define'
d_gethbyname='define'
d_gethent='define'
d_gethname='define'
d_gethostbyaddr_r='undef'
d_gethostbyname_r='undef'
d_gethostent_r='undef'
d_gethostprotos='define'
d_getitimer='define'
d_getlogin='define'
d_getlogin_r='undef'
d_getmnt='undef'
d_getmntent='define'
d_getnbyaddr='define'
d_getnbyname='define'
d_getnent='define'
d_getnetbyaddr_r='undef'
d_getnetbyname_r='undef'
d_getnetent_r='undef'
d_getnetprotos='define'
d_getpagsz='define'
d_getpbyname='define'
d_getpbynumber='define'
d_getpent='define'
d_getpgid='define'
d_getpgrp2='undef'
d_getpgrp='define'
d_getppid='define'
d_getprior='define'
d_getprotobyname_r='undef'
d_getprotobynumber_r='undef'
d_getprotoent_r='undef'
d_getprotoprotos='define'
d_getprpwnam='undef'
d_getpwent='define'
d_getpwent_r='undef'
d_getpwnam_r='undef'
d_getpwuid_r='undef'
d_getsbyname='define'
d_getsbyport='define'
d_getsent='define'
d_getservbyname_r='undef'
d_getservbyport_r='undef'
d_getservent_r='undef'
d_getservprotos='define'
d_getspnam='define'
d_getspnam_r='undef'
d_gettimeod='define'
d_gmtime_r='undef'
d_gnulibc='undef'
d_grpasswd='define'
d_hasmntopt='define'
d_htonl='define'
d_ilogbl='define'
d_index='undef'
d_inetaton='undef'
d_int64_t='define'
d_isascii='define'
d_isfinite='undef'
d_isinf='undef'
d_isnan='define'
d_isnanl='define'
d_killpg='define'
d_lchown='define'
d_ldbl_dig='define'
d_link='define'
d_localtime_r='undef'
d_locconv='define'
d_lockf='define'
d_longdbl='define'
d_longlong='define'
d_lseekproto='define'
d_lstat='define'
d_madvise='define'
d_mblen='define'
d_mbstowcs='define'
d_mbtowc='define'
d_memchr='define'
d_memcmp='define'
d_memcpy='define'
d_memmove='define'
d_memset='define'
d_mkdir='define'
d_mkdtemp='undef'
d_mkfifo='define'
d_mkstemp='define'
d_mkstemps='undef'
d_mktime='define'
d_mmap='define'
d_modfl='define'
d_modfl_pow32_bug='undef'
d_modflproto='undef'
d_mprotect='define'
d_msg='define'
d_msg_ctrunc='define'
d_msg_dontroute='define'
d_msg_oob='define'
d_msg_peek='define'
d_msg_proxy='undef'
d_msgctl='define'
d_msgget='define'
d_msghdr_s='define'
d_msgrcv='define'
d_msgsnd='define'
d_msync='define'
d_munmap='define'
d_mymalloc='undef'
d_nice='define'
d_nl_langinfo='define'
d_nv_preserves_uv='undef'
d_off64_t='define'
d_old_pthread_create_joinable='undef'
d_oldpthreads='undef'
d_oldsock='undef'
d_open3='define'
d_pathconf='define'
d_pause='define'
d_perl_otherlibdirs='undef'
d_phostname='undef'
d_pipe='define'
d_poll='define'
d_portable='define'
d_procselfexe='undef'
d_pthread_atfork='define'
d_pthread_attr_setscope='define'
d_pthread_yield='undef'
d_pwage='define'
d_pwchange='undef'
d_pwclass='undef'
d_pwcomment='define'
d_pwexpire='undef'
d_pwgecos='define'
d_pwpasswd='define'
d_pwquota='undef'
d_qgcvt='define'
d_quad='define'
d_random_r='undef'
d_readdir64_r='undef'
d_readdir='define'
d_readdir_r='undef'
d_readlink='define'
d_readv='define'
d_recvmsg='define'
d_rename='define'
d_rewinddir='define'
d_rmdir='define'
d_safebcpy='undef'
d_safemcpy='undef'
d_sanemcmp='define'
d_sbrkproto='define'
d_scalbnl='define'
d_sched_yield='undef'
d_scm_rights='define'
d_seekdir='define'
d_select='define'
d_sem='define'
d_semctl='define'
d_semctl_semid_ds='undef'
d_semctl_semun='define'
d_semget='define'
d_semop='define'
d_sendmsg='define'
d_setegid='define'
d_seteuid='define'
d_setgrent='define'
d_setgrent_r='undef'
d_setgrps='define'
d_sethent='define'
d_sethostent_r='undef'
d_setitimer='define'
d_setlinebuf='define'
d_setlocale='define'
d_setlocale_r='undef'
d_setnent='define'
d_setnetent_r='undef'
d_setpent='define'
d_setpgid='define'
d_setpgrp2='undef'
d_setpgrp='define'
d_setprior='define'
d_setproctitle='undef'
d_setprotoent_r='undef'
d_setpwent='define'
d_setpwent_r='undef'
d_setregid='define'
d_setresgid='undef'
d_setresuid='undef'
d_setreuid='define'
d_setrgid='undef'
d_setruid='undef'
d_setsent='define'
d_setservent_r='undef'
d_setsid='define'
d_setvbuf='define'
d_sfio='undef'
d_shm='define'
d_shmat='define'
d_shmatprototype='define'
d_shmctl='define'
d_shmdt='define'
d_shmget='define'
d_sigaction='define'
d_sigprocmask='define'
d_sigsetjmp='define'
d_sockatmark='define'
d_sockatmarkproto='define'
d_socket='define'
d_socklen_t='define'
d_sockpair='define'
d_socks5_init='undef'
d_sqrtl='define'
d_srand48_r='undef'
d_srandom_r='undef'
d_sresgproto='undef'
d_sresuproto='undef'
d_statblks='define'
d_statfs_f_flags='undef'
d_statfs_s='define'
d_statvfs='define'
d_stdio_cnt_lval='define'
d_stdio_ptr_lval='define'
d_stdio_ptr_lval_nochange_cnt='define'
d_stdio_ptr_lval_sets_cnt='undef'
d_stdio_stream_array='define'
d_stdiobase='define'
d_stdstdio='define'
d_strchr='define'
d_strcoll='define'
d_strctcpy='define'
d_strerrm='strerror(e)'
d_strerror='define'
d_strerror_r='undef'
d_strftime='define'
d_strtod='define'
d_strtol='define'
d_strtold='define'
d_strtoll='define'
d_strtoq='undef'
d_strtoul='define'
d_strtoull='define'
d_strtouq='undef'
d_strxfrm='define'
d_suidsafe='define'
d_symlink='define'
d_syscall='define'
d_syscallproto='undef'
d_sysconf='define'
d_sysernlst=''
d_syserrlst='define'
d_system='define'
d_tcgetpgrp='define'
d_tcsetpgrp='define'
d_telldir='define'
d_telldirproto='define'
d_time='define'
d_times='define'
d_tm_tm_gmtoff='undef'
d_tm_tm_zone='undef'
d_tmpnam_r='undef'
d_truncate='define'
d_ttyname_r='undef'
d_tzname='define'
d_u32align='define'
d_ualarm='define'
d_umask='define'
d_uname='define'
d_union_semun='undef'
d_unordered='define'
d_usleep='define'
d_usleepproto='define'
d_ustat='define'
d_vendorarch='define'
d_vendorbin='define'
d_vendorlib='define'
d_vendorscript='define'
d_vfork='undef'
d_void_closedir='undef'
d_voidsig='define'
d_voidtty=''
d_volatile='define'
d_vprintf='define'
d_wait4='define'
d_waitpid='define'
d_wcstombs='define'
d_wctomb='define'
d_writev='define'
d_xenix='undef'
date='date'
db_hashtype='u_int32_t'
db_prefixtype='size_t'
db_version_major=''
db_version_minor=''
db_version_patch=''
defvoidused='15'
direntrytype='struct dirent'
dlext='so'
dlsrc='dl_dlopen.xs'
doublesize='8'
drand01='drand48()'
drand48_r_proto='0'
dynamic_ext='B ByteLoader Cwd Data/Dumper Devel/DProf Devel/PPPort Devel/Peek Digest/MD5 Encode Fcntl File/Glob Filter/Util/Call I18N/Langinfo IO IPC/SysV List/Util MIME/Base64 NDBM_File ODBM_File Opcode POSIX PerlIO/encoding PerlIO/scalar PerlIO/via SDBM_File Socket Storable Sys/Hostname Sys/Syslog Time/HiRes Unicode/Normalize XS/APItest XS/Typemap attrs re'
eagain='EAGAIN'
ebcdic='undef'
echo='echo'
egrep='egrep'
emacs=''
endgrent_r_proto='0'
endhostent_r_proto='0'
endnetent_r_proto='0'
endprotoent_r_proto='0'
endpwent_r_proto='0'
endservent_r_proto='0'
eunicefix=':'
exe_ext=''
expr='expr'
extensions='B ByteLoader Cwd Data/Dumper Devel/DProf Devel/PPPort Devel/Peek Digest/MD5 Encode Fcntl File/Glob Filter/Util/Call I18N/Langinfo IO IPC/SysV List/Util MIME/Base64 NDBM_File ODBM_File Opcode POSIX PerlIO/encoding PerlIO/scalar PerlIO/via SDBM_File Socket Storable Sys/Hostname Sys/Syslog Time/HiRes Unicode/Normalize XS/APItest XS/Typemap attrs re Errno'
extras=''
fflushNULL='define'
fflushall='undef'
find=''
firstmakefile='makefile'
flex=''
fpossize='8'
fpostype='fpos_t'
freetype='void'
from=':'
full_ar='/usr/ccs/bin/ar'
full_csh='/usr/bin/csh'
full_sed='/usr/bin/sed'
gccansipedantic=''
gccosandvers=''
gccversion=''
getgrent_r_proto='0'
getgrgid_r_proto='0'
getgrnam_r_proto='0'
gethostbyaddr_r_proto='0'
gethostbyname_r_proto='0'
gethostent_r_proto='0'
getlogin_r_proto='0'
getnetbyaddr_r_proto='0'
getnetbyname_r_proto='0'
getnetent_r_proto='0'
getprotobyname_r_proto='0'
getprotobynumber_r_proto='0'
getprotoent_r_proto='0'
getpwent_r_proto='0'
getpwnam_r_proto='0'
getpwuid_r_proto='0'
getservbyname_r_proto='0'
getservbyport_r_proto='0'
getservent_r_proto='0'
getspnam_r_proto='0'
gidformat='"ld"'
gidsign='-1'
gidsize='4'
gidtype='gid_t'
glibpth='/usr/shlib /usr/lib /usr/lib/386 /lib/386 /usr/ccs/lib /usr/local/lib'
gmake='gmake'
gmtime_r_proto='0'
gnulibc_version=''
grep='grep'
groupcat='cat /etc/group'
groupstype='gid_t'
gzip='gzip'
h_fcntl='true'
h_sysfile='false'
hint='recommended'
hostcat='cat /etc/hosts'
html1dir=''
html1direxp=''
html3dir=''
html3direxp=''
i16size='2'
i16type='short'
i32size='4'
i32type='long'
i64size='8'
i64type='long long'
i8size='1'
i8type='char'
i_arpainet='define'
i_bsdioctl=''
i_crypt='define'
i_db='undef'
i_dbm='undef'
i_dirent='define'
i_dld='undef'
i_dlfcn='define'
i_fcntl='define'
i_float='define'
i_fp='undef'
i_fp_class='undef'
i_gdbm='undef'
i_grp='define'
i_ieeefp='define'
i_inttypes='define'
i_langinfo='define'
i_libutil='undef'
i_limits='define'
i_locale='define'
i_machcthr='undef'
i_malloc='define'
i_math='define'
i_memory='undef'
i_mntent='undef'
i_ndbm='define'
i_netdb='define'
i_neterrno='undef'
i_netinettcp='define'
i_niin='define'
i_poll='define'
i_prot='undef'
i_pthread='define'
i_pwd='define'
i_rpcsvcdbm='define'
i_sfio='undef'
i_sgtty='undef'
i_shadow='define'
i_socks='undef'
i_stdarg='define'
i_stddef='define'
i_stdlib='define'
i_string='define'
i_sunmath='define'
i_sysaccess='undef'
i_sysdir='undef'
i_sysfile='undef'
i_sysfilio='define'
i_sysin='undef'
i_sysioctl='define'
i_syslog='define'
i_sysmman='define'
i_sysmode='define'
i_sysmount='define'
i_sysndir='undef'
i_sysparam='define'
i_sysresrc='define'
i_syssecrt='undef'
i_sysselct='define'
i_syssockio='define'
i_sysstat='define'
i_sysstatfs='define'
i_sysstatvfs='define'
i_systime='define'
i_systimek='undef'
i_systimes='define'
i_systypes='define'
i_sysuio='define'
i_sysun='define'
i_sysutsname='define'
i_sysvfs='define'
i_syswait='define'
i_termio='undef'
i_termios='define'
i_time='undef'
i_unistd='define'
i_ustat='define'
i_utime='define'
i_values='define'
i_varargs='undef'
i_varhdr='stdarg.h'
i_vfork='undef'
ignore_versioned_solibs=''
inc_version_list=''
inc_version_list_init='0'
incpath=''
inews=''
installarchlib='/usr/perl5/5.8.4/lib/s390x-solaris-64int'
installbin='/usr/perl5/5.8.4/bin'
installhtml1dir=''
installhtml3dir=''
installman1dir='/usr/perl5/5.8.4/man/man1'
installman3dir='/usr/perl5/5.8.4/man/man3'
installprefix='/usr/perl5/5.8.4'
installprefixexp='/usr/perl5/5.8.4'
installprivlib='/usr/perl5/5.8.4/lib'
installscript='/usr/perl5/5.8.4/bin'
installsitearch='/usr/perl5/site_perl/5.8.4/s390x-solaris-64int'
installsitebin='/usr/perl5/5.8.4/bin'
installsitehtml1dir=''
installsitehtml3dir=''
installsitelib='/usr/perl5/site_perl/5.8.4'
installsiteman1dir='/usr/perl5/5.8.4/man/man1'
installsiteman3dir='/usr/perl5/5.8.4/man/man3'
installsitescript='/usr/perl5/5.8.4/bin'
installstyle='lib'
installusrbinperl='undef'
installvendorarch='/usr/perl5/vendor_perl/5.8.4/s390x-solaris-64int'
installvendorbin='/usr/perl5/5.8.4/bin'
installvendorhtml1dir=''
installvendorhtml3dir=''
installvendorlib='/usr/perl5/vendor_perl/5.8.4'
installvendorman1dir='/usr/perl5/5.8.4/man/man1'
installvendorman3dir='/usr/perl5/5.8.4/man/man3'
installvendorscript='/usr/perl5/5.8.4/bin'
intsize='4'
issymlink='test -h'
ivdformat='"lld"'
ivsize='8'
ivtype='long long'
known_extensions='B ByteLoader Cwd DB_File Data/Dumper Devel/DProf Devel/PPPort Devel/Peek Digest/MD5 Encode Fcntl File/Glob Filter/Util/Call GDBM_File I18N/Langinfo IO IPC/SysV List/Util MIME/Base64 NDBM_File ODBM_File Opcode POSIX PerlIO/encoding PerlIO/scalar PerlIO/via SDBM_File Socket Storable Sys/Hostname Sys/Syslog Thread Time/HiRes Unicode/Normalize XS/APItest XS/Typemap attrs re'
ksh=''
ld='cc'
lddlflags='-G'
ldflags=''
ldflags_uselargefiles=''
ldlibpthname='LD_LIBRARY_PATH'
less='less'
lib_ext='.a'
libc='/lib/libc.so'
libperl='libperl.so'
libpth='/lib /usr/lib /usr/ccs/lib'
libs='-lsocket -lnsl -ldl -lm -lc'
libsdirs='/usr/lib'
libsfiles='libsocket.so.1 libnsl.so.1 libdl.so.1 libm.so.2 libc.so.1'
libsfound='/usr/lib/libsocket.so.1 /usr/lib/libnsl.so.1 /usr/lib/libdl.so.1 /usr/lib/libm.so.2 /usr/lib/libc.so.1'
libspath='/lib /usr/lib /usr/ccs/lib'
libswanted='sfio socket bind inet nsl nm ndbm gdbm dbm db dl dld sun m util c cposix posix bsd BSD'
libswanted_uselargefiles=''
line=''
lint=''
lkflags=''
ln='ln'
lns='/usr/bin/ln -s'
localtime_r_proto='0'
locincpth='/usr/sfw/include /opt/sfw/include /usr/local/include /opt/local/include /usr/gnu/include /opt/gnu/include /usr/GNU/include /opt/GNU/include'
loclibpth='/usr/sfw/lib /opt/sfw/lib /usr/lib /usr/ccs/lib /usr/local/lib /opt/local/lib /usr/gnu/lib /opt/gnu/lib /usr/GNU/lib /opt/GNU/lib'
longdblsize='16'
longlongsize='8'
longsize='4'
lp=''
lpr=''
ls='ls'
lseeksize='8'
lseektype='off_t'
mail=''
mailx=''
make='make'
make_set_make='#'
mallocobj=''
mallocsrc=''
malloctype='void *'
man1dir='/usr/perl5/5.8.4/man/man1'
man1direxp='/usr/perl5/5.8.4/man/man1'
man1ext='1'
man3dir='/usr/perl5/5.8.4/man/man3'
man3direxp='/usr/perl5/5.8.4/man/man3'
man3ext='3'
mips_type=''
mistrustnm='run'
mkdir='mkdir'
mmaptype='caddr_t'
modetype='mode_t'
more='more'
multiarch='undef'
mv=''
myarchname='s390x-solaris'
mydomain='.sun.com'
myhostname='localhost'
myuname='sunos localhost 5.11 sun4u s390x SUNW,Ultra-2'
n=''
need_va_copy='undef'
netdb_hlen_type='size_t'
netdb_host_type='const void *'
netdb_name_type='const char *'
netdb_net_type='in_addr_t'
nm='nm'
nm_opt='-p'
nm_so_opt=''
nonxs_ext='Errno'
nroff='nroff'
nvEUformat='"E"'
nvFUformat='"F"'
nvGUformat='"G"'
nv_preserves_uv_bits='53'
nveformat='"e"'
nvfformat='"f"'
nvgformat='"g"'
nvsize='8'
nvtype='double'
o_nonblock='O_NONBLOCK'
obj_ext='.o'
old_pthread_create_joinable=''
optimize='-xO3 -xspace -xildoff'
orderlib='false'
osname='solaris'
osvers='2.11'
otherlibdirs=''
package='perl5'
pager='/usr/bin/more'
passcat='cat /etc/passwd'
patchlevel='8'
path_sep=':'
perl5='/bin/perl'
perl=''
perl_patchlevel=''
perladmin='perl-bugs@sun.com'
perllibs='-lsocket -lnsl -ldl -lm -lc'
perlpath='/usr/perl5/5.8.4/bin/perl'
pg='pg'
phostname='hostname'
pidtype='pid_t'
plibpth=''
pmake=''
pr=''
prefix='/usr/perl5/5.8.4'
prefixexp='/usr/perl5/5.8.4'
privlib='/usr/perl5/5.8.4/lib'
privlibexp='/usr/perl5/5.8.4/lib'
procselfexe=''
prototype='define'
ptrsize='4'
quadkind='3'
quadtype='long long'
randbits='48'
randfunc='drand48'
random_r_proto='0'
randseedtype='long'
ranlib=':'
rd_nodata='-1'
readdir64_r_proto='0'
readdir_r_proto='0'
revision='5'
rm='rm'
rmail=''
run=''
runnm='true'
sPRIEUldbl='"LE"'
sPRIFUldbl='"LF"'
sPRIGUldbl='"LG"'
sPRIXU64='"llX"'
sPRId64='"lld"'
sPRIeldbl='"Le"'
sPRIfldbl='"Lf"'
sPRIgldbl='"Lg"'
sPRIi64='"lli"'
sPRIo64='"llo"'
sPRIu64='"llu"'
sPRIx64='"llx"'
sSCNfldbl='"Lf"'
sched_yield='undef'
scriptdir='/usr/perl5/5.8.4/bin'
scriptdirexp='/usr/perl5/5.8.4/bin'
sed='sed'
seedfunc='srand48'
selectminbits='32'
selecttype='fd_set *'
sendmail=''
setgrent_r_proto='0'
sethostent_r_proto='0'
setlocale_r_proto='0'
setnetent_r_proto='0'
setprotoent_r_proto='0'
setpwent_r_proto='0'
setservent_r_proto='0'
sh='/bin/sh'
shar=''
sharpbang='#!'
shmattype='void *'
shortsize='2'
shrpenv=''
shsharp='true'
sig_count='49'
sig_name='ZERO HUP INT QUIT ILL TRAP ABRT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM USR1 USR2 CHLD PWR WINCH URG IO STOP TSTP CONT TTIN TTOU VTALRM PROF XCPU XFSZ WAITING LWP FREEZE THAW CANCEL LOST XRES JVM1 JVM2 RTMIN NUM42 NUM43 NUM44 NUM45 NUM46 NUM47 RTMAX IOT CLD POLL'
sig_name_init='"ZERO", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM", "USR1", "USR2", "CHLD", "PWR", "WINCH", "URG", "IO", "STOP", "TSTP", "CONT", "TTIN", "TTOU", "VTALRM", "PROF", "XCPU", "XFSZ", "WAITING", "LWP", "FREEZE", "THAW", "CANCEL", "LOST", "XRES", "JVM1", "JVM2", "RTMIN", "NUM42", "NUM43", "NUM44", "NUM45", "NUM46", "NUM47", "RTMAX", "IOT", "CLD", "POLL", 0'
sig_num='0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 6 18 22'
sig_num_init='0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 6, 18, 22, 0'
sig_size='52'
signal_t='void'
sitearch='/usr/perl5/site_perl/5.8.4/s390x-solaris-64int'
sitearchexp='/usr/perl5/site_perl/5.8.4/s390x-solaris-64int'
sitebin='/usr/perl5/5.8.4/bin'
sitebinexp='/usr/perl5/5.8.4/bin'
sitehtml1dir=''
sitehtml1direxp=''
sitehtml3dir=''
sitehtml3direxp=''
sitelib='/usr/perl5/site_perl/5.8.4'
sitelib_stem='/usr/perl5/site_perl'
sitelibexp='/usr/perl5/site_perl/5.8.4'
siteman1dir='/usr/perl5/5.8.4/man/man1'
siteman1direxp='/usr/perl5/5.8.4/man/man1'
siteman3dir='/usr/perl5/5.8.4/man/man3'
siteman3direxp='/usr/perl5/5.8.4/man/man3'
siteprefix='/usr/perl5/5.8.4'
siteprefixexp='/usr/perl5/5.8.4'
sitescript='/usr/perl5/5.8.4/bin'
sitescriptexp='/usr/perl5/5.8.4/bin'
sizesize='4'
sizetype='size_t'
sleep=''
smail=''
so='so'
sockethdr=''
socketlib=''
socksizetype='int'
sort='sort'
spackage='Perl5'
spitshell='cat'
srand48_r_proto='0'
srandom_r_proto='0'
src='.'
ssizetype='ssize_t'
startperl='#!/usr/perl5/5.8.4/bin/perl'
startsh='#!/bin/sh'
static_ext=''
stdchar='unsigned char'
stdio_base='((fp)->_base)'
stdio_bufsiz='((fp)->_cnt + (fp)->_ptr - (fp)->_base)'
stdio_cnt='((fp)->_cnt)'
stdio_filbuf=''
stdio_ptr='((fp)->_ptr)'
stdio_stream_array='__iob'
strerror_r_proto='0'
strings='/usr/include/string.h'
submit=''
subversion='4'
sysman='/usr/share/man/man1'
tail=''
tar=''
targetarch=''
tbl=''
tee=''
test='test'
timeincl='/usr/include/sys/time.h'
timetype='time_t'
tmpnam_r_proto='0'
to=':'
touch='touch'
tr='tr'
trnl='\n'
troff=''
ttyname_r_proto='0'
u16size='2'
u16type='unsigned short'
u32size='4'
u32type='unsigned long'
u64size='8'
u64type='unsigned long long'
u8size='1'
u8type='unsigned char'
uidformat='"ld"'
uidsign='-1'
uidsize='4'
uidtype='uid_t'
uname='uname'
uniq='uniq'
uquadtype='unsigned long long'
use5005threads='undef'
use64bitall='undef'
use64bitint='define'
usecrosscompile='undef'
usedl='define'
usefaststdio='define'
useithreads='undef'
uselargefiles='define'
uselongdouble='undef'
usemallocwrap='define'
usemorebits='undef'
usemultiplicity='undef'
usemymalloc='n'
usenm='true'
useopcode='true'
useperlio='define'
useposix='true'
usereentrant='undef'
usesfio='false'
useshrplib='true'
usesocks='undef'
usethreads='undef'
usevendorprefix='define'
usevfork='false'
usrinc='/usr/include'
uuname=''
uvXUformat='"llX"'
uvoformat='"llo"'
uvsize='8'
uvtype='unsigned long long'
uvuformat='"llu"'
uvxformat='"llx"'
vendorarch='/usr/perl5/vendor_perl/5.8.4/s390x-solaris-64int'
vendorarchexp='/usr/perl5/vendor_perl/5.8.4/s390x-solaris-64int'
vendorbin='/usr/perl5/5.8.4/bin'
vendorbinexp='/usr/perl5/5.8.4/bin'
vendorhtml1dir=''
vendorhtml1direxp=''
vendorhtml3dir=''
vendorhtml3direxp=''
vendorlib='/usr/perl5/vendor_perl/5.8.4'
vendorlib_stem='/usr/perl5/vendor_perl'
vendorlibexp='/usr/perl5/vendor_perl/5.8.4'
vendorman1dir='/usr/perl5/5.8.4/man/man1'
vendorman1direxp='/usr/perl5/5.8.4/man/man1'
vendorman3dir='/usr/perl5/5.8.4/man/man3'
vendorman3direxp='/usr/perl5/5.8.4/man/man3'
vendorprefix='/usr/perl5/5.8.4'
vendorprefixexp='/usr/perl5/5.8.4'
vendorscript='/usr/perl5/5.8.4/bin'
vendorscriptexp='/usr/perl5/5.8.4/bin'
version='5.8.4'
version_patchlevel_string='version 8 subversion 4'
versiononly='undef'
vi=''
voidflags='15'
xlibpth='/usr/lib/386 /lib/386'
yacc='yacc'
yaccflags=''
zcat=''
zip='zip'
# Configure command line arguments.
config_arg0='./Configure'
config_args=''
config_argc=0
PERL_REVISION=5
PERL_VERSION=8
PERL_SUBVERSION=4
PERL_API_REVISION=5
PERL_API_VERSION=8
PERL_API_SUBVERSION=0
PERL_PATCHLEVEL=
PERL_CONFIG_SH=true