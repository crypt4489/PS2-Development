

#include "my_vcl.inc"

.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter


#vuprog VU1_GenericBonesAnimStage1

START:

    ilw.x      bonesVectorsPosition,     11(vi00)

    ilw.w   renderFlags,    12(vi00)

    iaddiu  useSTQ,            vi00,         0x0040
    iand    useSTQ,           renderFlags,  useSTQ

    iaddiu  useColor,            vi00,         0x0080
    iand    useColor,            renderFlags,  useColor

    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    iadd    bones,    vertexData,   vertCount
    ibeq    useSTQ,          vi00,         colorData
    iadd    bones,    bones,  vertCount
colorData:
    ibeq    useColor,          vi00,      continue
    iadd    bones,    bones,  vertCount
continue:
    iadd weights, bones, vertCount
    iadd vertexCounter, iBase, vertCount ; loop vertCount times
    vertexLoop:
        move.xyz        totalPosition,  vf00
        mr32.w          totalPosition, vf00
        lq vertex,      0(vertexData)
        lq bone,        0(bones)
        lq weights,     0(weights)
        iaddiu          boneCounter,    vi00,           4
        iaddiu            boneChange,     vi00,           0
boneLoop:

        mtir            boneID,  bone[x]
        ibltz           boneID,  nextBone
        iadd            boneTransformLocation,  bonesVectorsPosition, boneID
        iadd            boneTransformLocation,  boneTransformLocation, boneID
        iadd            boneTransformLocation,  boneTransformLocation, boneID
       ; iadd            boneTransformLocation,  boneTransformLocation, boneID

        lq              rot, 1(boneTransformLocation)
        lq.xyz          scale, 2(boneTransformLocation)
        mr32        rot, rot
        QuaternionToMatrix{ boneRot, rot }
        MatrixTranspose{ boneTransform, boneRot }
        mulx.xyz    boneTransform[0], boneTransform[0], scale[x]
        muly.xyz    boneTransform[1], boneTransform[1], scale[y]
        mulz.xyz    boneTransform[2], boneTransform[2], scale[z]
        lq          boneTransform[3], 0(boneTransformLocation)

        MatrixMultiplyVertex{ pos, boneTransform, vertex }

        mul             pos,      pos,               weights[x]

        add             totalPosition, totalPosition, pos

        iaddiu          boneChange, vi00,  1
nextBone:
        mr32            weights,  weights
        mr32            bone,     bone
        iaddi           boneCounter,  boneCounter,      -1
        ibne            boneCounter,  vi00,             boneLoop

        ibeq            boneChange, vi00, nextVertex
        sq              totalPosition,  0(vertexData)

nextVertex:
        iaddiu          vertexData,     vertexData,     1
        iaddiu          bones,          bones,          1
        iaddiu          weights,        weights,        1

        iaddi   vertexCounter,  vertexCounter,  -1	; decrement the loop counter
        ibne    vertexCounter,  iBase,   vertexLoop	; and repeat if needed

        .vsm
           NOP             ilw.y   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm



    --exit
    --endexit

#endvuprog
