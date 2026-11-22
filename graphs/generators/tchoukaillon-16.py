from common import *

init_header(v_attr_count=2)
vertex_labels(0)
bipartite(16, 16, 1)

# The order of the girl vertices has to be very specific, so as to
# force the initial greedy matching to be non-maximum.
add_vertex('g16', 0)
add_vertex('g14', 0)
add_vertex('g12', 0)
add_vertex('g10', 0)
add_vertex('g8', 0)
add_vertex('g6', 0)
add_vertex('g4', 0)
add_vertex('g2', 0)
add_vertex('g1', 0)
add_vertex('g3', 0)
add_vertex('g7', 0)
add_vertex('g13', 0)
add_vertex('g9', 0)
add_vertex('g15', 0)
add_vertex('g11', 0)
add_vertex('g5', 0)

for i in range(1, 17):
  add_vertex(f'b{i}', 1)

# The order of the girl->boy arcs matters, in order to force
# Hopcroft-Karp to be slow
edges = '5-5 5-3 11-11 11-9 15-15 9-9 9-7 13-13 7-7 7-3 3-3 1-1 ' + \
  '2-2 2-1 4-4 4-2 4-3 6-6 6-4 6-5 8-8 8-5 8-7 10-10 10-8 10-6 10-9 ' + \
  '12-12 12-10 12-11 14-14 14-13 16-16 16-14 16-15'
for edge in edges.split(' '):
  endpoints = edge.split('-')
  a = endpoints[0]
  b = endpoints[1]
  add_edge(f'g{a}', f'b{b}')

fixup_header()
write_to('tchoukaillon-16.graf')