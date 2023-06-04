#include <stdio.h>
#include <string.h>
#include "defs.h"

#define ESC 		"\033"
#define COLOR(N) 	ESC "[3" #N "m"

#define SAVE 		ESC "7" 	/* save current state		*/
#define BOLD 		ESC "[1m"
#define LIGHTRED       	ESC "[1;31m"    /*BLUE COLOR..SK*/
#define RESET 		ESC "[0m"	/* reset attr to their defaults	*/

/* restore most recently saved state */
#define RESTORE 	ESC "8"

extern int cols, rows;
static int msg_x, msg_y;

static inline void goto_col(int x)
{
	char buf[16];
	snprintf(buf, sizeof buf, "\033[1;%dH", x);
	printf("%s", buf);
}

static inline void goto_xy(int x, int y)
{
	char buf[16];
	snprintf(buf, sizeof buf, "\033[%d;%dH", y, x);
	printf("%s", buf);
}
	

void erase(int l)
{
	int i;
	i = l>cols?0:cols-l;
	printf(SAVE);
	goto_xy(msg_x, msg_y);
	while(i++ < cols)
		printf(" ");
	printf(RESTORE RESET);
}

void echo(char *s, int c)
{
	int l = strlen(s);
	int y, x;
	y = 1;
	x = cols - l;
//	if(l > cols) l = cols;
//	else l = cols - l;
	if(c) {
		y = rows/2;
		x = cols/2 - l/2;
	}
	msg_x = x;
	msg_y = y;
	goto_xy(x, y);

#ifdef RED_MESSAGE
	printf(LIGHTRED "%s", s);
#else	
	printf(BOLD "%s", s);
#endif
	printf(RESTORE RESET);
}
	
