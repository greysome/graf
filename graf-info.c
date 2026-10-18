#include <stdio.h>
#include <stdlib.h>
#include "graf.h"

#define CLEANUP()          \
  do {                     \
    graf_close(&graf);     \
    free(V);               \
    free(A);               \
    free(s);               \
  } while (0)

int main(int argc, char **argv) {
  Graf graf;
  graf_init(&graf);

  if (argc < 2) {
    fprintf(stderr, "Usage: graf-info <file>\n");
    return 1;
  }

  if (graf_open(argv[1], &graf) != GRAF_OK) {
    fprintf(stderr, "%s\n", graf.detailed_err);
    return 2;
  }

  size_t v_count = graf.v == 0 ? 1 : graf.v;
  size_t a_count = graf.a == 0 ? 1 : graf.a;
  size_t char_count = graf.char_count == 0 ? 1 : graf.char_count;
  GrafVertex *V = calloc(v_count, sizeof(*V));
  GrafArc *A = calloc(a_count, sizeof(*A));
  char *s = calloc(char_count, sizeof(*s));

  if (V == NULL || A == NULL || s == NULL) {
    fprintf(stderr, "allocation failed\n");
    CLEANUP();
    return 1;
  }

  if (graf_load(&graf, V, A, s) != GRAF_OK) {
    fprintf(stderr, "%s\n", graf.detailed_err);
    CLEANUP();
    return 2;
  }

  fprintf(stderr, "ok!\n");
  CLEANUP();
  return 0;
}
