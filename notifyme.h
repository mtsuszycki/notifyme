#include <sys/types.h>
#include <utmp.h>
#include <regex.h>
#include "list.h"
#include "config.h"
#include "defs.h"

struct min_idle_host
{
	struct tty_t *tty;	
	time_t min_idle;
};

extern struct list_head entry_l;

#define USER_TYPE	1
#define HOST_TYPE	2
#define TTY_TYPE	3
#define GLOBAL_TYPE	4

#define TYPES_NR	GLOBAL_TYPE+1

#define	IN_MSG		0
#define OUT_MSG		1
#define LAST_MSG	2	/* if it was last login */
#define MSG_NR		3
#define IN_EXEC		0
#define OUT_EXEC	1

#define IN_REPORT	0x1
#define OUT_REPORT	0x2
#define WELCOME_REPORT	0x4
#define IN_BEEP		0x8
#define OUT_BEEP	0x10
#define CENTER_MSG	0x20

struct entry_t
{
	struct entry_t *next;
	struct entry_t *prev;
	int type;		/* type of the key: user,host,tty	*/
	int flags;		/* see defines above			*/
	char *key;		/* can be username, host or tty		*/
	regex_t reg;		/* precompiled key for regex matching	*/
	char *msg[MSG_NR];	/* messages to print taken from config	*/
	int count;		/* current number of logins		*/
	struct min_idle_host h; /* used only at startup to report logins*/
	int hold;		/* number of second to hold a mesasge 	*/
	char *ext_exec[OUT_EXEC+1]; /* external programs to run		*/
};

/* 
 * to check out logouts we have to remember tty
 */
struct tty_t
{
	struct tty_t *next;
	struct tty_t *prev;
	char *tty;		// 
	char *name;		// taken from utmp struct at each login
	char *host;		//		
	struct entry_t *entry;
};

struct list_head 
{ 
	void *next;
	void *prev; 
};

/*
 * Keeps global setting as specified in configuration file.
 */
struct defaults {
	int flags;
	int hold;
	char *msg[MSG_NR];
	char *ext_exec[OUT_EXEC+1];
};

extern struct defaults globals[];

int read_conf(char *);
void show_conf();
void clear_conf(void);
struct entry_t *new_entry(char **, int, int);
#ifndef HAVE_ERRX
void errx(int d, const char *, ...);
#endif

#ifndef HAVE_SNPRINTF
int snprintf(char *, int, const char *, ...);
#endif
