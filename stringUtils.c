//
// Created by barry on 10/5/15.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "stringUtils.h"

char *stringConcat(char *string1, char *string2) {
    int l1 = strlen(string1);
    int l2 = strlen(string2);
    int l3 = l1 + l2;
    char *result = (char *) malloc(sizeof(char) * (l3 + 1));
    int i;
    for (i = 0; i < l1; i++) {
        result[i] = string1[i];
        //printf("--%c--\n",string1[i]);
    }
    for (i = l1; i < l3; i++) {
        result[i] = string2[i - l1];
        //printf("--%c--\n",string2[i-l1]);
    }
    result[l3] = 0;
    return result;

}

bool stringEquals(char *string1, char *string2) {
    if (strlen(string1) != strlen(string2)) {
        return false;
    }
    else {
        int i;
        for (i = 0; string1[i] != '\0'; i++) {
            //check if the character is a capital alphabet
            if (string1[i] >= 65 && string1[i] <= 90) {
                if (string1[i] != string2[i] && string1[i] + 32 != string2[i])
                    return false;
            }
                //check if the character is a small alphabet
            else if (string1[i] >= 97 && string1[i] <= 122) {
                if (string1[i] != string2[i] && string1[i] - 32 != string2[i])
                    return false;
            }
            else {
                if (string1[i] != string2[i])
                    return false;
            }
        }
        return true;
    }
}

char **splitString(char *input, char delimiter, int *outputSize) {
    char **output;
    char *inputCopy = (char *) malloc((strlen(input) + 1) * sizeof(char));

    // make a copy of the string
    int i;
    for (i = 0; input[i] != '\0'; i++) {
        inputCopy[i] = input[i];
    }
    inputCopy[i] = '\0';

    //calculate the number the tokens
    *outputSize = 0;
    for (i = 0; inputCopy[i] != '\0'; i++) {
        if (inputCopy[i] == delimiter)
            *outputSize += 1;
    }
    (*outputSize)++; //outputsize = number of delimiters + 1
    //printf("Number of tokens: %d\n", *outputSize);

    //allocate space for the output
    output = (char **) malloc(sizeof(char *) * (*outputSize));

    //split the string
    *output = inputCopy;
    int size = 1;
    for (i = 0; inputCopy[i] != '\0' && size < *outputSize; i++) {
        if (inputCopy[i] != delimiter) {
            continue;
        }
        else {
            inputCopy[i] = '\0';
            *(output + size) = inputCopy + i + 1;
            size++;
        }
    }
    return output;
}

char *printCurrentTime()
{
    char time_string[100];
    time_t current_time = time(0);
    strftime(time_string, 100, "%H:%M:%S", localtime(&current_time));
    return strdup(time_string);
}