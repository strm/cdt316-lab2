/*
 * Text: Header file for local parsing function
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */

#ifndef PARSER_H
#define PARSER_H

#include "trans.h"

int getVars(void);
int getValues(void);


varList * fileToVarList(int fp);

#endif
