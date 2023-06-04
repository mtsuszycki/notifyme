/* 
 * Simple double linked list operations.
 */


#define for_each(e,l) 	for((e)=(l).next; (e)!=(void *) &(l); (e)=(e)->next)
#define addto_list(e,l)	{(e)->prev=(l).prev;		\
			 (e)->prev->next=(e);		\
			 (e)->next=(typeof(e))&l;	\
			 (l).prev=(e);}
#define delfrom_list(e,l) {(e)->prev->next=(e)->next;	\
			   (e)->next->prev=(e)->prev; }

#define dellist(e,l)	{ void *p;			\
			  for((e)=(l).next; (e)!=(void *) &(l); (e)=p){	\
			  	p=(e)->next;		\
				delfrom_list((e),(l));	\
				free((e));		\
			  }				\
			}		
#define dellist1(e,l)	{ void *p;			\
			  for((e)=(l).next; (e)!=(void *) &(l); (e)=p){	\
			  	p=(e)->next;		\
				delfrom_list((e),(l));	\
				if((e)->msg) free((e)->msg); \
				free((e));		\
			  }				\
			}		
#define IS_EMPTY(l)	((l)->next == l)
			
