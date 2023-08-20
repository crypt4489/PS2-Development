


#include "my_vcl.inc"

#vuprog VU1_ClippingStage

START:
    lq.xyz      camProps,          13(vi00)
    lq        camQuat,           14(vi00)
    lq.xyz    camPos,            15(vi00)

    ilw.w   renderFlags,    12(vi00)

    iaddiu  useSTQ,            vi00,            0x0040
    iand    useSTQ,            renderFlags,     useSTQ

    iaddiu  useColor,            vi00,           0x0080
    iand    useColor,            renderFlags,    useColor

    xtop    iBase

    iaddiu  vertexData,     iBase,      1          ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)
    iadd    clippedBuffer,     vertexData,    vertCount
    ;iaddiu  clippedBuffer,     vertexData,     243
    ibeq    useSTQ,            vi00,           addColors
    iadd  clippedBuffer,     clippedBuffer,     vertCount
addColors:
    ibeq    useColor,           vi00,         finishSetup
    iadd  clippedBuffer,     clippedBuffer,     vertCount
finishSetup:
    ;////////////////////////////////////////////

    iadd vertexCounter, iBase, vertCount ; loop vertCount times

    mr32    camQuat, camQuat

    QuaternionToMatrix{ tempMat, camQuat }

    MatrixTranspose{ out, tempMat }

    sub.xyz     plane, vf00,  out[2]
    mulx.xyz    nearP1, plane, camProps
    add.xyz   nearP1,   nearP1,  camPos
    sub.xyz  planeVec, vf00, nearP1
    VectorDotProduct{ dot, planeVec, plane }
    mr32.w plane, dot

    MatrixLoad{ globalMatrix, 4, vi00 }

    iadd clippedVertices, vi00, vi00

    iaddiu planes, vi00, 4


    vertexLoop:


        iadd  clipData, vi00, vertexData

        move  stq1, vf00
        move  stq2, vf00
        move  stq3, vf00

        move  color1, vf00
        move  color2, vf00
        move  color3, vf00

        lq vertex1, 0(clipData)
        lq vertex2, 1(clipData)
        lq vertex3, 2(clipData)

        MatrixMultiplyVertex{ gvertex1, globalMatrix, vertex1 }
        MatrixMultiplyVertex{ gvertex2, globalMatrix, vertex2 }
        MatrixMultiplyVertex{ gvertex3, globalMatrix, vertex3 }

        SignedDistanceof3Vectors{ distance, gvertex1, gvertex2, gvertex3, plane }

        add.xyz fake, distance, vf00

        iaddiu  comp, vi00, 0x00e0

        fmand res, comp


        ibeq res, comp, next_verts

        move o_vertex1, vertex1

        move o_vertex2, vertex2

        move o_vertex3, vertex3
        iaddiu clippedVertices, clippedVertices, 3
        ibne res, vi00, checkAB

        bal  ret, write_to_clipbuffer

        ibeq    useSTQ,            vi00,           next_verts
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            write_to_clipbuffer
all_pass_color:

        ibeq    useColor,            vi00,           next_verts
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            write_to_clipbuffer
        b       next_verts
checkAB:


        iaddiu  comp, vi00, 0x00c0

        ibne res, comp, checkBC

        move tempIntersect, vertex3

        move tempCalc1, vertex1

        move tempCalc2, vertex2

        move.xyz gIntersect, gvertex3

        move.xyz gCalc1, gvertex1

        move.xyz gCalc2, gvertex2

        bal    ret, calculate_intersect

        move o_vertex1, tempCalc1

        move o_vertex2, tempCalc2

        bal  ret, write_to_clipbuffer
        ibeq    useSTQ,            vi00,           loadColorsAB
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            ABIntersect
        bal     ret,            write_to_clipbuffer

loadColorsAB:
        ibeq    useColor,            vi00,           next_verts
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            ABIntersect
        bal     ret,            write_to_clipbuffer
        b next_verts
ABIntersect:
        move tempIntersect1, o_vertex3

        move tempCalc3, o_vertex1

        move tempCalc4, o_vertex2
        bal     ret2,       lerpValues

        move o_vertex1, tempCalc3

        move o_vertex2, tempCalc4

        jr ret
checkBC:
        iaddiu  comp, vi00, 0x0060

        ibne  res, comp, checkCA

         move tempIntersect, vertex1

        move tempCalc1, vertex2

        move tempCalc2, vertex3

        move.xyz gIntersect, gvertex1

        move.xyz gCalc1, gvertex2

        move.xyz gCalc2, gvertex3

        bal    ret, calculate_intersect

        move o_vertex2, tempCalc1

        move o_vertex3, tempCalc2

        bal ret, write_to_clipbuffer
        ibeq    useSTQ,            vi00,           loadColorsBC
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            BCIntersect
        bal     ret,            write_to_clipbuffer

loadColorsBC:
        ibeq    useColor,            vi00,           next_verts
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            BCIntersect
        bal     ret,            write_to_clipbuffer
        b next_verts
BCIntersect:
        move tempIntersect1, o_vertex1

        move tempCalc3, o_vertex2

        move tempCalc4, o_vertex3

        bal     ret2,       lerpValues

        move o_vertex2, tempCalc3

        move o_vertex3, tempCalc4

        jr ret
checkCA:
        iaddiu comp, vi00, 0x00A0

        ibne  res, comp, checkA

        move tempIntersect, vertex2

        move tempCalc1, vertex1

        move tempCalc2, vertex3

        move.xyz gIntersect, gvertex2

        move.xyz gCalc1, gvertex1

        move.xyz gCalc2, gvertex3

        bal    ret, calculate_intersect

        move o_vertex1, tempCalc1

        move o_vertex3, tempCalc2

        bal  ret, write_to_clipbuffer
        ibeq    useSTQ,            vi00,           loadColorsCA
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            CAIntersect
        bal     ret,            write_to_clipbuffer

loadColorsCA:
        ibeq    useColor,            vi00,           next_verts
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            CAIntersect
        bal     ret,            write_to_clipbuffer
        b next_verts
CAIntersect:
        move tempIntersect1, o_vertex2

        move tempCalc3, o_vertex1

        move tempCalc4, o_vertex3

        bal     ret2,       lerpValues

        move o_vertex1, tempCalc3

        move o_vertex3, tempCalc4

        jr ret
checkA:

        iaddiu clippedVertices, clippedVertices, 3

        iaddiu comp, vi00, 0x0080

        ibne  res, comp, checkB

        move tempIntersect, vertex1

        move tempCalc1, vertex2

        move tempCalc2, vertex3

        move.xyz gIntersect, gvertex1

        move.xyz gCalc1, gvertex2

        move.xyz gCalc2, gvertex3

        bal    ret, calculate_intersect

        move o_vertex1, vertex2

        move o_vertex2, vertex3

        move o_vertex3, tempCalc1

        bal    ret, write_to_clipbuffer

        ibeq    useSTQ,            vi00,           loadColorsA1
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            AIntersect
        bal     ret,            write_to_clipbuffer
        move.xyz stq1,          tempCalc3
        move.xyz stq2,          tempIntersect1
        move.xyz stq3,          tempCalc4

loadColorsA1:
        ibeq    useColor,            vi00,           write_A_p2
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            AIntersect
        bal     ret,            write_to_clipbuffer
        move.xyz color1,          tempCalc3
        move.xyz color2,          tempIntersect1
        move.xyz color3,          tempCalc4
write_A_p2:
        move o_vertex1, tempCalc1


        move o_vertex2, vertex3

        move o_vertex3, tempCalc2

        bal    ret, write_to_clipbuffer

        ibeq    useSTQ,            vi00,           loadColorsA2
        move o_vertex1, stq1

        move o_vertex2, stq2

        move o_vertex3, stq3

        bal     ret, write_to_clipbuffer
loadColorsA2:
        ibeq    useColor,            vi00,           next_verts

        move o_vertex1, color1

        move o_vertex2, color2

        move o_vertex3, color3

        bal     ret, write_to_clipbuffer

        b next_verts

AIntersect:
        move tempIntersect1, o_vertex1

        move tempCalc3, o_vertex2

        move tempCalc4, o_vertex3

        bal     ret2,       lerpValues

      move o_vertex1, o_vertex2

        move o_vertex2, o_vertex3

        move  tempIntersect1, o_vertex3

        move o_vertex3, tempCalc3


        jr ret
checkB:

        iaddiu comp, vi00, 0x0040

        ibne  res, comp, checkC

        move tempIntersect, vertex2

        move tempCalc1, vertex1

        move tempCalc2, vertex3

        move.xyz gIntersect, gvertex2

        move.xyz gCalc1, gvertex1

        move.xyz gCalc2, gvertex3

        bal    ret, calculate_intersect

       move o_vertex1, vertex3

        move o_vertex2, vertex1

        move o_vertex3, tempCalc2

        bal    ret, write_to_clipbuffer


        ibeq    useSTQ,            vi00,           loadColorsB1
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            BIntersect
        bal     ret,            write_to_clipbuffer
        move stq1,          tempCalc4
        move stq2,          tempIntersect1
        move stq3,          tempCalc3

loadColorsB1:
        ibeq    useColor,            vi00,           write_B_p2
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            BIntersect
        bal     ret,            write_to_clipbuffer
        move color1,          tempCalc4
        move color2,          tempIntersect1
        move color3,          tempCalc3
write_B_p2:
        move o_vertex1, tempCalc2

        move o_vertex2, vertex1

        move o_vertex3, tempCalc1

        bal    ret, write_to_clipbuffer
        ibeq    useSTQ,            vi00,           loadColorsB2
     move o_vertex1, stq1

        move o_vertex2, stq2

        move o_vertex3, stq3

        bal     ret, write_to_clipbuffer
loadColorsB2:
        ibeq    useColor,            vi00,           next_verts

        move o_vertex1, color1

        move o_vertex2, color2

        move o_vertex3, color3

        bal     ret, write_to_clipbuffer

        b next_verts

BIntersect:
        move tempIntersect1, o_vertex2

        move tempCalc3, o_vertex1

        move tempCalc4, o_vertex3

        bal     ret2,       lerpValues

        move tempIntersect1, o_vertex1

        move o_vertex1, o_vertex3

        move o_vertex2, tempIntersect1

        move o_vertex3, tempCalc4

        jr ret


checkC:

        iaddiu comp, vi00, 0x0020

        ibne  res, comp, next_verts

        move tempIntersect, vertex3

        move tempCalc1, vertex1

        move tempCalc2, vertex2

        move.xyz gIntersect, gvertex3

        move.xyz gCalc1, gvertex1

        move.xyz gCalc2, gvertex2

        bal    ret, calculate_intersect

        move o_vertex1, vertex2

        move o_vertex2, vertex1

        move o_vertex3, tempCalc2

        bal    ret, write_to_clipbuffer


        ibeq    useSTQ,            vi00,           loadColorsC1
        iadd   clipData,       clipData,        vertCount
        bal     ret,            loadData
        bal     ret,            CIntersect
        bal     ret,            write_to_clipbuffer
        move stq1,          tempCalc4
        move stq2,          tempIntersect1
        move stq3,          tempCalc3

loadColorsC1:
        ibeq    useColor,            vi00,           write_C_p2
        iadd   clipData,       clipData,       vertCount
        bal     ret,            loadData
        bal     ret,            CIntersect
        bal     ret,            write_to_clipbuffer
        move color1,          tempCalc4
        move color2,          tempIntersect1
        move color3,          tempCalc3
write_C_p2:
        move o_vertex1, tempCalc2

        move o_vertex2, vertex1

        move o_vertex3, tempCalc1

         bal    ret, write_to_clipbuffer

        ibeq    useSTQ,            vi00,           loadColorsC2
        move o_vertex1, stq1

        move o_vertex2, stq2

        move o_vertex3, stq3

        bal     ret, write_to_clipbuffer
loadColorsC2:
        ibeq    useColor,            vi00,           next_verts

        move o_vertex1, color1

        move o_vertex2, color2

        move o_vertex3, color3

        bal     ret, write_to_clipbuffer

        b next_verts

CIntersect:
        move tempIntersect1, o_vertex3

        move tempCalc3, o_vertex1

        move tempCalc4, o_vertex2

        bal     ret2,       lerpValues

        move tempIntersect1, o_vertex1

        move o_vertex1, o_vertex2

        move o_vertex2, tempIntersect1

        move o_vertex3, tempCalc4

        jr ret

    lerpValues:
      sub  temp, tempCalc3, tempIntersect1

      mul  temp, temp, inter[x]

      add tempCalc3, tempIntersect1, temp

        sub  temp, tempCalc4, tempIntersect1

      mul  temp, temp, inter[y]

      add tempCalc4, tempIntersect1, temp

      jr ret2

    loadData:
        lq      o_vertex1, 0(clipData)
        lq      o_vertex2, 1(clipData)
        lq      o_vertex3, 2(clipData)
        jr      ret
    calculate_intersect:
        sub.x  negPlane, vf00, plane[w]
        VectorDotProduct{ temp1, plane, gIntersect }
        sub.xyz  temp2,  gCalc1, gIntersect
        VectorDotProduct{ temp3, temp2, plane }
        subx.x temp4, negPlane, temp1
        div  q, temp4[x], temp3[x]
        sub.xyz temp2, tempCalc1, tempIntersect
        add.x  inter,  vf00, q
        mul.xyz temp2, temp2, inter[x]
        add.xyz tempCalc1, tempIntersect, temp2
        sub.xyz  temp2,  gCalc2, gIntersect
        VectorDotProduct{ temp3, temp2, plane }
        div  q, temp4[x], temp3[x]
        sub.xyz temp2, tempCalc2, tempIntersect
        add.y  inter,  vf00, q
        mul.xyz temp2, temp2, inter[y]
        add.xyz tempCalc2, tempIntersect, temp2
        jr ret
    write_to_clipbuffer:

        sq o_vertex1, 0(clippedBuffer)
        sq o_vertex2, 1(clippedBuffer)
        sq o_vertex3, 2(clippedBuffer)
        iaddiu clippedBuffer, clippedBuffer, 3
        jr ret

    next_verts:
        iaddiu   vertexData, vertexData, 3
        iaddi   vertexCounter,  vertexCounter,  -3
        ibne    vertexCounter,  iBase,   vertexLoop
        ibeq    clippedVertices, vi00, end
        bal      ret,  write_verts
do_plane_check:

        iaddiu  ret, vi00, 4

        ibne planes, ret, right_plane

        muly.xyz tempP1, out[0], camProps

        add.xyz tempP1, nearP1, tempP1

        sub.xyz planeNormal, tempP1, camPos

        Normalize{ planeNormal, planeNormal, temp }

        VectorCrossProduct{  out[1], planeNormal, planeNormal }




        sub.xyz  planeVec, vf00, tempP1
        VectorDotProduct{ dot, planeVec, planeNormal }
        move.xyz plane, planeNormal
        mr32.w plane, dot

        sq    plane, 1(vi00)

        iaddi planes, planes, -1
        iadd    vertexCounter, iBase, clippedVertices
        iadd clippedVertices, vi00, vi00
        b vertexLoop


right_plane:
        iaddiu  ret, vi00, 3

        ibne planes, ret, bottom_plane

         muly.xyz tempP1, out[0], camProps

        sub.xyz tempP1, nearP1, tempP1

        sub.xyz planeNormal, tempP1, camPos

        Normalize{ planeNormal, planeNormal, temp }

        VectorCrossProduct{ planeNormal,  out[1], planeNormal }




        sub.xyz  planeVec, vf00, tempP1
        VectorDotProduct{ dot, planeVec, planeNormal }
        move.xyz plane, planeNormal
        mr32.w plane, dot



        iaddi planes, planes, -1
        iadd    vertexCounter, iBase, clippedVertices
        iadd clippedVertices, vi00, vi00
        b vertexLoop

bottom_plane:
        iaddiu  ret, vi00, 2

        ibne planes, ret, top_plane

          mulz.xyz tempP1, out[1], camProps

        sub.xyz tempP1, nearP1, tempP1

        sub.xyz planeNormal, tempP1, camPos

        Normalize{ planeNormal, planeNormal, temp }

        VectorCrossProduct{ out[0], planeNormal,  planeNormal }


        sub.xyz  planeVec, vf00, tempP1
        VectorDotProduct{ dot, planeVec, planeNormal }
        move.xyz plane, planeNormal
        mr32.w plane, dot



        iaddi planes, planes, -1
        iadd    vertexCounter, iBase, clippedVertices
        iadd clippedVertices, vi00, vi00
        b vertexLoop
top_plane:
        iaddiu  ret, vi00, 1

        ibne planes, ret, finish_plane

          mulz.xyz tempP1, out[1], camProps

        add.xyz tempP1, nearP1, tempP1

        sub.xyz planeNormal, tempP1, camPos

        Normalize{ planeNormal, planeNormal, temp }

        VectorCrossProduct{  planeNormal, out[0], planeNormal }


        sub.xyz  planeVec, vf00, tempP1
        VectorDotProduct{ dot, planeVec, planeNormal }
        move.xyz plane, planeNormal
        mr32.w plane, dot


        iaddi planes, planes, -1
        iadd    vertexCounter, iBase, clippedVertices
        iadd clippedVertices, vi00, vi00
        b vertexLoop


finish_plane:
        b end
write_verts:
        iaddiu  vertexData,     iBase,      1
        iadd    clippedBuffer,  vertexData,  clippedVertices
        iadd    outPtr,         vertexData,  vertCount
        ibeq    useSTQ,            vi00,           checkIfColorsClip
        iadd  clippedBuffer,     clippedBuffer,     clippedVertices
        iadd    outPtr,         outPtr,  vertCount
checkIfColorsClip:
        ibeq    useColor,            vi00,           continueSetupClip
        iadd  clippedBuffer,     clippedBuffer,  clippedVertices
        iadd  outPtr,            outPtr,         vertCount
continueSetupClip:
        iadd copyBack, vi00, clippedBuffer
        iadd clippedBuffer, vi00, outPtr
        iadd vertexCounter, clippedVertices, iBase
        ibeq vertexCounter, iBase, end
        iadd vertCount, vi00, clippedVertices


        write_back:

            iadd outData, vi00, vertexData
            lq vertex1, 0(clippedBuffer)
            lq vertex2, 1(clippedBuffer)
            lq vertex3, 2(clippedBuffer)

            sq vertex1, 0(outData)
            sq vertex2, 1(outData)
            sq vertex3, 2(outData)

            iaddiu clippedBuffer, clippedBuffer, 3
            ibeq    useSTQ,     vi00,        storeColors
            iadd   outData,     outData,     clippedVertices
            lq vertex1, 0(clippedBuffer)
            lq vertex2, 1(clippedBuffer)
            lq vertex3, 2(clippedBuffer)

            sq vertex1, 0(outData)
            sq vertex2, 1(outData)
            sq vertex3, 2(outData)
            iaddiu clippedBuffer, clippedBuffer, 3
storeColors:
            ibeq    useColor,            vi00,           doneVerts
            iadd  outData,     outData,     clippedVertices
            lq vertex1, 0(clippedBuffer)
            lq vertex2, 1(clippedBuffer)
            lq vertex3, 2(clippedBuffer)

            sq vertex1, 0(outData)
            sq vertex2, 1(outData)
            sq vertex3, 2(outData)
            iaddiu clippedBuffer, clippedBuffer, 3
doneVerts:
            iaddiu vertexData, vertexData, 3
            iaddi  vertexCounter, vertexCounter, -3

            ibne  vertexCounter, iBase, write_back
        iaddiu  vertexData,     iBase,      1
        iadd clippedBuffer, vi00, copyBack
        jr ret
end:
        isw.w   clippedVertices, 0(iBase)
         .vsm
           NOP             ilw.x   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm

#endvuprog