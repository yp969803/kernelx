#include "string.h"
#include <stdbool.h>

#define INT_MAX 2147483647

bool streq(const char *s1, const char *s2, int cnt)
{
    if (cnt < 0) {
        return false;
    } else if (cnt == 0) {
        cnt = INT_MAX;
    }
    int i = 0;
    int j = 0;
    while (s1[i] != '\0' && s2[j] != '\0' && i < cnt && j < cnt) {
        if (s1[i] != s2[j]) {
            return false;
        }
        i++;
        j++;
    }

    if (i >= cnt || j >= cnt) {
        return true;
    }

    return (s1[i] == '\0' && s2[j] == '\0');
}
