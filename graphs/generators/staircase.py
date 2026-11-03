# A pathological example of a bipartite graph that forces
# Hopcroft-Karp to achieve its asymptotic worst-case complexity.

import math
import sys

from common import *

def max_rounds(s):
  # R(s), the most rounds possible at size s.
  return math.isqrt(4 * s + 1) - 1

def staircase_adj(n):
  # Number of cells in each row of the cell diagram.
  lengths = [max(2, (n + 2 - a) // 2) for a in range(1, n)]
  boy_cells = [(a, c) for a, length in enumerate(lengths, 1)
               for c in range(1, length + 1)]
  boy_at = {cell: b for b, cell in enumerate(boy_cells, 1)}

  # The first girl of each row is numbered n-a; all remaining girls are
  # numbered consecutively in row-major order.
  girl_cells = [(n - a, 1) for a in range(1, n)]
  girl_cells += [cell for cell in boy_cells if cell[1] > 1]
  girl_number = {}
  for i, cell in enumerate(girl_cells):
    a, c = cell
    girl_number[cell] = n - a if c == 1 else n + i - (n - 1)

  s = len(girl_cells)
  adj = [None] * s
  corner = (n - 1, 2)
  for a, c in girl_cells:
    row = []

    # The boy to the left of this girl.
    if c > 1:
      row.append(boy_at[a, c - 1])

    # The boy above this girl. The edge into the corner is omitted.
    if a > 1 and (a, c) != corner:
      row.append(boy_at[a - 1, c])

    row.append(boy_at[a, c])
    adj[girl_number[(a, c)] - 1] = row
  return adj

def staircase_padded_adj(s):
  if s < 2:
    return [[] for _ in range(s)]

  n = max_rounds(s)
  adj = staircase_adj(n)
  adj.extend([[g, g - 1] for g in range(len(adj) + 1, s + 1)])
  return adj

if len(sys.argv) != 2:
  print('Usage: staircase.py <s>')
  print('s = floor((n+1)^2/4) gives Gamma_n; other sizes are padded.')
  exit(1)

s = int(sys.argv[1])
assert s >= 2
adj = staircase_padded_adj(s)

init_header(v_attr_count=2)
vertex_labels(0)
bipartite(s, s, 1)

# libgraf scans girls in increasing file order, so store the logical
# girls in Knuth's scan order: g_s, g_(s-1), ..., g_1.
for g in range(s, 0, -1):
  add_vertex(f'g{g}', 0)

for b in range(1, s + 1):
  add_vertex(f'b{b}', 1)

for g in range(s, 0, -1):
  for b in reversed(adj[g - 1]):
    add_arc(f'g{g}', f'b{b}')
    add_arc(f'b{b}', f'g{g}')

fixup_header()
write_to(f'staircase-{s}.graf')
