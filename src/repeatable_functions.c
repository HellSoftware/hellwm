#include <complex.h>
#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/repeatable_functions.h"

int addElement(char*** arr, int* size, const char* newElement)
{
    char** newArr = (char**)realloc(*arr, (*size + 1) * sizeof(char*));

    if (newArr == NULL) {
        return 1;
    }

    *arr = newArr;

    (*arr)[*size] = (char*)malloc((strlen(newElement) + 1) * sizeof(char));

    if ((*arr)[*size] == NULL) {
        return 1;
    }

    strcpy((*arr)[*size], newElement);

    *size += 1;

    return 0;
}
