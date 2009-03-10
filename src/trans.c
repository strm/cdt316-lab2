/*
 * Text: Transaction handling system
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */

#include "trans.h"
#include "lock.h"
#include <stdio.h>


int removeTransaction(transNode ** list, int id){
	transNode * current;
	transNode * prev;
	
	if((*list) == NULL)
		return FALSE;
	else{
		current = (*list);
		prev = (*list);
		do{
			printf("%d\n", current->id);
			if( current->id == id){
				if(prev == (*list))
					(*list) = current->next;
				else
					prev->next = current->next;
				free(current);
				printf("removing %d\n", id);
				return TRUE;
			}
			else{
				prev = current;
				current = current->next;
			}
		
		}while(current != NULL);
	}
	printf("not removed %d\n", id);
	return FALSE;
}


/*
 * fafaf afea fa fa eafae ae fae fea eaf afeafeg wrgsr grg srg srg srgsr gsr
 *  ge ajka g
 */
int addTransaction(transNode ** list, transNode * arg){
	transNode * tmp;
	if((*list) == NULL){
		(*list) = arg;
		return TRUE;
	}
	else{
		tmp = (*list);
		while(tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = arg;
		arg->next = NULL;
		return TRUE;
	}
	return FALSE;
}

/*
 * Creates a new transaction
 */
transNode * createTransaction(int id){
	transNode * tmp = (transNode *) malloc(sizeof(transNode));
	tmp->id = id;
	tmp->next = NULL;
	return tmp;
}

/*
 * Pops the first element of argument varList
 * TODO will this be used?
 */
data_t varListPop(varList ** arg){
	varList * tmp;
	data_t ret;
	ret.cmd = NO_ARG;
	if((*arg) == NULL)
		printf("{0}");
	else if((*arg)->next == NULL){
		tmp = (*arg);
		(*arg) = NULL;
		ret = tmp->data;
		free(tmp);
		printf("{1}");
	}
	else{
		tmp = (*arg);
		(*arg) = (*arg)->next;
		ret = tmp->data;
		free(tmp);
		printf("{2+}");
	}
	return ret;
}
/*
 * Adds a new item to the list.
 * Placed last for reasons.
 */
int varListPush(data_t data, varList ** arg){
	varList * tmp = (varList *) malloc(sizeof(varList));
	varList * cur;
	tmp->data = data;
	tmp->next = NULL;
	printf("push(%d)", data.cmd);
	if(*arg == NULL){
		(*arg) = tmp;
		return -1;
	}
	else{
		cur = (*arg);
		while( (cur)->next != NULL ){
			cur = cur->next;
		}
		cur->next = tmp;
		return 1;
	}
	return 0;
}
