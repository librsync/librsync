#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#if HAVE_MCHECK_H 
#include <mcheck.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <malloc.h>
#include <process.h>
#define getuid() 0
#define geteuid() 0
#endif

#ifdef __NeXT
/* access macros are not declared in non posix mode in unistd.h -
 don't try to use posix on NeXTstep 3.3 ! */
#include <libc.h>
#endif

/* AIX requires this to be the first thing in the file.  */ 
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#elif defined(__GNUC__) && defined(__STRICT_ANSI__)
#define alloca __builtin_alloca
#endif

#ifndef X_OK
#define X_OK 1
#endif

/*@only@*/ char * xstrdup (const char *str);

#if HAVE_MCHECK_H && defined(__GNUC__)
#define	vmefail()	(fprintf(stderr, "virtual memory exhausted.\n"), exit(EXIT_FAILURE), NULL)
#define xstrdup(_str)   (strcpy((malloc(strlen(_str)+1) ? : vmefail()), (_str)))
#elif defined(WIN32)
#define	xstrdup(_str)	_strdup(_str)
#else
#define	xstrdup(_str)	strdup(_str)
#endif  /* HAVE_MCHECK_H && defined(__GNUC__) */


#include "popt.h"
