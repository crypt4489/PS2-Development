#ifndef PS_ADJACENCY_H
#define PS_ADJACENCY_H

#include "ps_global.h"
FaceVertexTable ComputeFaceToVertexTable(VECTOR *vertices, u32 numVertices);
VECTOR *CreateAdjacencyVertices(FaceVertexTable table, VECTOR *verts, u32 numVerts, u32 *numAdjVerts);
#endif