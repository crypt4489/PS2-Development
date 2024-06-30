#include "my_vcl.inc"

#vuprog VU1_ShadowExtrusion

START:

    MatrixLoad{ viewProj, 0, vi00 }

    MatrixLoad{ world, 4, vi00 }

	fcset   0x000000

    lq.xyz  scale,          8(vi00)

    
begin:
    xtop    iBase

    lq     outColor, 9(vi00)
    lq.xyz     lightPos, 12(vi00)

    iaddiu  vertexData,     iBase,        1
    ilw.w   vertCount,      0(iBase)
    iadd    destAddress,    vertexData,   vertCount
    iadd    outCount,       vi00, vi00


    iadd kickAddress, vi00, destAddress
    lq.yzw  primTag,    10(vi00)
    sq.yzw  primTag,    0(destAddress)
    iaddiu  destAddress, destAddress, 1


    iadd vertexCounter, iBase, vertCount



    vertexLoop:

        lq.xyz neighbors, 0(vertexData)

        add.xyz neighbors, neighbors, vf00

        ilw.w  disp, 0(vertexData)

        iaddiu  comp, vi00, 0x00e0

        fmand triNeighbors, comp

        iaddiu vertexData,     vertexData,     1

        lq vertex, 0(vertexData)
        lq vertex1, 1(vertexData)
        lq vertex2, 2(vertexData)

        MatrixMultiplyVertex{ vertex, world, vertex }
        MatrixMultiplyVertex{ vertex1, world, vertex1 }
        MatrixMultiplyVertex{ vertex2, world, vertex2 }

        sub.xyz edge1, vertex1, vertex
        sub.xyz edge2, vertex2, vertex1

        VectorCrossProduct{ edge1, edge2, cross }

        sub.xyz util, lightPos, vertex

        VectorDotProduct{ dot, cross, util }

        iaddiu  comp, vi00, 0x0088

        fmand res, comp

        ibne res, vi00, adjTest

        iadd vertexData, vertexData, disp

        b vertexLoop

adjTest:
        add dists, vf00, vf00
        addw.x dists, vf00, vf00
        addw.y dists, vf00, vf00
        addw.z dists, vf00, vf00

        add.xyz vertex5, vf00, vf00

        iaddiu   counter, vertexData, 3

        iaddiu  comp, vi00, 0x0080

        iand    comp, triNeighbors, comp

        ibne  comp, vi00, distAB

        move  writeVer1, vertex

        move  writeVer2, vertex1
      
        bal ret, writeOut

        b check2Face

distAB:

        move.xyz vertex5, vertex1

        bal  ret, dotFace

        move.x dists, dot
check2Face:
        iaddiu  comp, vi00, 0x0040

        iand    comp, triNeighbors, comp

        ibne  comp, vi00, distBC

        move  writeVer1, vertex1

        move  writeVer2, vertex2

        
        bal ret, writeOut

        b check3Face


distBC:
        move.xyz vertex5, vertex2

        move.xyz edge1, edge2

        bal  ret, dotFace

       addx.y dists, vf00, dot[x]
check3Face:
        iaddiu  comp, vi00, 0x0020

        iand    comp, triNeighbors, comp

        ibne  comp, vi00, distCA

        move  writeVer1, vertex2

        move  writeVer2, vertex

        bal ret, writeOut

        b checkDists


distCA:

        sub.xyz edge1, vertex, vertex2

        move.xyz vertex5, vertex2

        bal  ret, dotFace

        addx.z dists, vf00, dot[x]
checkDists:
        add.xyz dists, vf00, dists

        iaddiu  comp, vi00, 0x00ee

        fmand triNeighbors, comp

        iaddiu  comp, vi00, 0x0088

        iand comp, comp, triNeighbors

        ibne comp, vi00, checkDistB

        move  writeVer1, vertex

        move  writeVer2, vertex1

        bal ret, writeOut

checkDistB:


        iaddiu  comp, vi00, 0x0044

        iand comp, comp, triNeighbors

        ibne comp, vi00, checkDistC

        move writeVer1, vertex1

        move  writeVer2, vertex2

        bal ret, writeOut

checkDistC:


        iaddiu  comp, vi00, 0x0022

        iand comp, comp, triNeighbors

        ibne comp, vi00, Data

        move  writeVer1, vertex2

        move  writeVer2, vertex

        bal ret, writeOut

Data:
    

        iaddiu          vertexData,     vertexData,     disp

        iaddi   vertexCounter,  vertexCounter,  -1
        isub    vertexCounter,  vertexCounter, disp 
        ibne    vertexCounter,  iBase,   vertexLoop

        ilw.x   countTag,       10(vi00)
        ior     countTag,       countTag,     outCount
        isw.x   countTag,       0(kickAddress)

        b end

dotFace:

    lq vertex4, 0(counter)

    iaddiu counter, counter, 1

    MatrixMultiplyVertex{ vertex4, world, vertex4 }

    sub.xyz vertex4, vertex4, vertex5

    VectorCrossProduct{ vertex4, edge1, cross }

    sub.xyz vertex4, lightPos, vertex5

    VectorDotProduct{ dot, cross, vertex4 }

    jr ret

writeOut:

    sub.xyz v3, lightPos, writeVer1
    add.xyz v3, v3, v3
    sub.xyz v4, lightPos, writeVer2
    add.xyz v4, v4, v4
    add.xyz v3, v3, writeVer1
    add.xyz v4, v4, writeVer2
    move.w v4, vf00
    move.w v3, vf00
    MatrixMultiplyVertex{ writeVer1, viewProj, writeVer1 }
    MatrixMultiplyVertex{ writeVer2, viewProj, writeVer2 }
    MatrixMultiplyVertex{ v3, viewProj, v3 }
    MatrixMultiplyVertex{ v4, viewProj, v4 }
    move cVertex, writeVer1
    bal ret2, clipp
    move cVertex, v3
    bal ret2, clipp
    move cVertex, writeVer2
    bal ret2, clipp
    move cVertex, v3
    bal ret2, clipp
    move cVertex, writeVer2
    bal ret2, clipp
    move cVertex, v4
    bal ret2, clipp
    iaddiu outCount, outCount, 6
    jr ret


clipp:
    clipw.xyz	cVertex, cVertex
    fcand		VI01,   0x3FFFF
    iaddiu		iADC,   VI01,       0x7FFF



    div         q,      vf00[w],    cVertex[w]
    mul.xyz     cVertex, cVertex,     q
    mula.xyz    acc,    scale,      vf00[w]
    madd.xyz    cVertex, cVertex,     scale
    ftoi4.xyz   cVertex, cVertex 
    sq outColor,    0(destAddress)
    sq.xyz cVertex,  1(destAddress)
    isw.w		iADC,   1(destAddress)
    iaddiu destAddress, destAddress, 2
    jr ret2   

end:

    --cont

    b begin

#endvuprog