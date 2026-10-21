# GRAF v0.0

GRAF is a binary file format for graphs, designed to be easy-to-parse
yet flexible. It aims to support various types of graphs: simple,
directed, weighted, bipartite, and so on.

Currently GRAF understands two data types: UINT (4 bytes, unsigned)
and STR (a null-terminated UTF-8 sequence). UINTs are stored in big
endian. Some noteworthy consequences:

- GRAF supports up to 2^32-1 vertices.
- The codepoint U+0000 cannot be encoded in a GRAF string.

GRAF primarily deals with 'arcs' which are directed edges --- this
reflects the internal representation of the graph as per-vertex
adjacency lists. An undirected graph can be defined by having two arcs
for each edge.

# Repo contents

This repository contains a reference implementation of the GRAF parser
and the `graf-info` utility. The release includes the compiled library
`libgraf.a`, a public header `graf.h` with documented routines, and
the `graf-info` executable.

# GRAF file format

The header consists of the following:

1. The magic bytes `graf`
2. (UINT) Version number (currently `0`)
3. (UINT) Vertex count
4. (UINT) Arc count
5. (UINT) Vertex attribute count (`0`-`4`)
6. (UINT) Arc attribute count (`0`-`4`)
7. (UINT) Total length of all strings in file, including null-terminators

This is followed by a list of segments, each starting with a (UINT)
segment code. The last two segments must be vertex and arc array
respectively; everything afterward is ignored.

# Segments

Some segments define the meaning of an attribute in vertex or arc
records. If an attribute is not defined, it is treated as a (UINT) and
ignored. It is not allowed for the same attribute to be defined
multiple times. Attribute indices cannot exceed the values specified
in the header.

## Vertex labels

1. (UINT) Code = `00 00 00 00`
2. (UINT) Vertex attribute index for label

Note:
- The vertex attribute at the specified index must be a (STR).

## Arc labels

1. (UINT) Code = `00 00 00 01`
2. (UINT) Arc attribute index for label

Note:
- The arc attribute at the specified index must be a (STR).

## Bipartite

1. (UINT) Code = `00 00 00 02`
2. (UINT) Size of part 0
3. (UINT) Size of part 1
4. (UINT) Vertex attribute index for part number

Note:
- The vertex attribute at the specified index must be a (UINT) with
  value `0` or `1`.
- The sum of part sizes must match the vertex count specified in the
  header.
- All arcs in the arc array segment must cross between parts 0 and 1.

## Vertex array

1. (UINT) Code = `FF FF FF FE`
2. A packed array of vertex records, where each record is a list of
attributes defined according to other segments.

Note:
- The records must appear in vertex id sequence.
- The number of records, and the number of attributes within each
record must match the values specified in the header.

## Arc array

1. (UINT) Code = `FF FF FF FF`
2. A packed array of arc records, where each record starts with a
start and end vertex id (UINT). This is followed by a list of
attributes, defined according to other segments.

Note:
- The number of records, and the number of attributes within each
record must match the values specified in the header.
- Duplicate arcs and self-arcs are allowed.
- It is recommended for the arcs to be sorted by the start vertex
id. This makes it easier for implementations to store the arcs with a
given start vertex close in memory.

# Example

Here is the complete graph K_4 with vertex set {A,B,C,D}, stored in
GRAF format. The newlines and comments are merely illustrative: the
actual file is just a raw sequence of bytes.

```
67 72 61 66  # "graf"
00 00 00 00  # version 0
00 00 00 04  # 4 vertices
00 00 00 0C  # 12 arcs
00 00 00 01  # 1 vertex attribute (label)
00 00 00 00  # 0 arc attributes
00 00 00 08  # 8 chars for all strings
00 00 00 00  # SEGMENT: vertex label
00 00 00 00  # attribute index 0
FF FF FF FE  # SEGMENT: vertex array
41 00  # vertex 0: "A"
42 00  # vertex 1: "B"
43 00  # vertex 2: "C"
44 00  # vertex 3: "D"
FF FF FF FF  # SEGMENT: arc array
00 00 00 00 00 00 00 01  # 0->1
00 00 00 00 00 00 00 02  # 0->2
00 00 00 00 00 00 00 03  # 0->3
00 00 00 01 00 00 00 00  # 1->0
00 00 00 01 00 00 00 02  # 1->2
00 00 00 01 00 00 00 03  # 1->3
00 00 00 02 00 00 00 00  # 2->0
00 00 00 02 00 00 00 01  # 2->1
00 00 00 02 00 00 00 03  # 2->3
00 00 00 03 00 00 00 00  # 3->0
00 00 00 03 00 00 00 01  # 3->1
00 00 00 03 00 00 00 02  # 3->2
```

As can be seen, compactness is not a goal of GRAF.