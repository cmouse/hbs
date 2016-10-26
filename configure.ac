#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(HBS, 1.0, cmouse@desteem.org)
AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADER([config.h])

CFLAGS="-W -Wall -funsigned-char -pipe -Wno-unused $CFLAGS -fvisibility=hidden"

AC_ARG_WITH([libxml2-headers],AS_HELP_STRING([--with-libxml2-headers],[Location of libxml2 header files (as in, contains libxml directory)]), [XML2INC="-I${withval}"], [XML2INC="-I/usr/include/libxml2"])
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Enable debugging symbols]), [CFLAGS="$CFLAGS -g"], [CFLAGS="$CFLAGS -fomit-frame-pointer -O3"])
AC_ARG_ENABLE([helpersnet],AS_HELP_STRING([--enable-helpersnet],[Use helpersnet specific defines]), [CFLAGS="$CFLAGS -DIN_HNET"], [CFLAGS="$CFLAGS -DIN_QNET"])
AC_ARG_ENABLE([calc],AS_HELP_STRING([--enable-calc],[Allow calc command to be built. Uses bc, so potentially dangerous]), [AC_DEFINE([WANT_CALC],[1],[Enable calc command])])
AC_ARG_ENABLE([voice-kick],AS_HELP_STRING([--enable-voice-kick],[Allow voiced people to access kick command]), [AC_DEFINE([ALLOW_VOICE_KICK],[1],[Enable voiced kick])])
CFLAGS="$CFLAGS $XML2INC"
CPPFLAGS="$CPPFLAGS $XML2INC"

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE

TMP_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -pie -fpie"

AC_MSG_CHECKING([Whether $CC accepts -pie -fpie])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[ ]],[[ ]])],
	[AC_MSG_RESULT(yes)],
	[CFLAGS=$TMP_CFLAGS; AC_MSG_RESULT([no])]
)

unset TMP_CFLAGS

TMP_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -fPIC"

AC_MSG_CHECKING([Whether $CC accepts -fPIC])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[ ]],[[ ]])],
	[CFLAGS="$CFLAGS -fPIC"; AC_MSG_RESULT([yes])],
	[CFLAGS=$TMP_CFLAGS; AC_MSG_RESULT([no])]
)

unset TMP_CFLAGS

AC_PROG_MAKE_SET
AC_CHECK_PROG(PAXCTL, [paxctl], [paxctl])
# Checks for libraries.
AC_CHECK_LIB([crypto], [SHA1_Init], , AC_ERROR([libcrypto must be installed]))
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([xml2], [xmlCheckVersion], , AC_ERROR([libxml2 must be installed]))

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h dlfcn.h limits.h malloc.h netdb.h netinet/in.h stdlib.h sys/file.h sys/socket.h sys/time.h unistd.h string.h libxml/parser.h libxml/tree.h openssl/crypto.h openssl/sha.h openssl/md5.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF([unsigned char])
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF([unsigned long])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([endpwent gettimeofday inet_ntoa memmove memset socket strcasecmp strchr strdup strerror strstr mallinfo memcpy sranddev srand rand])

# HBS_CHOOSE_MODULES

AC_CONFIG_FILES([Makefile
commands/Makefile
commands/ban/Makefile
commands/chanmod/Makefile
commands/conwrapper/Makefile
commands/invite/Makefile
commands/kick/Makefile
commands/misc/Makefile
commands/opvoice/Makefile
commands/quiz/Makefile
commands/test/Makefile
commands/topic/Makefile
commands/user/Makefile
commands/whois/Makefile
])
		 
AC_OUTPUT

echo ""
echo "	Please read README	"
echo ""

