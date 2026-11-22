package graf

import "core:c"

_ :: c

foreign import lib "system:graf"

VERSION            :: 0
HEADER_SIZE        :: 28
MAX_ATTRS          :: 2
MAX_INTERNAL_ATTRS :: 3

SegCodes :: enum u32 {
  VERTEX_LABEL = 0,
  ARC_LABEL    = 1,
  BIPARTITE    = 2,
  VERTEX_ARRAY = 4294967294,
  ARC_ARRAY    = 4294967295,
}

Status :: enum u32 {
  OK                = 0,
  ERR_SYS           = 1,
  ERR_PARSE         = 2,
  ERR_USAGE         = 3,
  ERR_UNIMPLEMENTED = 4,
}

Type :: enum u32 {
  UNUSED = 0,
  UINT   = 1,
  STR    = 2,
}

Flag :: enum u32 {
  VERTEX_LABELLED = 0,
  ARC_LABELLED    = 1,
  BIPARTITE       = 2,
}

Flags :: bit_set[Flag; u64]

Attr :: struct {
  type: Type,

  using _: struct #raw_union {
    u: u32,
    f: f32,
    s: cstring,
    v: ^Vertex,
    a: ^Arc,
  },
}

Arc :: struct {
  from:           ^Vertex,
  to:             ^Vertex,
  next:           ^Arc,
  attrs:          [MAX_ATTRS]Attr,
  internal_attrs: [MAX_INTERNAL_ATTRS]Attr,
}

Vertex :: struct {
  id:             u32,
  arcs:           ^Arc,
  attrs:          [MAX_ATTRS]Attr,
  internal_attrs: [MAX_INTERNAL_ATTRS]Attr,
}

Internal :: struct {
  fd:              i32,
  all_arcs:        [^]Arc,
  all_strs:        cstring,
  all_strs_offset: c.size_t,
  detailed_err:    cstring,
}

Graf :: struct {
  nv:                        u32,
  na:                        u32,
  v_attr_count:              u32,
  a_attr_count:              u32,
  nchars:                    u32,
  V:                         [^]Vertex,
  segment_flags:             Flags,
  v_attr_types:              [MAX_ATTRS]Type,
  a_attr_types:              [MAX_ATTRS]Type,
  label_v_attr_idx:          u32,
  label_a_attr_idx:          u32,
  bipartite_parts:           [2]u32,
  bipartite_part_v_attr_idx: u32,
  internal:                  Internal,
}

@(default_calling_convention="c")
@(link_prefix="graf_")
foreign lib {
  init       :: proc(G: ^Graf) -> Status ---
  open       :: proc(path: cstring, G: ^Graf) -> Status ---
  load       :: proc(G: ^Graf, V: [^]Vertex, A: [^]Arc, s: cstring) -> Status ---
  finalize   :: proc(G: ^Graf) ---
  close      :: proc(G: ^Graf) -> Status ---
  error_info :: proc(G: ^Graf, status: Status) -> cstring ---
}