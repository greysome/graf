#if !defined(__unix__) && !defined(__APPLE__)
#  error "GRAF is currently POSIX-only"
#endif

#include <fcntl.h>
#include <printf.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "graf.h"

/*
  --------------------------------------------------------------------
  VERTEX PRINTF FORMAT
  --------------------------------------------------------------------
*/

/*
  Used to determine how to print vertices in the custom format
  routine. If there are no vertex labels (based on
  _graf_global->segment_flags), then fall back to '#<id>'.
*/
Graf *_graf_global = NULL;
bool _graf_registered_printf = false;

static int _printf_vertex_arginfo(const struct printf_info *info, size_t n, int *argtypes) {
  if (n > 0) {
    argtypes[0] = PA_POINTER;
  }
  return 1;
}

static int _printf_vertex_handler(FILE *stream, const struct printf_info *info, const void *const *args) {
  GrafVertex *v = *((GrafVertex **) args[0]);
  int len = 0;

  if (_graf_global == NULL || !(_graf_global->segment_flags & GRAF_VERTEX_LABELLED)) {
    len = fprintf(stream, "#%d", v->id);
  }
  else {
    uint32_t idx = _graf_global->label_v_attr_idx;
    len = fprintf(stream, "%s", v->attrs[idx].s);
  }

  return len;
}

/*
  --------------------------------------------------------------------
  INTERNALS
  --------------------------------------------------------------------
*/

const char *GRAF_OPEN_FAIL = "open() failed";
const char *GRAF_READ_FAIL = "read() failed";
const char *GRAF_STAT_FAIL = "stat() failed";
const char *GRAF_MMAP_FAIL = "mmap() failed";
const char *GRAF_CLOSE_FAIL = "close() failed";
const char *GRAF_BAD_HEADER = "bad header";
const char *GRAF_NOT_OPENED = "file not opened";
const char *GRAF_BAD_ATTR_IDX = "attribute index out of bounds";
const char *GRAF_ABRUPT_EOF = "abrupt end-of-file";
const char *GRAF_UNKNOWN_SEG = "unknown segment";
const char *GRAF_VERTEX_OOB = "vertex index out of bounds";
const char *GRAF_BAD_NCHARS = "total character count too small";
const char *GRAF_VA_NOT_SEEN = "vertex array not seen before arc array";
const char *GRAF_NULLPTR = "null pointer was passed";
const char *GRAF_UNIMPLEMENTED = "not implemented yet";
const char *GRAF_ATTR_ALREADY_DEFINED = "attribute already defined";
const char *GRAF_BIPARTITE_BAD_PART_SIZES = "bad part sizes in bipartite segment";
const char *GRAF_BIPARTITE_BAD_ARC = "bad arc in bipartite segment";

typedef struct {
  const char *s;
  size_t n;
} GrafSlice;

static bool _graf_slice_require(const GrafSlice *slice, size_t n) {
  GRAF_ASSERT(slice != NULL);
  return slice->n >= n;
}

static void _graf_slice_advance(GrafSlice *slice, size_t n) {
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(_graf_slice_require(slice, n));
  slice->s += n;
  slice->n -= n;
}

static GrafStatus _graf_read_uint(Graf *graf, GrafSlice *slice, uint32_t *u) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(u != NULL);

  if (!_graf_slice_require(slice, sizeof(uint32_t))) {
    graf->detailed_err = GRAF_ABRUPT_EOF;
    return GRAF_ERR_PARSE;
  }

  unsigned char *s = (unsigned char *) slice->s;
  *u = ((uint32_t) s[0] << 24) | ((uint32_t) s[1] << 16) | ((uint32_t) s[2] << 8) | (uint32_t) s[3];
  _graf_slice_advance(slice, sizeof(uint32_t));
  return GRAF_OK;
}

static GrafStatus _graf_read_str(Graf *graf, GrafSlice *slice, const char **s) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(s != NULL);

  char *end = memchr(slice->s, '\0', slice->n);

  if (end == NULL) {
    graf->detailed_err = GRAF_ABRUPT_EOF;
    return GRAF_ERR_PARSE;
  }

  size_t n = (size_t) (end - slice->s + 1);

  if (graf->internal.all_strs_offset + n > graf->nchars) {
    graf->detailed_err = GRAF_BAD_NCHARS;
    return GRAF_ERR_PARSE;
  }

  char *t = &graf->internal.all_strs[graf->internal.all_strs_offset];
  memcpy(t, slice->s, n);
  *s = t;

  _graf_slice_advance(slice, n);
  graf->internal.all_strs_offset += n;

  return GRAF_OK;
}

#define _GRAF_READ_UINT(graf, slice, value)                       \
  do {                                                            \
    GrafStatus _status = _graf_read_uint(graf, slice, value);     \
    if (_status != GRAF_OK) {                                     \
      return _status;                                             \
    }                                                             \
  } while (0)

#define _GRAF_READ_STR(graf, slice, s)                            \
  do {                                                            \
    GrafStatus _status = _graf_read_str(graf, slice, s);          \
    if (_status != GRAF_OK) {                                     \
      return _status;                                             \
    }                                                             \
  } while (0)

static GrafStatus _graf_read_vertex_record(Graf *graf, GrafSlice *slice, GrafVertex *vertex) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(vertex != NULL);

  for (uint32_t j = 0; j < graf->v_attr_count; j++) {
    GrafType type = graf->v_attr_types[j];
    GrafAttr *attr = &vertex->attrs[j];
    attr->type = type;

    if (type == GRAF_UNUSED) {
      uint32_t unused;
      _GRAF_READ_UINT(graf, slice, &unused);
    }
    else if (type == GRAF_UINT) {
      _GRAF_READ_UINT(graf, slice, &attr->u);
    }
    else if (type == GRAF_STR) {
      _GRAF_READ_STR(graf, slice, &attr->s);
    }
    else {
      graf->detailed_err = GRAF_UNIMPLEMENTED;
      return GRAF_ERR_UNIMPLEMENTED;
    }
  }

  return GRAF_OK;
}

static GrafStatus _graf_read_arc_record(Graf *graf, GrafSlice *slice, GrafArc *arc) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(arc != NULL);

  uint32_t from_id;
  uint32_t to_id;
  _GRAF_READ_UINT(graf, slice, &from_id);
  _GRAF_READ_UINT(graf, slice, &to_id);

  if (from_id >= graf->nv || to_id >= graf->nv) {
    graf->detailed_err = GRAF_VERTEX_OOB;
    return GRAF_ERR_PARSE;
  }

  if (graf->segment_flags & GRAF_BIPARTITE) {
    uint32_t part_idx = graf->bipartite_part_v_attr_idx;
    uint32_t from_part = graf->V[from_id].attrs[part_idx].u;
    uint32_t to_part = graf->V[to_id].attrs[part_idx].u;

    if ((from_part != 0 && from_part != 1) ||
        (to_part != 0 && to_part != 1)) {
      graf->detailed_err = GRAF_BIPARTITE_BAD_ARC;
      return GRAF_ERR_PARSE;
    }

    if (from_part == to_part) {
      graf->detailed_err = GRAF_BIPARTITE_BAD_ARC;
      return GRAF_ERR_PARSE;
    }
  }

  arc->from = &graf->V[from_id];
  arc->to = &graf->V[to_id];

  for (uint32_t j = 0; j < graf->a_attr_count; j++) {
    GrafType type = graf->a_attr_types[j];
    GrafAttr *attr = &arc->attrs[j];
    attr->type = type;

    if (type == GRAF_UNUSED) {
      uint32_t unused;
      _GRAF_READ_UINT(graf, slice, &unused);
    }
    else if (type == GRAF_UINT) {
      _GRAF_READ_UINT(graf, slice, &attr->u);
    }
    else if (type == GRAF_STR) {
      _GRAF_READ_STR(graf, slice, &attr->s);
    }
    else {
      graf->detailed_err = GRAF_UNIMPLEMENTED;
      return GRAF_ERR_UNIMPLEMENTED;
    }
  }

  return GRAF_OK;
}

GrafStatus _graf_seg_vertex_label(Graf *graf, GrafSlice *slice) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);

  uint32_t v_attr_idx;
  _GRAF_READ_UINT(graf, slice, &v_attr_idx);

  if (v_attr_idx >= graf->v_attr_count) {
    graf->detailed_err = GRAF_BAD_ATTR_IDX;
    return GRAF_ERR_PARSE;
  }

  graf->label_v_attr_idx = v_attr_idx;

  if (graf->v_attr_types[v_attr_idx] != GRAF_UNUSED) {
    graf->detailed_err = GRAF_ATTR_ALREADY_DEFINED;
    return GRAF_ERR_PARSE;
  }

  graf->v_attr_types[v_attr_idx] = GRAF_STR;
  graf->segment_flags |= GRAF_VERTEX_LABELLED;
  return GRAF_OK;
}

GrafStatus _graf_seg_arc_label(Graf *graf, GrafSlice *slice) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);

  uint32_t a_attr_idx;
  _GRAF_READ_UINT(graf, slice, &a_attr_idx);

  if (a_attr_idx >= graf->a_attr_count) {
    graf->detailed_err = GRAF_BAD_ATTR_IDX;
    return GRAF_ERR_PARSE;
  }

  graf->label_a_attr_idx = a_attr_idx;

  if (graf->a_attr_types[a_attr_idx] != GRAF_UNUSED) {
    graf->detailed_err = GRAF_ATTR_ALREADY_DEFINED;
    return GRAF_ERR_PARSE;
  }

  graf->a_attr_types[a_attr_idx] = GRAF_STR;
  graf->segment_flags |= GRAF_ARC_LABELLED;
  return GRAF_OK;
}

GrafStatus _graf_seg_bipartite(Graf *graf, GrafSlice *slice) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);

  uint32_t part0;
  uint32_t part1;
  _GRAF_READ_UINT(graf, slice, &part0);
  _GRAF_READ_UINT(graf, slice, &part1);

  if (part0 + part1 != graf->nv) {
    graf->detailed_err = GRAF_BIPARTITE_BAD_PART_SIZES;
    return GRAF_ERR_PARSE;
  }

  graf->bipartite_parts[0] = part0;
  graf->bipartite_parts[1] = part1;

  uint32_t v_attr_idx;
  _GRAF_READ_UINT(graf, slice, &v_attr_idx);

  if (v_attr_idx >= graf->v_attr_count) {
    graf->detailed_err = GRAF_BAD_ATTR_IDX;
    return GRAF_ERR_PARSE;
  }

  graf->bipartite_part_v_attr_idx = v_attr_idx;

  if (graf->v_attr_types[v_attr_idx] != GRAF_UNUSED) {
    graf->detailed_err = GRAF_ATTR_ALREADY_DEFINED;
    return GRAF_ERR_PARSE;
  }

  graf->v_attr_types[v_attr_idx] = GRAF_UINT;
  graf->segment_flags |= GRAF_BIPARTITE;
  return GRAF_OK;
}

GrafStatus _graf_seg_vertex_array(Graf *graf, GrafSlice *slice) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(graf->V != NULL);

  for (uint32_t id = 0; id < graf->nv; id++) {
    graf->V[id].id = id;
    GrafStatus status = _graf_read_vertex_record(graf, slice, &graf->V[id]);

    if (status != GRAF_OK) {
      return status;
    }
  }

  return GRAF_OK;
}

GrafStatus _graf_seg_arc_array(Graf *graf, GrafSlice *slice) {
  GRAF_ASSERT(graf != NULL);
  GRAF_ASSERT(slice != NULL);
  GRAF_ASSERT(graf->internal.all_arcs != NULL);

  for (uint32_t i = 0; i < graf->na; i++) {
    GrafArc *arc = &graf->internal.all_arcs[i];
    GrafStatus status = _graf_read_arc_record(graf, slice, arc);

    if (status != GRAF_OK) {
      return status;
    }

    arc->next = arc->from->arcs;
    arc->from->arcs = arc;
  }

  return GRAF_OK;
}

/*
  --------------------------------------------------------------------
  PUBLIC API
  --------------------------------------------------------------------
*/

GrafStatus graf_init(Graf *graf) {
  if (graf == NULL) {
    return GRAF_ERR_USAGE;
  }

  *graf = (Graf) {0};
  // Make sure we don't refer to an existing fd
  graf->internal.fd = -1;

  return GRAF_OK;
}

GrafStatus graf_open(const char *path, Graf *graf) {
  if (graf == NULL) {
    return GRAF_ERR_USAGE;
  }

  if (path == NULL) {
    graf->detailed_err = GRAF_NULLPTR;
    return GRAF_ERR_USAGE;
  }

  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    graf->detailed_err = GRAF_OPEN_FAIL;
    return GRAF_ERR_SYS;
  }

  graf->internal.fd = fd;

  char header[GRAF_HEADER_SIZE];
  size_t n = GRAF_HEADER_SIZE;
  char *ptr = header;

  while (n > 0) {
    ssize_t n_read = read(fd, ptr, n);

    if (n_read == -1) {
      graf->detailed_err = GRAF_READ_FAIL;
      return GRAF_ERR_SYS;
    }

    if (n_read == 0) {
      graf->detailed_err = GRAF_BAD_HEADER;
      return GRAF_ERR_PARSE;
    }

    n -= (size_t) n_read;
    ptr += n_read;
  }

  GRAF_ASSERT((size_t)(ptr - header) == GRAF_HEADER_SIZE);

  GrafSlice header_slice = {header, sizeof(header)};

  if (strncmp(header, "graf", 4) != 0) {
    goto bad_header;
  }
  _graf_slice_advance(&header_slice, 4);

  uint32_t version;
  _GRAF_READ_UINT(graf, &header_slice, &version);

  if (version > GRAF_VERSION) {
    goto bad_header;
  }

  _GRAF_READ_UINT(graf, &header_slice, &graf->nv);
  _GRAF_READ_UINT(graf, &header_slice, &graf->na);
  _GRAF_READ_UINT(graf, &header_slice, &graf->v_attr_count);
  _GRAF_READ_UINT(graf, &header_slice, &graf->a_attr_count);
  _GRAF_READ_UINT(graf, &header_slice, &graf->nchars);

  if (graf->v_attr_count > GRAF_MAX_ATTRS ||
      graf->a_attr_count > GRAF_MAX_ATTRS) {
    goto bad_header;
  }

  return GRAF_OK;

bad_header:
  graf->detailed_err = GRAF_BAD_HEADER;
  return GRAF_ERR_PARSE;
}

GrafStatus graf_load(Graf *graf, GrafVertex *V, GrafArc *A, char *s) {
  if (graf == NULL) {
    return GRAF_ERR_USAGE;
  }

  if (V == NULL || A == NULL || s == NULL) {
    graf->detailed_err = GRAF_NULLPTR;
    return GRAF_ERR_USAGE;
  }

  int fd = graf->internal.fd;

  if (fd == -1) {
    graf->detailed_err = GRAF_NOT_OPENED;
    return GRAF_ERR_USAGE;
  }

  struct stat st;

  if (fstat(fd, &st) == -1) {
    graf->detailed_err = GRAF_STAT_FAIL;
    return GRAF_ERR_SYS;
  }

  if (st.st_size <= GRAF_HEADER_SIZE) {
    graf->detailed_err = GRAF_BAD_HEADER;
    return GRAF_ERR_PARSE;
  }

  size_t n = (size_t) st.st_size - GRAF_HEADER_SIZE;
  GRAF_ASSERT(n > 0);
  char *buf = mmap(NULL, (size_t) st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (buf == (char *) -1) {
    graf->detailed_err = GRAF_MMAP_FAIL;
    return GRAF_ERR_SYS;
  }

  graf->V = V;
  graf->internal.all_arcs = A;
  graf->internal.all_strs = s;
  GrafSlice slice = {buf + GRAF_HEADER_SIZE, n};
  GrafStatus status;

  while (1) {
    uint32_t seg_code;

    status = _graf_read_uint(graf, &slice, &seg_code);

    if (status != GRAF_OK) {
      goto load_done;
    }

    // BEGIN SEGMENT PARSING -----------------------------------------

    if (seg_code == GRAF_SEGCODE_VERTEX_LABEL) {
      status = _graf_seg_vertex_label(graf, &slice);
    }

    else if (seg_code == GRAF_SEGCODE_ARC_LABEL) {
      status = _graf_seg_arc_label(graf, &slice);
    }

    else if (seg_code == GRAF_SEGCODE_BIPARTITE) {
      status = _graf_seg_bipartite(graf, &slice);
    }

    // Parse vertex then arc array
    else if (seg_code == GRAF_SEGCODE_VERTEX_ARRAY) {
      status = _graf_seg_vertex_array(graf, &slice);

      if (status != GRAF_OK) {
        goto load_done;
      }

      uint32_t code;
      status = _graf_read_uint(graf, &slice, &code);

      if (status != GRAF_OK) {
        goto load_done;
      }

      if (code != GRAF_SEGCODE_ARC_ARRAY) {
        graf->detailed_err = GRAF_UNKNOWN_SEG;
        status = GRAF_ERR_PARSE;
        goto load_done;
      }

      status = _graf_seg_arc_array(graf, &slice);

      if (status == GRAF_OK) {
        goto load_done;
      }
    }

    else if (seg_code == GRAF_SEGCODE_ARC_ARRAY) {
      graf->detailed_err = GRAF_VA_NOT_SEEN;
      status = GRAF_ERR_PARSE;
    }

    else {
      graf->detailed_err = GRAF_UNKNOWN_SEG;
      status = GRAF_ERR_PARSE;
    }

    // END SEGMENT PARSING -------------------------------------------

    if (status != GRAF_OK) {
      goto load_done;
    }
  }

load_done:
  munmap(buf, (size_t) st.st_size);

  if (!_graf_registered_printf) {
    _graf_registered_printf = true;
    GRAF_ASSERT(register_printf_function('v', _printf_vertex_handler, _printf_vertex_arginfo) == 0);
  }
  _graf_global = graf;

  return status;
}

GrafStatus graf_close(Graf *graf) {
  if (graf == NULL) {
    return GRAF_ERR_USAGE;
  }

  int fd = graf->internal.fd;

  if (fd == -1) {
    graf->detailed_err = GRAF_NOT_OPENED;
    return GRAF_ERR_USAGE;
  }

  if (close(fd) == -1) {
    graf->detailed_err = GRAF_CLOSE_FAIL;
    return GRAF_ERR_SYS;
  }

  graf->internal.fd = -1;
  _graf_global = NULL;
  return GRAF_OK;
}
