/* Determine the platform byte order as _BIG_ENDIAN or _LITTLE_ENDIAN */
 

#ifdef WIN32
#define _LITTLE_ENDIAN   
#endif

#if defined(LINUX) || defined(__CYGWIN__)
#include <endian.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
# ifndef _LITTLE_ENDIAN
# define _LITTLE_ENDIAN
# endif
#elif __BYTE_ORDER == __BIG_ENDIAN
# ifndef _BIG_ENDIAN
# define _BIG_ENDIAN
# endif
#endif

#ifdef NETBSD
#include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
# define _LITTLE_ENDIAN
# elif BYTE_ORDER == BIG_ENDIAN
# define _BIG_ENDIAN
# endif
#endif

#ifdef FREEBSD
# include <sys/param.h>
# include <machine/endian.h>
# if __FreeBSD_version < 500000
#  if BYTE_ORDER == LITTLE_ENDIAN
#   define _LITTLE_ENDIAN
#   elif BYTE_ORDER == BIG_ENDIAN
#   define _BIG_ENDIAN
#  endif
# endif
#endif

#endif