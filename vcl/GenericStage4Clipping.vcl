#include "my_vcl.inc"

#vuprog VU1_FullClipStage4

START:

    MatrixLoad{ viewProj, 0, vi00 }
    MatrixLoad{ world, 4, vi00 }

 

    ilw.w   renderFlags,    8(vi00)

   

    iaddiu  useSTQ,            vi00,         0x0040
    iand    useSTQ,           renderFlags,  useSTQ

    iaddiu  useColor,            vi00,         0x0080
    iand    useColor,            renderFlags,  useColor

begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,        1
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,          end
    iadd    destAddress,    vertexData,   vertCount
    ibeq    useSTQ,          vi00,         colorData
    iadd    destAddress,    destAddress,  vertCount

colorData:
    ibeq    useColor,          vi00,         createColorOut
    iadd    destAddress,    destAddress,  vertCount
    b       checkOutBonesAndWeights
createColorOut:
    lq     outColor, 11(vi00)
checkOutBonesAndWeights:
    iaddiu  useBW,            vi00,         0x0100
    iand    useBW,           renderFlags,    useBW
    ibeq    useBW,            vi00,         startLoop
    iadd    destAddress,    destAddress,  vertCount
    iadd    destAddress,    destAddress,  vertCount
    

startLoop:
    iadd    outAddress, vi00,  destAddress

    iadd vertexCounter, iBase, vertCount

    MatrixMultiply{ WVP, world, viewProj }

    iaddi clipStart, vi00, -1
    iaddi passedStart, vi00, -1
    iaddi lastPassed, vi00, -1
    iaddi lastClipped, vi00, -1

    iadd clipCount, vi00, vi00
    
    iadd  inPtr, vi00, vertexData

    clipTestLoop:

        lq vertex1, 0(inPtr)
        lq vertex2, 1(inPtr)
        lq vertex3, 2(inPtr)


        MatrixMultiplyVertex{ vertex1, WVP, vertex1 }
        MatrixMultiplyVertex{ vertex2, WVP, vertex2 }
        MatrixMultiplyVertex{ vertex3, WVP, vertex3 }
        
        fcset       0x000000
        
        clipw.xyz	vertex1, vertex1
        clipw.xyz   vertex2, vertex2
        clipw.xyz   vertex3, vertex3
        ; check if inside clip space
        fceq		VI01,   0
        ibne        VI01, VI00, InClipSpace

        ; near plane
        fceq        VI01, 0x20820
        ibne        VI01, VI00, OutsideClipSpace

        ; far plane
        fceq        VI01, 0x10410
        ibne        VI01, VI00, OutsideClipSpace

        ; right plane
        fceq        VI01, 0x01041
        ibne        VI01, VI00, OutsideClipSpace

        ; left plane
        fceq        VI01, 0x02082
        ibne        VI01, VI00, OutsideClipSpace

        ; top plane
        fceq        VI01, 0x04104
        ibne        VI01, VI00, OutsideClipSpace

        ; bottom plane
        fceq        VI01, 0x08208
        ibne        VI01, VI00, OutsideClipSpace
    ; if none of those work, then we intersect
    IntersectClipSpace:
        iaddiu, clipCount, clipCount, 3
        sq vertex1, 0(destAddress)
        sq vertex2, 1(destAddress)
        sq vertex3, 2(destAddress)
        iaddiu destAddress, destAddress, 3
        iadd    nextPtr, inPtr, vertCount
        ibeq    useSTQ, vi00, IntersectingColor
        
        lq.xyz  tex1, 0(nextPtr)
        lq.xyz  tex2, 1(nextPtr)
        lq.xyz  tex3, 2(nextPtr)  
        iadd    nextPtr, inPtr, vertCount
        sq.xyz  tex1, 0(destAddress)  
        sq.xyz  tex2, 1(destAddress)  
        sq.xyz  tex3, 2(destAddress)   
        iaddiu destAddress, destAddress, 3

    IntersectingColor:
        ibeq    useColor, vi00, LastClippedDet
        lq.xyz  color1, 0(nextPtr)
        lq.xyz  color2, 1(nextPtr)
        lq.xyz  color3, 2(nextPtr)  
        sq.xyz  color1, 0(destAddress)  
        sq.xyz  color2, 1(destAddress)  
        sq.xyz  color3, 2(destAddress)   
        iaddiu destAddress, destAddress, 3

    LastClippedDet:
        ibgez  lastClipped, SetLastClipped
        iadd   lastClipped, inPtr, vi00
        iadd   clippedStart, inPtr, vi00
        b      SetClipCount
    SetLastClipped
        isub   temp, inPtr, lastClipped
        isw.x  temp, 0(lastClipped)
        iadd   lastClipped, vi00, temp

    SetClipCount:
        iaddiu temp, vi00, 3
        isw.y  temp, 0(inPtr)
        isw.x  VI00, 0(inPtr)
        b      NextVertCheck

    OutsideClipSpace:
        ibgez  lastPassed, SetLastPassed
        iadd   lastPassed, inPtr, vi00
        iadd   passedStart, inPtr, vi00
        b      NextVertCheck
    SetLastPassed:
        isub   temp, inPtr, lastPassed
        isw.x  temp, 0(lastPassed)
        iadd   lastPassed, vi00, temp
        b      NextVertCheck
    InClipSpace:
        sq vertex1, 0(inPtr)
        sq vertex2, 1(inPtr)
        sq vertex3, 2(inPtr)
   NextVertCheck: 
        iadd inPtr, inPtr, 3

        iaddi   vertexCounter,  vertexCounter,  -3
        ibne    vertexCounter,  iBase,   clipTestLoop



ibeq clipCount, vi00, PerspectiveDivide



PerspectiveDivide:
lq.xyz  scale,          8(vi00)
lq.xyz  camScale,       9(vi00)
lq.yzw  primTag,        10(vi00)
sq.yzw  primTag,    0(outAddress)
iadd totalCount, vi00, vi00
iadd vertexCounter, vi00, vertCount
iaddiu outAddress, outAddress, 1
iaddi kickAddress, outAddress, VI00
iaddiu writeAddr, VI00, WriteOutVertex
PerspectiveLoop:

        iadd  inPtr, vi00, vertexData

        ibne  inPtr, passedStart, CheckClipStart

        ilw.x  skip, 0(inPtr)

        iadd passedStart, skip, passedStart

CheckClipStart:
        ibne  inPtr, clippedStart, InClipSpaceVert

        ilw.x skip, 0(inPtr)

        iadd clipStart, skip, clipStart

        iaddiu vertexData, vertexData, 2 ; skip two extra, because we did it by triangle in beginning

        ilw.y loop,  0(inPtr)

        ibeq  loop, vi00, NextPersVert

        iaddiu stride, vi00, 1

        iadd inPtr, destAddress, vi00

        ClippedWriteLoop:

            jalr  ret, WriteOutVertex

            iaddiu totalCount, totalCount, 1

            iaddi loop, loop, -1

            ibne  loop, vi00, ClippedWriteLoop

        iadd destAddress, inPtr, vi00

        b NextPersVert
InClipSpaceVert:

        iadd stride, vi00, vertCount

        jalr ret, writeAddr


NextPersVert:
        iaddiu vertexData, vertexData, 1
        iaddi  vertexCounter, vertexCounter, -1
        ibne   vertexCounter,  vi00,   PerspectiveLoop

    ilw.x   countTag,       10(vi00)

    ior     countTag,       countTag,     vertCount

    isw.x   countTag,   0(kickAddress)

    xgkick kickAddress

    b end

WriteOutVertex:

        lq          vertex, 0(inPtr)
        div         q,      vf00[w],    vertex[w]
        iadd        inPtr,  stride,    inPtr
        mul.xyz     vertex, vertex,     q
        mula.xyz    acc,    scale,      vf00[w]
        madd.xyz    vertex, vertex,     camScale
        ftoi4.xyz   vertex, vertex
        

        ibeq    useSTQ,            vi00,         loadColor
        lq      stq,            0(inPtr)
        mulq    modStq,         stq,          q
        sq      modStq,         0(outAddress)
        iaddiu  outAddress,    outAddress,    1
        iadd    inPtr,  inPtr, stride

loadColor:
        ibeq    useColor,       vi00,         WriteDest
        lq      outColor,       0(inPtr)

WriteDest:
        sq outColor,    0(outAddress)
        sq.xyz vertex,  1(outAddress)
        isw.w		VI00,   1(outAddress)
        iaddiu  outAddress, outAddress, 2
        jr ret

    --barrier

end:

    --cont

    b begin

#endvuprog