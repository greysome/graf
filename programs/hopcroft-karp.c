/*
  An implementation of Hopcroft-Karp algorithm for finding a maximum
  bipartite matching. It closely follows Knuth's Algorithm F14A.H. In
  particular, I use the terms 'boys' and 'girls' to refer to vertices
  in the first and second part.
*/

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "graf.h"

#define ATTR_PART(vv) (vv)->attrs[graf->bipartite_part_v_attr_idx].u
#define ATTR_LABEL(vv) (vv)->attrs[graf->label_v_attr_idx].s
#define ATTR_MATE(vv) (vv)->internal_attrs[0].v
#define ATTR_MARK(vv) (vv)->internal_attrs[1].u
#define ATTR_B_LINK(vv) (vv)->internal_attrs[2].a
#define ATTR_B_NEXT(ww) (ww)->internal_attrs[0].a

const char *GRAF_BAD_GRAPH = "not a bipartite graph";

typedef struct {
  // First `n_unmatched_girls` elements stores the unmatched girls.
  // Further elements are populated via BFS.
  GrafVertex **queue;
  int *iqueue;
  GrafVertex **marked;
  GrafVertex **stack;
  int n_unmatched_girls;
  int n_marked;

  int n_iter;
  int matching_size;
  int *bfs_vertex_counts;
  int *bfs_arc_counts;
  int *sap_levels;
  int *sap_counts;
} State;

void init_state(uint32_t nv, State *state) {
  state->queue = calloc(nv, sizeof(*state->queue));
  state->iqueue = calloc(nv, sizeof(*state->iqueue));
  state->marked = calloc(nv, sizeof(*state->marked));
  state->stack = calloc(nv, sizeof(*state->stack));
  state->bfs_vertex_counts = calloc(nv, sizeof(*state->bfs_vertex_counts));
  state->bfs_arc_counts = calloc(nv, sizeof(*state->bfs_arc_counts));
  state->sap_levels = calloc(nv, sizeof(*state->sap_levels));
  state->sap_counts = calloc(nv, sizeof(*state->sap_counts));

  state->n_unmatched_girls = 0;
  state->n_marked = 0;
  state->matching_size = 0;
  state->n_iter = 0;
}

void cleanup(Graf *graf, State *state) {
  free(graf->V);
  free(graf->internal.all_arcs);
  free(graf->internal.all_strs);

  free(state->queue);
  free(state->iqueue);
  free(state->marked);
  free(state->stack);
  free(state->bfs_vertex_counts);
  free(state->bfs_arc_counts);
  free(state->sap_levels);
  free(state->sap_counts);
  free(state);
}

/*
  Greedily construct a maximal matching by matching each girl in turn
  with her first unmatched boy neighbour, if possible.
*/
void construct_maximal_matching(Graf *graf, State *state) {
  for (GrafVertex *g = graf->V; g < graf->V + graf->nv; g++) {
    // We iterate through only girls' neighbours.
    if (ATTR_PART(g) == 1) {
      continue;
    }

    for (GrafArc *a = g->arcs; ; a = a->next) {
      // If all neighbours of g are matched, then add to the unmatched
      // queue
      if (a == NULL) {
        state->queue[state->n_unmatched_girls] = g;
        state->iqueue[g->id] = state->n_unmatched_girls;
        state->n_unmatched_girls++;
        break;
      }

      // Match with the first matched neighbour of g
      if (ATTR_MATE(a->to) == NULL) {
        state->matching_size++;
        ATTR_MATE(a->to) = g;
        ATTR_MATE(g) = a->to;
        break;
      }
    }
  }

  // Print initial matching
  GRAF_DEBUG("Initial matching:\n");
  for (GrafVertex *v = graf->V; v < graf->V + graf->nv; v++) {
    if (ATTR_PART(v) == 1) {
      continue;
    }

    GrafVertex *g = v;
    GrafVertex *b = ATTR_MATE(g);

    if (b == NULL) {
      continue;
    }

    GRAF_DEBUG("%v - %v\n", g, b);
  }
  GRAF_DEBUG("Size: %d\n\n", state->matching_size);
}

GrafStatus hopcroft_karp(Graf *graf, State *state) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(state != NULL);

  if (!(graf->segment_flags & GRAF_BIPARTITE)) {
    graf->internal.detailed_err = GRAF_BAD_GRAPH;
    return GRAF_ERR_USAGE;
  }

  init_state(graf->nv, state);
  construct_maximal_matching(graf, state);

bfs:
  GRAF_DEBUG("---------- BFS ----------\n");
  state->n_marked = 0;

  bool sap_found = false;

  // Current level, increments until the SAP is found
  int cur_level = 0;
  int sap_level = 0;
  // Number of boys at the final level
  int r = 0;

  int qhead = 0;
  int qtail = state->n_unmatched_girls;
  // The end of the level in the queue
  int qmark;

next_level:
  GRAF_DEBUG("Level %d\n", cur_level);
  qmark = qtail;

  while (qhead < qmark) {
    GrafVertex *g = state->queue[qhead++];

    GRAF_DEBUG("- visit girl: %v\n", g);
    state->bfs_vertex_counts[state->n_iter]++;

    for (GrafArc *a = g->arcs; a != NULL; a = a->next) {
      state->bfs_arc_counts[state->n_iter]++;

      GrafVertex *b = a->to;

      // Add new link if boy already marked at current level
      if (ATTR_MARK(b) == (uint32_t) cur_level + 1) {
        ATTR_B_NEXT(a) = ATTR_B_LINK(b);
        ATTR_B_LINK(b) = a;
        continue;
      }
      // Ignore boy if encountered at lower level already
      else if (ATTR_MARK(b) > 0) {
        continue;
      }

      // Boy is unmarked: BFS found a new vertex
      GRAF_DEBUG("  - visit boy: %v", b);
      state->bfs_vertex_counts[state->n_iter]++;

      GrafVertex *g1 = ATTR_MATE(b);

      // Ignore boy if he is matched and SAP already found. He is not
      // in an SAP
      if (g1 != NULL && sap_found) {
        GRAF_DEBUG(" (matched; ignoring)\n");
        continue;
      }

      // Mark the boy
      ATTR_MARK(b) = (uint32_t) cur_level + 1;
      state->marked[state->n_marked++] = b;

      // Add the first link
      ATTR_B_NEXT(a) = NULL;
      ATTR_B_LINK(b) = a;

      // Enqueue if boy is matched and SAP not found yet
      if (g1 != NULL) {
        GRAF_DEBUG(" (mate %v)\n", g1);
        state->queue[qtail++] = g1;
      }
      // First unmatched boy found: we found an SAP!
      else if (!sap_found) {
        GRAF_DEBUG(" (unmatched)\n");
        sap_found = true;
        sap_level = cur_level + 1;
        r = 1;
      }
      else {
        GRAF_DEBUG(" (unmatched)\n");
        r++;
      }
    }
  }

  if (sap_found) {
    GRAF_DEBUG("\n---------- DFS ----------\n");
    state->sap_levels[state->n_iter] = sap_level;
    goto next_sap;
  }
  // No more levels? Then there are no more SAPs and we are done
  else if (qmark == qtail) {
    return GRAF_OK;
  }
  else {
    cur_level++;
    goto next_level;
  }

next_sap:
  if (r == 0) {
    for (int i = 0; i < state->n_marked; i++) {
      ATTR_MARK(state->marked[i]) = 0;
    }
    state->n_iter++;
    GRAF_DEBUG("\n");
    goto bfs;
  }

  // The goal of steps H12 and H13 is to find an SAP in the current
  // dag (where some augmented SAPs might have already been
  // removed). Then, g is the first vertex in the SAP.
  GrafVertex *g;
  GrafVertex *b = state->marked[state->n_marked - r];

  r--;
  cur_level = sap_level;

H12:
  state->stack[--cur_level] = b;
  GRAF_DEBUG("- level %d: visit boy: %v\n", cur_level, b);

  // Find a girl preceding b by one level, that hasn't been visited
H13:
  GrafArc *a;
  while (1) {
    a = ATTR_B_LINK(b);

    // No more girls preceding b; resume higher level
    if (a == NULL) {
      cur_level++;
      GRAF_DEBUG("  - moving up one level\n");
      if (cur_level == sap_level) {
        goto next_sap;
      }
      else {
        b = state->stack[cur_level];
        goto H13;
      }
    }

    ATTR_B_LINK(b) = ATTR_B_NEXT(a);
    g = a->from;

    // The girl g is eligible if her current mate is empty (i.e. she
    // is unmatched) or unmarked.
    GrafVertex *b1 = ATTR_MATE(g);
    if (b1 == NULL || ATTR_MARK(b1) != (uint32_t) sap_level + 2) {
      b = b1;
      GRAF_DEBUG("  - found girl: %v\n", g);
      break;
    }
  }

  // The eligible girl g is matched, so we have not reached the start
  // of the SAP. Go down one level
  if (b != NULL) {
    goto H12;
  }

  GRAF_DEBUG("  - augmenting\n");

  // Remove g from the list of unmatched girls; replace its position
  // in queue with the last unmatched girl
  int k = state->iqueue[g->id];
  GrafVertex *g1 = state->queue[--state->n_unmatched_girls];
  state->queue[k] = g1;
  state->iqueue[g1->id] = k;

  b = state->stack[0];

  // Augment!
  while (1) {
    ATTR_MARK(b) = (uint32_t) sap_level + 2;
    g1 = ATTR_MATE(b);
    ATTR_MATE(b) = g;
    ATTR_MATE(g) = b;

    if (g1 == NULL) {
      break;
    }

    g = g1;
    b = state->stack[++cur_level];
  }

  state->sap_counts[state->n_iter]++;
  state->matching_size++;
  goto next_sap;
}

void input_file(char *path, Graf *graf) {
  GrafStatus status = graf_open(path, graf);
  if (status != GRAF_OK) {
    fprintf(stderr, "[ERROR] graf_open() failed %s\n", graf_error_info(graf, status));
    exit(1);
  }

  GrafVertex *V = calloc(graf->nv, sizeof(*V));
  GrafArc *A = calloc(graf->na, sizeof(*A));
  char *s = calloc(graf->nchars, 1);

  status = graf_load(graf, V, A, s);
  if (status != GRAF_OK) {
    fprintf(stderr, "[ERROR] graf_load() failed %s\n", graf_error_info(graf, status));
    exit(1);
  }
}

void random_bipartite(Graf *graf, long m, long n, double p) {
  graf->nv = (uint32_t) (m + n);
  graf->a_attr_count = 0;
  graf->nchars = 0;
  graf->internal.detailed_err = NULL;
  graf->internal.fd = -1;

  graf->segment_flags = GRAF_BIPARTITE;
  graf->bipartite_parts[0] = (uint32_t) m;
  graf->bipartite_parts[1] = (uint32_t) n;
  graf->bipartite_part_v_attr_idx = 0;
  graf->v_attr_count = 1;
  graf->v_attr_types[0] = GRAF_UINT;

  GrafVertex *V = calloc(graf->nv, sizeof(*V));
  // Overallocate the arcs, just to be very safe
  GrafArc *A = calloc((size_t) (2 * m * n), sizeof(*graf->internal.all_arcs));

  // Initialise vertices
  for (int i = 0; i < m + n; i++) {
    GrafVertex *v = &V[i];
    v->id = (uint32_t) i;
    v->arcs = NULL;
    v->attrs[0].u = i < m ? 0 : 1;
  }

  // Initialise arcs randomly
  srand((unsigned int) time(NULL));
  GrafArc *a = A;

  for (GrafVertex *v = V; v < V + m; v++) {
    for (GrafVertex *w = V + m; w < V + m + n; w++) {
      // TODO: use a better RNG
      double x = (double) rand() / RAND_MAX;
      if (x < p) {
        a->from = v;
        a->to = w;
        a->next = v->arcs;
        v->arcs = a;
        a++;

        a->from = w;
        a->to = v;
        a->next = w->arcs;
        w->arcs = a;
        a++;
      }
    }
  }

  graf->V = V;
  graf->internal.all_arcs = A;
  // Dummy pointer that can be free()'d in graf_close()
  graf->internal.all_strs = calloc(1, sizeof(char));
  graf_finalize(graf);
}

void input_rand(char *spec, Graf *graf) {
  char *ptr = spec;
  // The spec is known to start with 'rand:' so it has at least 5 chars
  ptr += 5;
  if (*ptr == '\0') {
    goto bad;
  }

  long m = strtol(ptr, &ptr, 10);
  if (m <= 0 || *ptr == '\0' || *(ptr++) != ',') {
    goto bad;
  }

  long n = strtol(ptr, &ptr, 10);
  if (n <= 0 || *ptr == '\0' || *(ptr++) != ',') {
    goto bad;
  }

  double p = strtod(ptr, &ptr);
  if (p < 0.0 || p > 1.0 || *ptr != '\0') {
    goto bad;
  }

  random_bipartite(graf, m, n, p);
  return;

bad:
  fprintf(stderr, "[ERROR] Invalid random spec '%s'\n", spec);
  exit(1);
}

void print_tabular_format(State *state) {
  printf("iter,bfs-vertices,bfs-arcs,sap-level,sap-count\n");
  for (int i = 0; i < state->n_iter; i++) {
    printf("%d,%d,%d,%d,%d\n", i, state->bfs_vertex_counts[i], state->bfs_arc_counts[i], state->sap_levels[i], state->sap_counts[i]);
  }
}

void print_human_format(Graf *graf, State *state) {
  printf(" %-4s  |  %-12s  %-8s  %-9s  %-9s\n", "Iter", "BFS vertices", "BFS arcs", "SAP level", "# SAPs");
  printf("----------------------------------------------------\n");
  for (int i = 0; i < state->n_iter; i++) {
    printf(" %4d  |  %-12d  %-8d  %-9d  %-9d\n", i, state->bfs_vertex_counts[i], state->bfs_arc_counts[i], state->sap_levels[i], state->sap_counts[i]);
  }

  printf("\nMaximum matching:\n");
  for (GrafVertex *v = graf->V; v < graf->V + graf->nv; v++) {
    GrafVertex *w = ATTR_MATE(v);

    if (ATTR_PART(v) != 0 || w == NULL) {
      continue;
    }

    printf("%v - %v\n", v, w);
  }

  printf("Size: %d\n", state->matching_size);
}

int main(int argc, char **argv) {
  bool flag_tabular_format = false;

  while (1) {
    int c = getopt(argc, argv, "t");
    if (c == 't') {
      flag_tabular_format = true;
    }
    else if (c == -1) {
      break;
    }
  }

  if (optind >= argc) {
    printf(
"Usage: hopcroft-karp [OPTION]... [SPEC]\n"
"\n"
"SPEC is either a path, or a string of the form 'rand:<M>,<N>,<P>', which\n"
"tells the program to generate a random bipartite graph with part sizes M\n"
"and N, where the proportion of edges given by P (a floating point number\n"
"between 0 and 1).\n"
"\n"
"  -t\toutput algorithm metrics in tabular format, for friendly parsing\n"
"    \t(does not output the final matching)\n"
    );
    return 1;
  }

  char *path = argv[optind];
  Graf *graf = calloc(1, sizeof(*graf));
  GrafStatus status = graf_init(graf);
  GRAF_ASSERT(status == GRAF_OK);

  if (strncmp(path, "rand:", 5) == 0) {
    input_rand(path, graf);
  }
  else {
    input_file(path, graf);
  }

  State *state = calloc(1, sizeof(*state));
  hopcroft_karp(graf, state);

  if (flag_tabular_format) {
    print_tabular_format(state);
  }
  else {
    print_human_format(graf, state);
  }

  graf_close(graf);
  cleanup(graf, state);
  free(graf);
  return 0;
}
