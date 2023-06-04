#include <sys/ioctl.h>
#include <sys/termios.h> 
#include <sys/types.h>
#include <strings.h>  
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>
#include "notifyme.h"

#ifdef HAVE_ERRX
#include <err.h>
#endif

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif

#define MIN(a,b)  ( (a) < (b) ? (a) : (b))

char *resfile;

static int lflag = 1;		/* should I report logouts?		*/
static int qflag = 1;		/* quiet flag				*/
static int center_flag;		/* whether message should be centered	*/
static int who_flag = 0;	/* Added by SK: Should notifyme exit
				 * after showing who is online now and
				 * keep on monitoring 
				 */

struct list_head entry_l = { &entry_l, &entry_l };
struct list_head tty_l = { &tty_l, &tty_l };

static int lastmsg_len = 1;
unsigned int cols, rows;

void erase(int);
void echo(char *, int);

/* message that should be hold on the screen for some time */
char *hold_msg; 
unsigned int hold_remain;
unsigned int center_hold;

void usage()
{
	printf("\nnotifyme [-c conf] [-C] [-l] [-q] [-w] [-h] [USERNAME]...\n");
	printf("-c conf\t\tspecify alternate configuration\n");
	printf("-C\t\tforce to print messages in the center of the screen\n");
	printf("-l\t\tdon't report logouts\n");
	printf("-q\t\tquiet - no beep with message\n");
	printf("-w\t\texit after printing current users. No monitoring\n");
	printf("-h\t\thelp - this message\n");
	printf("Rest of the arguments are usernames to watch.\n\n");
	printf("USR1 signal can be used to re-read resource file.\n\n");
} 	

time_t get_idle (char *tty)
{	
	struct stat st;
	char buf[32];
	/* Removed as sparc doesnt support it */
	snprintf(buf, sizeof buf, "/dev/%s",tty);
	if (stat(buf,&st) == -1) return -1;
	return (time(0) - st.st_atime);
}	

char *count_idle(time_t idle_time)
{
	static char buf[32];	
	
	if (idle_time >= 3600 * 24) 
		sprintf(buf,"%dd", (int) idle_time/(3600 * 24) );
	else if (idle_time >= 3600){
		time_t min = (idle_time % 3600) / 60;
		if (min < 10)
			sprintf(buf,"%d:0%d",(int) idle_time/3600, (int) min);
		else
			sprintf(buf,"%d:%d", (int) idle_time/3600, (int) min);
	}
	else if (idle_time >= 60)
		sprintf(buf,"%d minute%s", (int) idle_time/60,
			idle_time<120?"":"s");
	else
		sprintf(buf,"no");
	return buf;
}

static char *cp_string(char *s, int len)
{
	char *d;
	if(!s) return 0;
	d = calloc(1, len + 1);
	if(!d) errx(1, "Cannot allocate memory.");
	strncpy(d, s, len);
	return d;
}

struct tty_t *new_tty(struct utmp *u, struct entry_t *e)
{
	struct tty_t *t;
	t = calloc(1, sizeof *t);
	if(!t) errx(1, "Cannot allocate memory. Exiting.");
	t->name = cp_string(u->ut_user, sizeof(u->ut_user));
	t->tty = cp_string(u->ut_line, sizeof(u->ut_line));
	t->host = cp_string(u->ut_host, sizeof(u->ut_host));
	t->entry = e;
	return t;
}

static int regmatch(struct entry_t *t, char *d)
{
	int err;
	err = regexec(&t->reg, d, 0, 0, 0);
#ifdef DEBUG
printf(__FUNCTION__": %s %s %s\n", t->key, d, err?"non match":"match");
#endif 
	return !err;
}

static int match_entry(struct entry_t *e, struct utmp *u)
{
	char *s = 0;
	int len, result;
	char *x = strchr(e->key, '*');

	switch(e->type) {
		case USER_TYPE: 
			s = cp_string(u->ut_user, sizeof(u->ut_user)); 
			break;
		case HOST_TYPE: 
			s = cp_string(u->ut_host, sizeof(u->ut_host)); 
			break;
		case TTY_TYPE: 
			s = cp_string(u->ut_line, sizeof(u->ut_line)); 
			break;
	}
	if (!s || !*s) return 0;
	if (x) len = x - e->key;
	else len = strlen(s); 

#ifdef DEBUG
printf(__FUNCTION__": %s -> %s (%d) - %d count: %d\n", 
	e->key, s, len, !strncmp(e->key, s, len), e->count);
#endif
	result = regmatch(e, s);
	free(s);
	return result;
}

/*
 * Get informations about users currently on the machine
 */
void read_utmp()
{
	int fd;
	struct utmp ent;
	struct entry_t *e;
	struct tty_t *t;
	time_t idle;

#ifdef HAVE_SETUTXENT
	setutxent();
#endif
	
	if (!(fd = open(CUR_UTMP,O_RDONLY)))
		errx(1, "Cannot open " CUR_UTMP ". Exiting.");
	while(read(fd, &ent, sizeof ent) > 0){
#ifdef USER_PROCESS
		if(ent.ut_type != USER_PROCESS) continue;
#else
		if(ent.ut_user[0] == 0) continue;
#endif

		/* we're checking logged in users */
		for_each(e, entry_l){
			if(!match_entry(e, &ent)) continue;
			t = new_tty(&ent, e);
			addto_list(t, tty_l);
			idle = get_idle(ent.ut_line);
			
			e->h.min_idle = e->count?MIN(e->h.min_idle, idle):idle;
			if(e->h.min_idle == idle) 
				e->h.tty = t;
			e->count++;
			break;
		}
	}
	close(fd);
}

/* 
 * Report users currently logged onto the machine.
 */
static void report_users()
{
	int i = 0;
	struct entry_t *e;
	for_each(e, entry_l) {

#ifdef DEBUG
printf(__FUNCTION__ ": %d\n", e->flags);
#endif

		if(!(e->flags & WELCOME_REPORT)) continue;
		if(!(e->h.tty)) continue;	
		if(!i++) printf("\nUsers currently on the machine:\n");
		printf("%-8.9s (%d login", e->h.tty->name, e->count);
		printf("%s", e->count==1?"":"s");
		printf(", %s idle time from %s)\n", 
			count_idle(e->h.min_idle),
			e->h.tty->host[0]?e->h.tty->host:"console"
			);
	}
	if(i) printf("\n");
}

void set_cols()
{
	struct winsize win;
	if(ioctl(1, TIOCGWINSZ, &win) != -1) { 
		cols = win.ws_col + 1;
		rows = win.ws_row + 1;
	}
	else {
		cols = 80;
		rows = 25;
	}
}

void print_msg(char *s, int beep, int center)
{
	erase(lastmsg_len);
	echo(s, center_flag || center);
	if(qflag && beep) printf("\a");
	lastmsg_len = strlen(s);
	fflush(stdout);
}

struct fmt {
	char c;
	char *s;
};

static char *def_msg[] = {
	"%u from %h arrived!",
	"%u logged out.",
	"%u - no more logins."
};	

static char *choose_msg(struct entry_t *e, int type)
{
	char *s = e->msg[type];
	if(s) return s;
	s = globals[e->type-1].msg[type];
	if(s) return s;
	s = globals[GLOBAL_TYPE-1].msg[type];
	if(s) return s;
	return def_msg[type];
}

/* 
 * Expand special characters (%u, %t, %h)
 */
static char *prepare_msg(struct tty_t *t, char *s)
{
	static char buf[64];
	struct fmt fmts[] = { 
		{ 'u', t->name },
		{ 't', t->tty },
		{ 'h', t->host }
	};
	int i, z, x;
	bzero(buf, sizeof buf);
	for(i = 0, z = 0; z < sizeof buf - 1 && s[i]; i++) {
		if(s[i] != '%') {
			buf[z++] = s[i];
			continue;
		}
		i++;
		for(x = 0; x < sizeof fmts/sizeof(struct fmt); x++) {
			int len;
			if(s[i] != fmts[x].c) continue;
			len = snprintf(buf+z, sizeof buf - z, 
					"%s", fmts[x].s);
			if(len != strlen(fmts[x].s)) return buf;
			z += len;
		}
	}
	return buf;
}

void reread()
{
	clear_conf();
	entry_l.next = &entry_l;
	entry_l.prev = &entry_l;
	tty_l.next = &tty_l;
	tty_l.prev = &tty_l;
	read_conf(resfile);
	read_utmp();
	/* SK Change */
	report_users();
	signal(SIGUSR1, reread);
}

static void free_tty_t(struct tty_t *t)
{
	free(t->name);
	free(t->tty);
	free(t->host);
	free(t);
}

/* 
 * Check if current message should be holded on the screen 
 */
static void hold_mark(struct entry_t *t, char *msg)
{
	if(!t->hold) hold_msg = 0;
	else {
		hold_msg = msg;
		hold_remain = t->hold;
		center_hold = t->flags & CENTER_MSG;
	}
}

static char *choose_exec(struct entry_t *e, int type)
{
	char *s = e->ext_exec[type];
#ifdef DEBUG
printf(__FUNCTION__": %d\n" , type);
#endif
	if(s) return s;
	s = globals[e->type-1].ext_exec[type];
	if(s) return s;
	s = globals[GLOBAL_TYPE-1].ext_exec[type];
	if(s) return s;
	return 0;
}

void no_zombie()
{
	wait(0);
}

static char *make_arg(char **s, char *b, char *e)
{
	*s = calloc(e-b+1, sizeof(char));
	snprintf(*s, e-b+1, "%s", b);
	return *s;
}

#define MAX_ARGS	32

/* 
 * Prepare args and execute external program.
 */
static void do_exec(char *s)
{
	int i = 0;
	char *args[MAX_ARGS], *prev = s;	
	bzero(args, sizeof args);
	while(*s) {
		if(!prev && *s != ' ' && *s != '\t') {
			 prev = s++;
			 continue;
		}
		if(i == MAX_ARGS - 1) break;
		if(*s == ' ' || *s == '\t') {
			if(prev) make_arg(&args[i++], prev, s);
			prev = 0;
		}
		s++;
	}
	if(prev) make_arg(&args[i++], prev, s);
#ifdef DEBUG
printf(__FUNCTION__ ": executing %s\n", args[0]);	
#endif
	execv(args[0], args);
	/* if for some reason exec failed then just exit */
	exit(0);
}

static void prg_exec(struct entry_t *e, struct tty_t *t, int type)
{
	int i;
	char *s, *tmp;
	tmp = choose_exec(e, type);
#ifdef DEBUG
printf(__FUNCTION__ ": entering ...\n");
#endif
	if(!tmp) return;
#ifdef DEBUG
printf(__FUNCTION__ ": ok passed - %s\n", e->ext_exec[type]);
#endif
	s =  prepare_msg(t, tmp);
	if(!s) return;
	i = fork();
	if(i == -1) errx(-1, "Cannot fork");
	if(i) return;
#ifdef DEBUG
printf(__FUNCTION__ ": executing %s\n", s);	
#endif
	do_exec(s);
}

static void check_logout(struct utmp *ut)
{
	char *msg;
	int msg_type = OUT_MSG, found = 0;
	struct tty_t *t = tty_l.next;
	while(t != (void *)&tty_l) {
		if(strncmp(t->tty, ut->ut_line, UT_LINESIZE)) {
			t = t->next;
			continue;
		}
		found = 1;	
		t->entry->count--;
		if(!(t->entry->flags & OUT_REPORT) || !lflag)
			break;
		if(!t->entry->count) msg_type = LAST_MSG; 
		prg_exec(t->entry, t, OUT_EXEC);
		msg = prepare_msg(t, choose_msg(t->entry, msg_type));
		print_msg(msg, t->entry->flags & OUT_BEEP, t->entry->flags & CENTER_MSG);
		hold_mark(t->entry, msg);
		break;
	}
	if(!found) return;
	delfrom_list(t, tty_l);
	free_tty_t(t);
}

static void check_login(struct utmp *ut)
{
	struct entry_t *e;
	char *msg;
	struct tty_t *t;
	for_each(e, entry_l) {
		if(!match_entry(e, ut)) continue;
		e->count++;
		t = new_tty(ut, e);
		addto_list(t, tty_l);	
		if(!(e->flags & IN_REPORT)) return;
		prg_exec(e, t, IN_EXEC);
		msg = prepare_msg(t, choose_msg(t->entry, IN_MSG));
		print_msg(msg, e->flags & IN_BEEP, e->flags & CENTER_MSG);
		hold_mark(e, msg);
		break;
	}
}

/*
 * If there is a message to hold, then print it. Otherwise just wait 3 sec.
 * Used in a main loop.
 */
static void hold_check()
{
	int i;
	int tmp = 0;
	if(!hold_msg) {
		sleep(3);
		return;
	}
	for(i = 0; i < 6; i++) {
		if(!hold_remain) {
			hold_msg = 0;
			return;
		}
		if(!tmp) hold_remain--;
		print_msg(hold_msg, 0, center_hold);
		usleep(500000);
		tmp ^= 1;
	}
}


int main (int argc, char *argv[])
{
	int f, c, v, argcount = 1;
	int prnt;
	struct utmp ut;
	char  *env = 0;
	char rcfile[256];
	
	while((c = getopt(argc, argv, "Cc:hlqw")) != -1) {
		switch(c) {
		case 'q':		/* to beep or not to beep */
			qflag = 0;
			argcount++;
			break;
		case 'w':
			who_flag=1;
			argcount++;
			break;	
		case 'c':		/* alternate config file */
			resfile = optarg;
			argcount += 2;
			break;
		case 'C':
			center_flag = 1;
			argcount++;
			break;
		case 'l':		/* don't report logouts */
			lflag = 0;
			argcount++;
			break;
		case 'h':
			usage();
			exit(0);
		default:
			printf("notifyme -h for help\n");
			exit(0);
		}
	}
	while(argcount < argc) {
		char *s;
		s = argv[argcount];
		/* set to global type to force flag settings */
		new_entry(&s, GLOBAL_TYPE, 1);
		argcount++;
	}	
	if (!isatty(1)) exit(0);
	if ((f = open(CUR_WTMP, O_RDONLY)) < 0){
		printf ("Cannot open " CUR_WTMP "\n");
		exit (0);
	}
	bzero(rcfile, sizeof rcfile);
	if(!resfile && !(env = getenv("HOME")))
		errx(1, "HOME is not set");
	
	if(!resfile) {
		snprintf(rcfile, sizeof rcfile - 1,
			"%s/.notifyrc",env);
		resfile = rcfile;
	}
	if (!read_conf(resfile) && IS_EMPTY(&entry_l)) {
		char *fake = ".*";
		/* create entry that matches all logins and logouts */ 
		new_entry(&fake, GLOBAL_TYPE, 1);
	}	

#ifdef DEBUG
	show_conf();
#endif
	read_utmp();
	if(lseek(f, 0, SEEK_END) == -1)
		errx(1,"Cannot lseek " CUR_UTMP);
	report_users();
	if(who_flag) {
#ifdef DEBUG
		printf("Exiting after showing current users. 'w' option supplied\n");
#endif
		exit(0);
	}
	prnt = getppid();
#ifndef DEBUG
	switch(v = fork()){
		case -1: 
			errx(1, "Cannot fork.");
		case 0 :
			setpgid(getpid(), prnt);
			break;
		default: exit (0);
	}
#endif	
	set_cols();

	signal(SIGWINCH, set_cols);
	signal(SIGUSR1, reread);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCHLD, no_zombie);

	for(;;) {
		while(!read(f, &ut, sizeof ut)) hold_check(); 
		if(!ttyname(1)) exit(0);
#ifdef USER_PROCESS
		if(ut.ut_type != USER_PROCESS &&
			ut.ut_type != DEAD_PROCESS) continue;
		if(ut.ut_type == DEAD_PROCESS) {
#else
		if(ut.ut_user[0] == 0) {
#endif
		/* user log out */
			check_logout(&ut);
			continue;
		}
		/* user just logged in */
		check_login(&ut);
	}
	return (0);
}
