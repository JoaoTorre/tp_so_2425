#include "utils.h"

void toUpperString(char *str)
{
    size_t i;
    for (i = 0; i < strlen(str); i++)
    {
        str[i] = toupper(str[i]);
    }
}