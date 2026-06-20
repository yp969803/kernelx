#include "string.h"

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

uint32_t strlen(const char *s)
{
    uint32_t len = 0;
    if (!s) {
        return 0;
    }

    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }

    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        if (a[i] == '\0' || a[i] != b[i]) {
            return (int)(uint8_t)a[i] - (int)(uint8_t)b[i];
        }
    }

    return 0;
}

char *strchr(const char *s, char ch)
{
    while (*s != '\0') {
        if (*s == ch) {
            return (char *)s;
        }
        s++;
    }

    return ch == '\0' ? (char *)s : 0;
}

void strcpy(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst++ = *src++;
    }
    *dst = '\0';
}
