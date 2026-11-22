import pathlib

_buf = bytearray()
_char_count = 0
_v_attr_count = None
_a_attr_count = None
_nv = 0
_na = 0
_v_label_attr_idx = None
# From label to id. Allows for add_arc() specifying vertex labels of
# src and dest instead of vertex ids, for convenience.
_v_labels_map = dict()

def add_uint(u):
  global _buf
  assert isinstance(u, int)
  _buf += u.to_bytes(4, 'big')

def add_str(s):
  global _buf, _char_count
  assert isinstance(s, bytes)
  _buf += s
  _buf += b'\x00'
  _char_count += len(s) + 1

def add_any(v):
  if isinstance(v, int):
    add_uint(v)
  elif isinstance(v, bytes):
    add_str(v)
  elif isinstance(v, str):
    add_str(v.encode('utf-8'))
  else:
    assert False and f'unknown type'

def init_header(version=0, v_attr_count=0, a_attr_count=0):
  global _buf, _char_count, _v_attr_count, _a_attr_count
  _buf += b'graf'
  add_uint(version)
  add_uint(0)  # fixup: nv
  add_uint(0)  # fixup: na
  add_uint(v_attr_count)
  add_uint(a_attr_count)
  add_uint(0)  # fixup: char_count
  _v_attr_count = v_attr_count
  _a_attr_count = a_attr_count
  print(f'- [HEADER] v_attrs={v_attr_count}, a_attrs={a_attr_count}')

def add_segment(code, *data):
  global _buf
  add_uint(code)
  for d in data:
    add_any(d)

def vertex_labels(v_attr_idx):
  global _v_label_attr_idx

  add_segment(0x00000000, v_attr_idx)
  _v_label_attr_idx = v_attr_idx
  print(f'- [SEGMENT] vertex label: v_attr {v_attr_idx}')

def arc_labels(a_attr_idx):
  add_segment(0x00000001, a_attr_idx)
  print(f'- [SEGMENT] arc label: a_attr {a_attr_idx}')

def bipartite(A, B, v_attr_idx):
  add_segment(0x00000002, A, B, v_attr_idx)
  print(f'- [SEGMENT] bipartite({A},{B}): v_attr {v_attr_idx}')

def v_to_str(v):
  if isinstance(v, int):
    return f'#{v}'
  else:
    return str(v)

def add_vertex(*attrs):
  global _v_attr_count, _nv
  assert len(attrs) == _v_attr_count

  print(f'- [VERTEX] #{_nv}: {attrs}')

  if _nv == 0:
    add_segment(0xfffffffe)

  for attr in attrs:
    add_any(attr)

  if _v_label_attr_idx is not None:
    _v_labels_map.update({attrs[_v_label_attr_idx]: _nv})

  _nv += 1

def add_arc(src, dest, *attrs):
  global _a_attr_count, _na
  assert len(attrs) == _a_attr_count

  print(f'- [ARC] {v_to_str(src)} -> {v_to_str(dest)}: {attrs}')

  if _na == 0:
    add_segment(0xffffffff)

  if isinstance(src, str) or isinstance(src, bytes):
    src = _v_labels_map[src]
  if isinstance(dest, str) or isinstance(dest, bytes):
    dest = _v_labels_map[dest]
  add_uint(src)
  add_uint(dest)

  for attr in attrs:
    add_any(attr)

  _na += 1

def add_edge(src, dest):
  add_arc(src, dest)
  add_arc(dest, src)

def fixup_header():
  global _buf
  _buf[8:12] = _nv.to_bytes(4, 'big')
  _buf[12:16] = _na.to_bytes(4, 'big')
  _buf[24:28] = _char_count.to_bytes(4, 'big')
  print(f'- [HEADER] nv={_nv}, na={_na}, chars={_char_count}')

def write_to(file):
  GRAPH_DIR = pathlib.Path(__file__).resolve().parent.parent
  with open(f'{GRAPH_DIR}/{file}', 'wb') as f:
    f.write(_buf)
  print(f'- [EXPORT] {GRAPH_DIR}/{file}')