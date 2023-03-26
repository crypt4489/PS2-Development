#ifndef PS_MISC_H
#define PS_MISC_H
#include "ps_global.h"
#include <math3d.h>

void MatrixVectorMultiply(VECTOR out, MATRIX m, VECTOR in);
void MatrixVectorTransform(VECTOR out, MATRIX m, VECTOR in);
MeshBuffers* CreateGrid(int N, int M, float depth, float width, MeshBuffers *buffer);
void CreateGridVectors(int N, int M, float depth, float width, MeshBuffers *buffer);
void CreateGridUVS(int N, int M, float depth, float width, MeshBuffers *buffer);
void CreateGridIndices(int N, int M, float depth, float width, MeshBuffers *buffer);
float DotProduct(VECTOR in1, VECTOR in2);
float dist(VECTOR in);
void RandomVectorsInit(VECTOR in);
void GetRandomVectors(VECTOR in);
void normalize(VECTOR in, VECTOR out);
float distCOP2(VECTOR in);
void CreateProjectionMatrix(MATRIX output, float width, float height, float aspect, float near, float far, float angle);
void CrossProduct(VECTOR m, VECTOR n, VECTOR out);
void CreateRotationMatrix(VECTOR axis, float angle, MATRIX output);
void CreateTranslationMatrix(VECTOR pos, MATRIX output);
void CreateScaleMatrix(VECTOR scales, MATRIX output);
void CreateWorldMatrix(MATRIX output, MATRIX scales, MATRIX rot, MATRIX trans);
void DumpMatrix(MATRIX elem);
void DumpVector(VECTOR elem);
void VectorSubtractXYZ(VECTOR in, VECTOR in2, VECTOR out);
void VectorAddXYZ(VECTOR in, VECTOR in2, VECTOR out);
void dump_packet(qword_t *q);
qword_t *vector_to_qword(qword_t *q, VECTOR v);
void computeNormal(VECTOR v0, VECTOR v1, VECTOR v2, VECTOR out);
float dotProductFour(VECTOR in1, VECTOR in2);
void CreateVector(float x, float y, float z, float w, VECTOR out);
void ScaleVectorXYZ(VECTOR vec, VECTOR input, float scale);
void ZeroVector(VECTOR out);
void MatrixInverse(MATRIX src, MATRIX out);
void MatrixTranspose(MATRIX src);
void CreateNormalizedTextureCoordinateMatrix(MATRIX src);
void CreateOrthoGraphicMatrix(float xLow, float xHigh, float yLow, float yHigh, float near, float far, MATRIX out);
void ComputePlane(VECTOR vec, VECTOR normal, VECTOR plane);
void PointInPlane(VECTOR plane, VECTOR p, VECTOR pointInPlane, VECTOR planePoint);
void ComputeReflectionMatrix(VECTOR normal, MATRIX res);
void SetupPlane(VECTOR planeNormal, VECTOR planePoint, Plane *plane);
void VectorCopyXYZ(VECTOR in, VECTOR out);
//float InnerProductXYZ(VECTOR p1, VECTOR p2);
float DistanceFromPlane(VECTOR planeEquation, VECTOR point);
void Matrix3VectorMultiply(VECTOR out, MATRIX m, VECTOR in);

void GetPositionVectorCopyLTM(MATRIX ltm, VECTOR out);
void SetPositionVectorLTM(MATRIX ltm, VECTOR pos);
void GetRightVectorCopyLTM(MATRIX ltm, VECTOR out);
void GetUpVectorCopyLTM(MATRIX ltm, VECTOR out);
void GetForwardVectorCopyLTM(MATRIX ltm, VECTOR out);
void SetRightVectorLTM(MATRIX ltm, VECTOR right);
void SetUpVectorLTM(MATRIX ltm, VECTOR up);
void SetForwardVectorLTM(MATRIX ltm, VECTOR forward);
void SetScaleVectorLTM(MATRIX ltm, VECTOR scale);
void GetScaleVectorLTM(MATRIX ltm, VECTOR out);
void CreateWorldMatrixLTM(MATRIX ltm, MATRIX m);
void SetRotationVectorsLTM(MATRIX ltm, VECTOR up, VECTOR right, VECTOR forward);
void GetRotationVectorsLTM(MATRIX ltm, VECTOR up, VECTOR right, VECTOR forward);
VECTOR* GetRightVectorLTM(MATRIX m);
VECTOR* GetForwardVectorLTM(MATRIX m);
VECTOR* GetUpVectorLTM(MATRIX m);
VECTOR* GetPositionVectorLTM(MATRIX m);
void CreateWorldMatrixFromVectors(VECTOR pos, VECTOR up, VECTOR forward, VECTOR right, VECTOR scales, MATRIX m);
void CreateRotationAndCopyMatFromObjAxes(MATRIX out, VECTOR up, VECTOR forward, VECTOR right);
MeshBuffers *InitMeshBuffersStruct(u32 count, MeshBuffers *buffer);
u32 GetDirtyLTM(MATRIX m);
void SetDirtyLTM(MATRIX m);
void ClearDirtyLTM(MATRIX m);
float GetLastLTM(MATRIX m);
void SetLastLTM(MATRIX m, float val);
void SetLastAndDirtyLTM(MATRIX m, float w);
void SetupLTM(VECTOR pos, VECTOR up, VECTOR right,
         VECTOR forward, VECTOR scales,
          float q, MATRIX ltm);

void Pathify(const char *name, char *file);
void AppendString(const char *input1, const char *input2, char *output, u32 max);
void NormalizePlane(VECTOR in, VECTOR out);
#endif
