#include "notifyme.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_ERRX
#include <err.h>
#endif

enum vtype { BOOLEAN, STRING, INT };

/*
 * This one contains all necessary information to parse config file
 * see config.c
 */
struct config_item
{
	char *t;				  	/* config token 		*/
	int index;					/* index in globals[]  		*/
	enum vtype val_type;				/* type of the value		*/			
	char *defaults;
	int mask;					/* mask used for setting flags  */
};
