import getopt
import sys
from common import *

def usage():
  print('''Usage: increasing-paths.py [SPEC]

Generate the disjoint union of paths with 3, 5, 7, ... edges, with a
suitable input order such that the hopcroft-karp program takes
Theta(sqrt(V)) iterations.

SPEC is exactly one of the following strings:
  -e [edges]
  -v [vertices]
  -p [paths]

If -e or -v is used, extra edges/vertices are treated as padding.''')
  sys.exit(2)


optlist, args = getopt.gnu_getopt(sys.argv[1:], 'e:v:p:', longopts=[])
if len(optlist) != 1:
  usage()

e, v, p = None, None, None
try:
  for opt, arg in optlist:
    if opt == '-e':
      e = int(arg)
    elif opt == '-v':
      v = int(arg)
    elif opt == '-p':
      p = int(arg)
except ValueError:
  usage()

# Exactly one valid mode must be selected
if len(optlist) != 1 or args or any(x is not None and x < 0 for x in (e, v, p)):
  usage()

init_header(v_attr_count=2)
vertex_labels(0)

if e is not None:
  # Find the largest p such that the paths use at most e edges
  paths_p = 0
  while (paths_p + 1) ** 2 + 2 * (paths_p + 1) <= e:
    paths_p += 1
  paths_e = paths_p ** 2 + 2 * paths_p
  paths_v = paths_e + paths_p
  excess_e = e - paths_e
  k = paths_v // 2
  # Arrange the excess edges into a perfect matching
  bipartite(k + excess_e, k + excess_e, 1)

elif v is not None:
  # Find the largest p such that the paths use at most v vertices
  paths_p = 0
  while (paths_p + 1) ** 2 + 3 * (paths_p + 1) <= v:
    paths_p += 1
  paths_e = paths_p ** 2 + 2 * paths_p
  paths_v = paths_e + paths_p
  excess_v = v - paths_v
  k = paths_v // 2
  # Make the excess vertices isolated girls
  bipartite(k + excess_v, k, 1)

elif p is not None:
  paths_p = p
  paths_e = paths_p ** 2 + 2 * paths_p
  paths_v = paths_e + paths_p
  k = paths_v // 2
  bipartite(k, k, 1)

assert(paths_v % 2 == 0)

for i in range(paths_p):
  for j in range(1, i + 2):
    add_vertex(f'g{i},{j}', 0)
    add_vertex(f'b{i},{j}', 1)
  # This vertex order is required for hopcroft-karp's initial maximal
  # matching to be suboptimal
  add_vertex(f'g{i},0', 0)
  add_vertex(f'b{i},0', 1)

# Add padding vertices
if e is not None:
  for i in range(excess_e):
    add_vertex(f'a{i}', 0)
    add_vertex(f'b{i}', 1)
elif v is not None:
  for i in range(excess_v):
    add_vertex(f'a{i}', 0)

for i in range(paths_p):
  add_edge(f'g{i},0', f'b{i},0')
  for j in range(1, i + 2):
    add_edge(f'g{i},{j}', f'b{i},{j}')
    add_edge(f'g{i},{j}', f'b{i},{j-1}')

# Add padding edges
if e is not None:
  for i in range(excess_e):
    add_edge(f'a{i}', f'b{i}')

fixup_header()
if e is not None:
  write_to(f'increasing-paths-e{e}.graf')
elif v is not None:
  write_to(f'increasing-paths-v{v}.graf')
elif p is not None:
  write_to(f'increasing-paths-p{p}.graf')
