#include "config_file.h"
#include <stdarg.h>

extern struct list_head tty_l;


#ifndef HAVE_ERRX
void errx(int d, const char *t, ...)
{
	va_list ap;
	fprintf(stderr, "\n");
	va_start(ap, t);
	vfprintf(stderr, t, ap); 
	va_end(ap);
	fprintf(stderr, "\n");
	exit(d);
}
#endif

// change me!
#ifndef HAVE_SNPRINTF
int snprintf(char *str, int n, const char *t, ...)
{
	va_list ap;
	va_start(ap, t);
	return vsprintf(str, t, ap);
	
}
#endif

static int valid_sep(char c)
{
	return  c == ' ' || c == '\t';
}

static int boolean(char *s, int *val, int f) 
{ 
	int ret = 1;
	ret = strncasecmp(s, "yes", 3);
	if(!ret) {
		*val |= f;
		return ret;
	}
	else *val &= ~f;
	return strncasecmp(s, "no", 2);
}

static int int_val(char *s, int *v)
{
	char *p = s;
	int i = 0;
	while(isdigit((int)*p)) 
		i = 10 * i + *(p++) - '0';
	*v = i;
#ifdef DEBUG
	printf(__FUNCTION__ ": %s, %d\n", s, *v);
#endif	
	if(p == s) return 1;
	return 0;	
}

static int token(char *s, char **val)
{
	char *p;
	p = strchr(s, '"');
	if(p) for(p++; *p != '"'  &&  *p != '\n' ; p++) ;
	else for(p = s; !valid_sep(*p) &&  *p != '\n' ; p++) ;

	if(p == s++) return 1;
	*val = realloc(*val, (p - s) * sizeof (char) + 1);
	if(!*val) errx(1, ": cannot allocate memory. Exiting.");
	bzero(*val, p - s + 1);
	strncpy(*val, s, p - s);
	return 0;
}

static inline void skip_white(char **s)
{
	for(;**s == ' ' || **s == '\t'; (*s)++);
} 

char *strnchr(char *s, int i, char c)
{
	int p;
	for(p = 0; *s && p < i; s++, p++)
		if(*s == c) return s;
	return 0;
} 	

struct defaults globals[TYPES_NR];

struct group {
	char *s;
	int type;
};
	
struct group groups[] = { 
	{"users:", USER_TYPE },
	{"hosts:", HOST_TYPE },
	{"ttys:", TTY_TYPE }
};

struct config_item config[] = { 
	{"Logins", -1 , BOOLEAN, 0, IN_REPORT },
	{"Logouts", -1 , BOOLEAN, 0, OUT_REPORT },
	{"LoginBeep",  -1 , BOOLEAN, 0, IN_BEEP },
	{"LogoutBeep",  -1 , BOOLEAN, 0, OUT_BEEP },
	{"PrintWelcome", -1 , BOOLEAN, 0, WELCOME_REPORT },
	{"LoginMsg", IN_MSG , STRING, 0, 0 },
	{"LogoutMsg", OUT_MSG , STRING, 0, 0 },
	{"LastMsg", LAST_MSG , STRING, 0, 0 },
	{"HoldMsg", -1 , INT, 0, 0 },
	{"CenterMsg", -1, BOOLEAN, 0, CENTER_MSG },
	{"LoginExec", IN_EXEC , STRING, 0, 0 },
	{"LogoutExec", OUT_EXEC , STRING, 0, 0 }

};

static int find_char(char **s)
{
	for(;**s == ' ' || **s == '\t'; (*s)++);
	if(**s == '\n' || !**s) return 0;
	return 1;
}

static int type_change(char *s, int *type)
{
	int i;
	for(i = 0; i < sizeof groups/sizeof(struct group); i++) {
		if(!strncmp(groups[i].s, s, strlen(groups[i].s))) { 
			*type = groups[i].type;
			globals[i].flags = globals[GLOBAL_TYPE-1].flags;
			globals[i].hold = globals[GLOBAL_TYPE-1].hold;
			return 1;
		}
	}	
	return 0;
}

static void comp_regex(struct entry_t *t, int line)
{
	char erbuf[64];
	int err;
	if (!t) return;
	err = regcomp(&t->reg, t->key, REG_EXTENDED);
	if (err) {
		regerror(err, &t->reg, erbuf, sizeof erbuf);
		errx(1, "Invalid regex around %d line: %s", line, erbuf);
	}
}

static int read_option(char *p, int type, struct entry_t *t, int line)
{
	int len, found, i, index, error;
	int *flags, *int_type;
	char **msg, **exec_str;

	len = found = 0;
	for (i = 0; i < sizeof config/sizeof (struct config_item) ; i++) {
		len = strlen(config[i].t);
		if(strncasecmp(p, config[i].t, len)) {
			found = 0; 
			continue;
		}	
		found = 1;
		index = config[i].index;
		if(!t) {
			flags = &globals[type-1].flags;
			msg = &globals[type-1].msg[index];
			exec_str = &globals[type-1].ext_exec[index];
			int_type = &globals[type-1].hold;
		}
		else {
			flags = &t->flags;
			msg = &t->msg[index];
			exec_str = &t->ext_exec[index];
			int_type = &t->hold;
		}
#ifdef DEBUG		
printf(__FUNCTION__": found %s\n", config[i].t);
#endif
		for(p = p + len; valid_sep(*p); p++) ;
		if(config[i].val_type == BOOLEAN) {
#ifdef DEBUG
if(t) printf(__FUNCTION__": entering for %s (%d)\n", t->key, line);
#endif
			error = boolean(p, flags, config[i].mask);
		} 
		else if(config[i].val_type == INT)
			 error = int_val(p, int_type);
		else {
		/* ugly hack to support LoginExec, LogoutExec options */
			if(!strncasecmp("LoginExec", config[i].t, len) || !strncasecmp("LogoutExec", config[i].t, len)) 
				error = token(p, exec_str);
			else error = token(p, msg);
		}	
		if(error) errx(1, "Syntax error in line %d", line);
		break;
	}
	return found;
}
		
struct entry_t *new_entry(char **s, int type, int line)
{
	struct entry_t *t;
	char *p = *s;
	while (**s && !valid_sep(**s) && **s != '\n') (*s)++;
	
	t = calloc(1, sizeof *t);
	if(!t) errx(1, ": Cannot allocate memory. Exiting.");

	/* for compatibility with older config file */
	if(type == GLOBAL_TYPE) globals[type-1].flags = ~0 & ~CENTER_MSG;
	t->type = type==GLOBAL_TYPE?USER_TYPE:type;
	t->flags = globals[type-1].flags;
	t->hold = globals[type-1].hold;
	t->key = calloc(1, *s - p + 1);
	strncpy(t->key , p, *s - p);
#ifdef DEBUG
printf(__FUNCTION__":-%s- %d 0x%x\n", t->key, t->type, t->flags);
#endif
	comp_regex(t, line);
	addto_list(t, entry_l);
	return t;
} 	

void read_options(FILE *f, struct entry_t *t, int type, int *n)
{
	char buf[128];
	char *p;
	int end = 0;
	while (fgets(buf, sizeof buf, f)){
		(*n)++;
#ifdef DEBUG		
printf(__FUNCTION__": line %d\n", *n);
#endif
		p = buf;
		if(!find_char(&p)) continue;
		if (*p == '#' || *p == '\n') continue;
		if (*p == '}') {
			end = 1;
			break;
		}
		if (read_option(p, type, t, *n)) continue;
		else errx(1, "Missing } in line %d", *n);
	}
	if(!end) errx(1, "Missing } in line %d", *n);
}	

/*
 * Parse config file. Call appriopriate function for each
 * recognized config option to set value.
 */
int read_conf(char *cf)
{
	FILE *f;
	char buf[128], *p = buf;
	struct entry_t *t = 0;
	int n, i, len, found, type = GLOBAL_TYPE;
	len = i = n = found = 0;
	if (!(f = fopen(cf, "r"))) return 0;
	while (fgets(buf, sizeof buf, f)){
		n++;
		p = buf;
		if (!find_char(&p)) continue;
		if (*p == '#' || *p  == '\n') continue;
		if (type_change(p, &type)) goto NEXT;
		if (read_option(p, type, t, n)) goto NEXT;
		if (*p == '{') {
			if(t) read_options(f, t, type, &n);
			else errx(1, "{ without previous identifier in line %d", n);
			continue;
		}
		t = new_entry(&p, type, n);
		if (!find_char(&p)) continue;
		if (*p != '{')  {
			read_option(p, type, t, n);
			goto NEXT;
		} 
//		errx(1,"%s { expected in line %d\n", p, n);
		read_options(f, t, type, &n);
NEXT:
		t = 0;		
		continue;
		len = 0;
	}
	fclose(f);
	return 1;
}	

void show_options(int flags)
{
	int size = sizeof config/sizeof (struct config_item);
	int i;
	for (i = 0; i < size ; i++) {
		switch (config[i].val_type) {
		case BOOLEAN:
			if(!(flags & config[i].mask)) continue;
			printf("%s yes\n", config[i].t); 
			break;
		case STRING:
		case INT:
			break;
		}
	}
}
	
void show_conf(void)
{
	int  prev_type = 0;
	struct entry_t *t;
	printf("\nCurrent configuration:\n");
printf("Global\n");
	show_options(globals[GLOBAL_TYPE-1].flags);
//printf("-messages %s, %s, hold %d\n", globals[GLOBAL_TYPE-1].msg[0],
//globals[GLOBAL_TYPE-1].msg[1], globals[GLOBAL_TYPE-1].hold ); 
//printf("-exec %s, %s\n", globals[GLOBAL_TYPE-1].ext_exec[0], globals[GLOBAL_TYPE-1].ext_exec[1]);

printf("Users:\n");
	show_options(globals[USER_TYPE-1].flags);
	for_each(t, entry_l) {
		if(t->type != prev_type) {
			printf("\nWatched %s\n", groups[t->type-1].s);
			prev_type = t->type;
		}
/*
		printf("\t--%s, %d, 0x%x, %s, %s, %s, %s, %d\n", 
			t->key, t->type, t->flags, t->msg[0], t->msg[1], 
			t->ext_exec[0], t->ext_exec[1], t->hold);
*/
		printf("\t--%s, %d, 0x%x, %d\n", 
			t->key, t->type, t->flags,  t->hold);
		show_options(t->flags);
	}
	printf("\n");
}	

void clear_conf(void)
{
	struct entry_t *t, *tmp;
	struct tty_t *ty, *x;
	t = entry_l.next;
	while(t != (void*) &entry_l) {
		tmp = t->next;
		if(t->key) free(t->key);
		free(t);
		t = tmp;
	}
	ty = tty_l.next;
	while(ty != (void*) &tty_l) {
		x = ty->next;
		if(ty->tty) free(ty->tty);
		if(ty->name) free(ty->name);
		if(ty->host) free(ty->host);
		free(ty);
		ty = x;
	}
}	
