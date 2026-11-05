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

  if (graf_open(argv[1], &graf) != GRAF_OK) {
    fprintf(stderr, "%s\n", graf.detailed_err);
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

  int exit_code = 0;

  if (V == NULL || A == NULL || s == NULL) {
    fprintf(stderr, "allocation failed\n");
    graf_close(&graf);
    exit_code = 1;
    goto cleanup;
  }

  if (graf_load(&graf, V, A, s) != GRAF_OK) {
    fprintf(stderr, "%s\n", graf.detailed_err);
    graf_close(&graf);
    exit_code = 2;
    goto cleanup;
  }

  fprintf(stderr, "ok!\n");
  graf_close(&graf);

cleanup:
  free(V);
  free(A);
  free(s);
  return exit_code;
}
