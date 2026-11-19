#include <stdio.h>
#include <stdlib.h>
#include "graf.h"

int main(int argc, char **argv) {
  Graf graf;
  graf_init(&graf);

  if (argc < 2) {
    fprintf(stderr, "Usage: graf-info <file>\n");
    return 1;
  }

  GrafStatus status = graf_open(argv[1], &graf);
  if (status != GRAF_OK) {
    fprintf(stderr, "graf_open() failed %s\n", graf_error_info(status, graf.detailed_err));
    graf_close(&graf);
    return 2;
  }

  size_t nv = graf.nv == 0 ? 1 : graf.nv;
  size_t na = graf.na == 0 ? 1 : graf.na;
  size_t nchars = graf.nchars == 0 ? 1 : graf.nchars;

  // Don't bother freeing since these are one-off allocations
  GrafVertex *V = calloc(nv, sizeof(*V));
  GrafArc *A = calloc(na, sizeof(*A));
  char *s = calloc(nchars, sizeof(*s));

  if (V == NULL || A == NULL || s == NULL) {
    fprintf(stderr, "allocation failed\n");
    graf_close(&graf);
    return 1;
  }

  status = graf_load(&graf, V, A, s);
  if (status != GRAF_OK) {
    fprintf(stderr, "graf_load() failed %s\n", graf_error_info(status, graf.detailed_err));
    graf_close(&graf);
    return 2;
  }

  fprintf(stderr, "ok!\n");
  graf_close(&graf);
  return 0;
}
