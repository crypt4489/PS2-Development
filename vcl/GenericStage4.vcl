#include "my_vcl.inc"

#vuprog VU1_ClipStage4

START:

    MatrixLoad{ viewProj, 0, vi00 }

	fcset   0x000000

    lq.xyz  scale,          8(vi00)

    ilw.w   renderFlags,    12(vi00)

    iaddiu  useSTQ,            vi00,         0x0040
    iand    useSTQ,           renderFlags,  useSTQ

    iaddiu  useColor,            vi00,         0x0080
    iand    useColor,            renderFlags,  useColor

    add     outColor,       vf00,       vf00
begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,        1
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,          end
    ilw.x   countTag,       10(vi00)
    ior     countTag,       countTag,     vertCount
    iadd    destAddress,    vertexData,   vertCount
    ibeq    useSTQ,          vi00,         colorData
    iadd    destAddress,    destAddress,  vertCount

colorData:
    ibeq    useColor,          vi00,         createColorOut
    iadd    destAddress,    destAddress,  vertCount
    b       writeCountTag
createColorOut:
    lq     outColor, 9(vi00)

writeCountTag:
    iadd kickAddress, vi00, destAddress
    lq.yzw  primTag,    10(vi00)
    sq.yzw  primTag,    0(destAddress)
    isw.x   countTag,   0(destAddress)
    iaddiu  destAddress, destAddress, 1


    iadd vertexCounter, iBase, vertCount



    vertexLoop:

        iadd  inPtr, vi00, vertexData

        lq vertex, 0(inPtr)


        MatrixMultiplyVertex{ vertex, viewProj, vertex }

        clipw.xyz	vertex, vertex
        fcand		VI01,   0x3FFFF
        iaddiu		iADC,   VI01,       0x7FFF



        div         q,      vf00[w],    vertex[w]
        mul.xyz     vertex, vertex,     q
        mula.xyz    acc,    scale,      vf00[w]
        madd.xyz    vertex, vertex,     scale
        ftoi4.xyz   vertex, vertex

        ibeq    useSTQ,            vi00,         loadColor
        iadd    inPtr,          inPtr,        vertCount
        lq      stq,            0(inPtr)
        mulq    modStq,         stq,          q
        sq      modStq,         0(destAddress)
        iaddiu  destAddress,    destAddress,    1

loadColor:

        ibeq    useColor,            vi00,         Data
        iadd    inPtr,          inPtr,        vertCount
        lq      outColor,       0(inPtr)



Data:
        sq outColor,    0(destAddress)
        sq.xyz vertex,  1(destAddress)
        isw.w		iADC,   1(destAddress)

        iaddiu          destAddress,    destAddress,    2
        iaddiu          vertexData,     vertexData,     1

        iaddi   vertexCounter,  vertexCounter,  -1
        ibne    vertexCounter,  iBase,   vertexLoop

    --barrier

    xgkick kickAddress
end:

    --cont

    b begin

#endvuprog