//
// Created by barry on 10/5/15.
//

#ifndef STRINGUTILS_H
#define STRINGUTILS_H
#include <stdbool.h>
char *stringConcat(char *string1, char *string2);
bool stringEquals(char *string1, char *string2);
char **splitString(char *input, char delimiter, int *outputSize);

char *printCurrentTime();
#endif //STRINGUTILS_H
