#pragma once

#include <stdbool.h>
#include <stdint.h>

bool streq(const char *s1, const char *s2, int cnt);
uint32_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, uint32_t n);
char *strchr(const char *s, char ch);
void strcpy(char *dst, const char *src);
