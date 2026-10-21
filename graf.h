#include <stdbool.h>
#include <stdint.h>

#define GRAF_VERSION 0
#define GRAF_HEADER_SIZE 28
#define GRAF_MAX_ATTRS 4

typedef enum {
  GRAF_SEGCODE_VERTEX_LABEL = 0x00000000,
  GRAF_SEGCODE_ARC_LABEL    = 0x00000001,
  GRAF_SEGCODE_BIPARTITE    = 0x00000002,
  GRAF_SEGCODE_VERTEX_ARRAY = 0xFFFFFFFE,
  GRAF_SEGCODE_ARC_ARRAY    = 0xFFFFFFFF,
} GrafSegCodes;

typedef enum {
  GRAF_OK,
  GRAF_ERR_SYS,
  GRAF_ERR_PARSE,
  GRAF_ERR_USAGE,
  GRAF_ERR_UNIMPLEMENTED,
} GrafStatus;

typedef enum {
  GRAF_UNUSED,
  GRAF_UINT,
  GRAF_STR,
} GrafType;

typedef enum {
  GRAF_VERTEX_LABELLED = 1,
  GRAF_ARC_LABELLED    = 1 << 1,
  GRAF_BIPARTITE       = 1 << 2,
} GrafFlag;

typedef struct {
  GrafType type;
  union {
    uint32_t u;
    float f;
    const char *s;
  };
} GrafAttr;

struct _GrafArc;
struct _GrafVertex;

struct _GrafVertex {
  uint32_t id;
  struct _GrafArc *arcs;
  GrafAttr attrs[GRAF_MAX_ATTRS];
};

struct _GrafArc {
  struct _GrafVertex *from;
  struct _GrafVertex *to;
  struct _GrafArc *next;
  GrafAttr attrs[GRAF_MAX_ATTRS];
};

typedef struct _GrafArc GrafArc;
typedef struct _GrafVertex GrafVertex;

typedef struct {
  int fd;
  GrafArc *all_arcs;
  char *all_strs;
  size_t all_strs_offset;
} GrafInternal;

typedef struct {
  uint32_t v;
  uint32_t a;
  uint32_t v_attr_count;
  uint32_t a_attr_count;
  uint32_t char_count;

  GrafVertex *V;

  uint64_t segment_flags;

  GrafType v_attr_types[GRAF_MAX_ATTRS];
  GrafType a_attr_types[GRAF_MAX_ATTRS];

  uint32_t label_v_attr_idx;
  uint32_t label_a_attr_idx;

  uint32_t bipartite_parts[2];
  uint32_t bipartite_part_v_attr_idx;

  GrafInternal internal;
  const char *detailed_err;
} Graf;

/*
  Initialise graf to some sane values.

  Errors:
  GRAF_ERR_USAGE if a null pointer was psased
*/
GrafStatus graf_init(Graf *graf);

/*
  Read first few bytes of file at path to determine the size of
  required buffers. Populate the header values for application use.

  Errors:
  GRAF_ERR_USAGE if a null pointer was passed
  GRAF_ERR_SYS if cannot open or read file
  GRAF_ERR_PARSE if GRAF header is malformed
*/
GrafStatus graf_open(const char *path, Graf *graf);

/*
  Load the graph with user-supplied buffers, with sizes requested by
  graf_open(). Must be called after graf_open() and at most once.

  V, A, s must have sizes graf->v, graf->a, graf->char_count
  respectively.

  Errors:
  GRAF_ERR_USAGE if a null pointer was psased
  GRAF_ERR_SYS if cannot mmap or close file
  GRAF_ERR_PARSE if GRAF body is malformed
*/
GrafStatus graf_load(Graf *graf, GrafVertex *V, GrafArc *A, char *s);

/*
  Close the file opened by graf_open(). This should be called for
  cleanup.

  Note: This can fail if opening the file failed in the first place.

  Errors:
  GRAF_ERR_USAGE if a null pointer was passed, or a file was not
  opened in the first place
  GRAF_ERR_SYS if close fails
*/
GrafStatus graf_close(Graf *graf);
