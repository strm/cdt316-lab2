/*
 * Text: Header file for local parsing function
 * Authors: Niklas Pettersson, Lars Cederholm
 * Date: Today
 */

#ifndef PARSER_H
#define PARSER_H

#include "trans.h"
#include "../framework/middle-support.h"

int varListToVarList(varList ** parsed, varList * unparsed);
int checkLocks(varList * parsed);
int getUsedVariables(varList ** var, varList * trans);
int localParse(varList ** var, varList * trans);
int commitParse(transNode * trans);
int sendResponse(transNode * trans);
#endif
