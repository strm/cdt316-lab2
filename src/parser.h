/*
 * Text: Header file for local parsing function
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */

#ifndef PARSER_H
#define PARSER_H

#include "trans.h"
#include "../framework/middle-support.h"
int getVars(void);
int getValues(void);


varList * fileToVarList(int fp);

int varListToVarList(varList ** parsed, varList * unparsed);
int checkLocks(varList * parsed);
int getUsedVariables(varList ** var, varList * trans);
int localParse(varList ** var, varList * trans);
#endif
