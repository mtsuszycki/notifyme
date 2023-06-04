/* uncomment it if you want messages to be printed in red */
//#define RED_MESSAGE
 
#ifdef HAVE_UTMPX
#define utmp utmpx
#define CUR_UTMP UTMPX_FILE
#define CUR_WTMP WTMPX_FILE
#else
#define CUR_UTMP UTMP_FILE
#define CUR_WTMP WTMP_FILE
#endif

#ifndef UT_NAMESIZE
#define UT_NAMESIZE 32
#endif

#ifndef UT_LINESIZE
#define UT_LINESIZE 32
#endif

#define IN 	1
#define OUT	2

#ifndef ut_name
#define ut_name ut_user
#endif

#ifndef ut_user
#define ut_user ut_name
#endif

#define ut_user	ut_name

