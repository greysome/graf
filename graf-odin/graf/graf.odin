package graf

import "core:c"

_ :: c

foreign import lib "system:graf"

GRAF_VERSION            :: 0
GRAF_HEADER_SIZE        :: 28
GRAF_MAX_ATTRS          :: 2
GRAF_MAX_INTERNAL_ATTRS :: 3

GrafSegCodes :: enum u32 {
  VERTEX_LABEL = 0,
  ARC_LABEL    = 1,
  BIPARTITE    = 2,
  VERTEX_ARRAY = 4294967294,
  ARC_ARRAY    = 4294967295,
}

GrafStatus :: enum u32 {
  OK                = 0,
  ERR_SYS           = 1,
  ERR_PARSE         = 2,
  ERR_USAGE         = 3,
  ERR_UNIMPLEMENTED = 4,
}

GrafType :: enum u32 {
  UNUSED = 0,
  UINT   = 1,
  STR    = 2,
}

GrafFlag :: enum u32 {
  VERTEX_LABELLED = 0,
  ARC_LABELLED    = 1,
  BIPARTITE       = 2,
}

GrafFlags :: bit_set[GrafFlag; u32]

GrafAttr :: struct {
  type: GrafType,

  using _: struct #raw_union {
    u: u32,
    f: f32,
    s: cstring,
    v: ^GrafVertex,
    a: ^GrafArc,
  },
}

GrafArc :: struct {
  from:           ^GrafVertex,
  to:             ^GrafVertex,
  next:           ^GrafArc,
  attrs:          [2]GrafAttr,
  internal_attrs: [3]GrafAttr,
}

GrafVertex :: struct {
  id:             u32,
  arcs:           ^GrafArc,
  attrs:          [2]GrafAttr,
  internal_attrs: [3]GrafAttr,
}

GrafInternal :: struct {
  fd:              i32,
  all_arcs:        [^]GrafArc,
  all_strs:        cstring,
  all_strs_offset: c.size_t,
}

Graf :: struct {
  nv:                        u32,
  na:                        u32,
  v_attr_count:              u32,
  a_attr_count:              u32,
  char_count:                u32,
  V:                         [^]GrafVertex,
  segment_flags:             GrafFlags,
  v_attr_types:              [2]GrafType,
  a_attr_types:              [2]GrafType,
  label_v_attr_idx:          u32,
  label_a_attr_idx:          u32,
  bipartite_parts:           [2]u32,
  bipartite_part_v_attr_idx: u32,
  internal:                  GrafInternal,
  detailed_err:              cstring,
}

@(default_calling_convention="c")
foreign lib {
  graf_init :: proc(graf: ^Graf) -> GrafStatus ---
  graf_open :: proc(path: cstring, graf: ^Graf) -> GrafStatus ---
  graf_load :: proc(graf: ^Graf, V: [^]GrafVertex, A: [^]GrafArc, s: cstring) -> GrafStatus ---
  graf_close :: proc(graf: ^Graf) -> GrafStatus ---
}