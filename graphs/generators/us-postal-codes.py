from common import *

codes = "Ak Al Ar As Az Ca Co Ct Dc De Fl Fm Ga Gu Hi Ia Id Il In Ks " \
  "Ky La Ma Md Me Mh Mi Mn Mo Mp Ms Mt Nc Nd Ne Nh Nj Nm Nv Ny " \
  "Oh Ok Or Pa Pr Pw Ri Sc Sd Tn Tx Um Ut Va Vi Vt Wa Wi Wv Wy"
codes = codes.split(' ')

init_header(v_attr_count=2)
vertex_labels(0)
bipartite(26, 26, 1)

for C in range(ord('A'), ord('Z') + 1):
  add_vertex(chr(C), 0)

for c in range(ord('a'), ord('z') + 1):
  add_vertex(chr(c), 1)

for C, c in codes:
  add_arc(C, c)

fixup_header()
write_to('us-postal-codes.graf')