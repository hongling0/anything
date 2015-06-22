#include <assert.h>
#include <stdio.h>

double str2double(const char* str, double def)
{
  char* endptr = NULL;
  assert(str);
  double n = strtod(str, &endptr);
  if (*endptr != '\0') {
    fprintf(stderr, "can't convert to double %s\n", str);
    return def;
  }
  return n;
}
long str2integer(const char* str, long def)
{
  char* endptr = NULL;
  assert(str);
  long n = strtol(str, &endptr, 0);
  if (*endptr != '\0') {
    fprintf(stderr, "can't convert to long %s\n", str);
    return def;
  }
  return n;
}
