#include "my_vcl.inc"

#vuprog VU1_FullClipStage4


; Persistent Integer Registers Mapping

; VI03 - useSTQ
; VI04 - useColor
; VI05 - vertexData
; VI06 - vertexCount
; VI07 - destAddress
; VI08 - outAddress
; VI09 - clipStart
; VI10 - passedStart


; Non-Persistent Mapping For Sections

; Setup 

; VI01 - renderFlags
; VI02 - iBase/useBW

; Trivial Clip Check

; VI01 - ClipTest/temp/nextPtr
; VI02 - lastPassed
; VI11 - lastClipped
; VI12 - clipCount
; VI13 - inPtr
; VI14 - vertexCounter


; Perspective Divide
; VI01 - skip/loop
; VI02 - stride
; VI11 - totalCount
; VI12 - vertexCounter
; VI13 - kickAddress 
; VI14 - inPtr
; VI15 - ret

START:

    MatrixLoad{ viewProj, 0, vi00 }
    MatrixLoad{ world, 4, vi00 }

 

    ilw.w   VI01,    8(vi00)

    

    iaddiu  VI03,            vi00,         0x0040
    iand    VI03,            VI01,         VI03

    iaddiu  VI04,            vi00,         0x0080
    iand    VI04,            VI01,         VI04

begin:
    xtop    VI02

    iaddiu  VI05,      VI02,          1
    ilw.w   VI06,      0(VI02)
    ibeq    VI06,      VI00,         end
    iadd    VI07,      VI05,         VI06
    ibeq    VI03,      vi00,         colorData
    iadd    VI07,      VI07,         VI06

colorData:
    ibeq    VI04,       vi00,       checkOutBonesAndWeights
    iadd    VI07,       VI07,       VI06
checkOutBonesAndWeights:
    iaddiu  VI02,       VI00,       0x0100
    iand    VI02,       VI01,       VI02
    ibeq    VI02,       VI00,       startLoop
    iadd    VI07,       VI07,       VI06
    iadd    VI07,       VI07,       VI06
    

startLoop:
    iadd   VI08,       VI00,       VI07

    iadd   VI14,        VI00,       VI06

    MatrixMultiply{ WVP, world, viewProj }

    iaddi VI09, vi00, -1
    iaddi VI10, vi00, -1
    iaddi VI02, vi00, -1
    iaddi VI11, vi00, -1

    iadd  VI12, vi00, vi00
    
    iadd  VI13, vi00, VI05

    clipTestLoop:

        lq vertex1, 0(VI13)
        lq vertex2, 1(VI13)
        lq vertex3, 2(VI13)


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
        iaddiu, VI12, VI12, 3
        sq vertex1, 0(VI07)
        sq vertex2, 1(VI07)
        sq vertex3, 2(VI07)
        iaddiu  VI07, VI07, 3
        iadd    VI01, VI13, VI06
        ibeq    VI03, VI00, IntersectingColor
        
        lq.xyz  tex1, 0(VI01)
        lq.xyz  tex2, 1(VI01)
        lq.xyz  tex3, 2(VI01)  
        iadd    VI01, VI01, VI05
        sq.xyz  tex1, 0(VI07)  
        sq.xyz  tex2, 1(VI07)  
        sq.xyz  tex3, 2(VI07)   
        iaddiu  VI07, VI07, 3

    IntersectingColor:
        ibeq    VI04,   VI00, LastClippedDet
        lq.xyz  color1, 0(VI01)
        lq.xyz  color2, 1(VI01)
        lq.xyz  color3, 2(VI01)  
        sq.xyz  color1, 0(VI07)  
        sq.xyz  color2, 1(VI07)  
        sq.xyz  color3, 2(VI07)   
        iaddiu  VI07, VI07, 3

    LastClippedDet:
        ibgez  VI11, SetLastClipped
        iadd   VI11, VI13, vi00
        iadd   VI09, VI13, vi00
        b      SetClipCount
    SetLastClipped:
        isub   VI01, VI13, VI11
        isw.x  VI01, 0(VI11)
        iadd   VI11, vi00, VI13

    SetClipCount:
        iaddiu VI01, vi00, 3
        isw.y  VI01, 0(VI13)
        isw.x  VI00, 0(VI13)
        b      NextVertCheck

    OutsideClipSpace:
        ibgez  VI02, SetLastPassed
        iadd   VI02, VI13, vi00
        iadd   VI10, VI13, vi00
        b      NextVertCheck
    SetLastPassed:
        isub   VI01, VI13, VI02
        isw.x  VI01, 0(VI02)
        iadd   VI02, vi00, VI13
        b      NextVertCheck
    InClipSpace:
        sq vertex1, 0(VI13)
        sq vertex2, 1(VI13)
        sq vertex3, 2(VI13)
   NextVertCheck: 
        iaddiu VI13, VI13, 3

        iaddi   VI14,  VI14,  -3
        ibne    VI14,  VI00,   clipTestLoop



ibeq VI12, vi00, PerspectiveDivide

PerspectiveDivide:
lq.xyz  scale,          8(vi00)
lq.xyz  camScale,       9(vi00)
lq.yzw  primTag,        10(vi00)
sq.yzw  primTag,    0(VI08)
lq     outColor, 11(vi00)
iadd VI11, vi00, vi00
iadd VI12, vi00, VI06
iaddiu VI08, VI08, 1
iaddiu VI13, VI08, 0
PerspectiveLoop:

        iadd  VI14, vi00, VI05

        ibne  VI14, VI10, CheckClipStart

        iaddiu VI05, VI05, 2 ; skip two extra verts

        ilw.x  VI01, 0(VI14)

        iadd VI10, VI01, VI10

        b NextPersVert

CheckClipStart:
        ibne  VI14, VI09, InClipSpaceVert

        ilw.x VI01, 0(VI14)

        iadd VI09, VI01, VI09

        iaddiu VI05, VI05, 2 ; skip two extra, because we did it by triangle in beginning

        ilw.y VI01,  0(VI14)

        ibeq  VI01, vi00, NextPersVert

        iaddiu VI02, vi00, 3

        

        ClippedWriteLoop:

            iaddiu VI14, VI07, 0

            bal VI15, WriteOutVertex

            iaddiu VI11, VI11, 1

            iaddi VI01, VI01, -1

            iaddiu VI07, VI07, 1

            ibne  VI01, vi00, ClippedWriteLoop

            iaddiu VI07, VI14, 1 ; swap the tmep pointer to clip buffer new pointer

        b NextPersVert
InClipSpaceVert:

        iadd VI02, vi00, VI06

        bal VI15, WriteOutVertex

         iaddiu VI11, VI11, 1


NextPersVert:
        iaddiu VI05, VI05, 1
        iaddi  VI12, VI12, -1
        ibne   VI12,  vi00,   PerspectiveLoop

    ilw.x   VI01,       10(vi00)

    ior     VI01,       VI01,     VI11

    isw.x   VI01,   0(VI13)

    xgkick VI13

    b end

WriteOutVertex:

        lq          vertex, 0(VI14)
        div         q,      vf00[w],    vertex[w]
        
        mul.xyz     vertex, vertex,     q
        mula.xyz    acc,    scale,      vf00[w]
        madd.xyz    vertex, vertex,     camScale
        ftoi4.xyz   vertex, vertex
        

        ibeq    VI03,            vi00,         loadColor
        iadd     VI14,  VI02,    VI14
        lq      stq,            0(VI14)
        mulq    modStq,         stq,          q
        sq      modStq,         0(VI08)
        iaddiu  VI08,    VI08,    1
        

loadColor:
        ibeq    VI04,           vi00,         WriteDest
        iadd    VI14,    VI14,    VI02
        lq      outColor,       0(VI14)


WriteDest:
        sq outColor,    0(VI08)
        sq.xyz vertex,  1(VI08)
        isw.w		VI00,   1(VI08)
        iaddiu  VI08, VI08, 2
        jr VI15

    --barrier

end:

    --cont

    b begin

#endvuprog