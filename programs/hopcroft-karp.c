/*
  An implementation of Hopcroft-Karp algorithm for finding a maximum
  bipartite matching. It closely follows Knuth's Algorithm F14A.H. In
  particular, I use the terms 'boys' and 'girls' to refer to vertices
  in the first and second part.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void _init_state(uint32_t nv, State *state) {
  state->n_unmatched_girls = 0;
  state->n_marked = 0;
  state->matching_size = 0;
  state->n_iter = 0;
}

/*
  Greedily construct a maximal matching by matching each girl in turn
  with her first unmatched boy neighbour, if possible.
*/
void _initial_matching(Graf *graf, State *state) {
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

    if (graf->segment_flags & GRAF_VERTEX_LABELLED) {
      GRAF_DEBUG("%s - %s\n", ATTR_LABEL(g), ATTR_LABEL(b));
    }
    else {
      GRAF_DEBUG("%u - %u\n", g->id, b->id);
    }
  }
  GRAF_DEBUG("Size: %d\n\n", state->matching_size);
}

GrafStatus hopcroft_karp(Graf *graf, State *state) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(state != NULL);

  if (!(graf->segment_flags & GRAF_BIPARTITE)) {
    graf->detailed_err = GRAF_BAD_GRAPH;
    return GRAF_ERR_USAGE;
  }

  _init_state(graf->nv, state);
  _initial_matching(graf, state);

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

    GRAF_DEBUG("- visit girl: %s\n", ATTR_LABEL(g));
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
      GRAF_DEBUG("  - visit boy: %s", ATTR_LABEL(b));
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
        GRAF_DEBUG(" (mate %s)\n", ATTR_LABEL(g1));
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
      GRAF_DEBUG("\n");
      ATTR_MARK(state->marked[i]) = 0;
      state->n_iter++;
      goto bfs;
    }
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
  GRAF_DEBUG("- level %d: visit boy: %s\n", cur_level, ATTR_LABEL(b));

  // Find a girl preceding b by one level, that hasn't been visited
H13:
  GrafArc *a;
  while (1) {
    a = ATTR_B_LINK(b);
    ATTR_B_LINK(b) = ATTR_B_NEXT(a);
    g = a->from;
    b = ATTR_MATE(g);

    // The girl g is eligible if her current mate is empty (i.e. she
    // is unmatched) or unmarked.
    if (b == NULL || ATTR_MARK(b) != (uint32_t) sap_level + 2) {
      GRAF_DEBUG("  - found girl: %s\n", ATTR_LABEL(g));
      break;
    }

    // No more girls preceding b; resume higher level
    if (ATTR_B_LINK(b) == NULL) {
      cur_level++;
      if (cur_level == sap_level) {
        goto next_sap;
      }
      else {
        b = state->stack[cur_level];
        goto H13;
      }
      GRAF_DEBUG("  - moving up one level\n");
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

    state->matching_size++;
    state->sap_counts[state->n_iter]++;
  }

  goto next_sap;
}

int main(int argc, char **argv) {
  if (argc == 1) {
    printf(
"Usage:\n"
"hopcroft-karp <file>\n"
"hopcroft-karp rand:<A>,<B>,<p>,<n>\n"
"\n"
"rand:... tells the program to run the algorithm n times, each time on\n"
"a randomly generated bipartite graph with part sizes A,B, with the\n"
"proportion of edges given by p (an integer from 0-100, inclusive).\n"
    );
    return 1;
  }

  char *path = argv[argc - 1];
  Graf *graf = calloc(1, sizeof(*graf));
  GrafStatus status;

  status = graf_init(graf);
  GRAF_ASSERT(status == GRAF_OK);

  status = graf_open(path, graf);
  if (status != GRAF_OK) {
    fprintf(stderr, "%s", graf->detailed_err);
    return 1;
  }

  GrafVertex *V = calloc(graf->nv, sizeof(*V));
  GrafArc *A = calloc(graf->na, sizeof(*A));
  char *s = calloc(graf->char_count, 1);

  status = graf_load(graf, V, A, s);
  if (status != GRAF_OK) {
    fprintf(stderr, "%s", graf->detailed_err);
    return 1;
  }

  State *state = calloc(1, sizeof(*state));
  state->queue = calloc(graf->nv, sizeof(*state->queue));
  state->iqueue = calloc(graf->nv, sizeof(*state->iqueue));
  state->marked = calloc(graf->nv, sizeof(*state->marked));
  state->stack = calloc(graf->nv, sizeof(*state->stack));
  state->bfs_vertex_counts = calloc(graf->nv, sizeof(*state->bfs_vertex_counts));
  state->bfs_arc_counts = calloc(graf->nv, sizeof(*state->bfs_arc_counts));
  state->sap_levels = calloc(graf->nv, sizeof(*state->sap_levels));
  state->sap_counts = calloc(graf->nv, sizeof(*state->sap_counts));

  hopcroft_karp(graf, state);

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

    printf("%s - %s\n", ATTR_LABEL(v), ATTR_LABEL(w));
  }

  printf("Size: %d\n", state->matching_size);

  graf_close(graf);
  free(V);
  free(A);
  free(s);
  free(state->queue);
  free(state->iqueue);
  free(state->marked);
  free(state->stack);
  free(state->bfs_vertex_counts);
  free(state->bfs_arc_counts);
  free(state->sap_levels);
  free(state->sap_counts);
}