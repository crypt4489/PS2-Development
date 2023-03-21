#include "my_vcl.inc"
   
.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter


#vuprog VU1_ClipStage4

START:
 ;//////////// --- Load data 1 --- /////////////
    ; Updated once per mesh
    MatrixLoad{ viewProj, 0, vi00 } 

    ;/////////////////////////////////////////////

	fcset   0x000000	; VCL won't let us use CLIP without first zeroing
				; the clip flags

    ;//////////// --- Load data 2 --- /////////////
    ; Updated dynamically

    lq.xyz  scale,          8(vi00) ; load program params
                                     ; float : X, Y, Z - scale vector that we will use to scale the verts after projecting them.
                                     ; float : W - vert count.
    ilw.w   renderFlags,    12(vi00)

    iaddiu  useSTQ,            vi00,         0x0040
    iand    useSTQ,           renderFlags,  useSTQ

    iaddiu  useColor,            vi00,         0x0080
    iand    useColor,            renderFlags,  useColor

    add     outColor,       vf00,       vf00
begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,        1          ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,          end
    ilw.x   countTag,       10(vi00)
    ior     countTag,       countTag,     vertCount  ; pointer for XGKICK
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


    iadd vertexCounter, iBase, vertCount ; loop vertCount times


    
    vertexLoop:

        iadd  inPtr, vi00, vertexData
        ;////////// --- Load loop data --- //////////
        lq vertex, 0(inPtr)           


        MatrixMultiplyVertex{ vertex, viewProj, vertex }

        clipw.xyz	vertex, vertex			; Dr. Fortuna: This instruction checks if the vertex is outside
							; the viewing frustum. If it is, then the appropriate
							; clipping flags are set
        fcand		VI01,   0x3FFFF                 ; Bitwise AND the clipping flags with 0x3FFFF, this makes
							; sure that we get the clipping judgement for the last three
							; verts (i.e. that make up the triangle we are about to draw)
        iaddiu		iADC,   VI01,       0x7FFF      ; Add 0x7FFF. If any of the clipping flags were set this will
							; cause the triangle not to be drawn (any values above 0x8000
							; that are stored in the w component of XYZ2 will set the ADC
							; bit, which tells the GS not to perform a drawing kick on this
							; triangle.

        

        div         q,      vf00[w],    vertex[w]   ; perspective divide (1/vert[w]):
        mul.xyz     vertex, vertex,     q
        mula.xyz    acc,    scale,      vf00[w]     ; scale to GS screen space
        madd.xyz    vertex, vertex,     scale       ; multiply and add the scales -> vert = vert * scale + scale
        ftoi4.xyz   vertex, vertex                  ; convert vertex to 12:4 fixed point format
        ;////////////////////////////////////////////

 
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
        ftoi0   outColor,       outColor 

      
        
Data:
        sq outColor,    0(destAddress)      
        sq.xyz vertex,  1(destAddress)      
        isw.w		iADC,   1(destAddress)

        iaddiu          destAddress,    destAddress,    2
        iaddiu          vertexData,     vertexData,     1
        
        iaddi   vertexCounter,  vertexCounter,  -1	; decrement the loop counter
        ibne    vertexCounter,  iBase,   vertexLoop	; and repeat if needed

    ;////////////////////////////////////////////

    --barrier

    xgkick kickAddress ; dispatch to the GS rasterizer.
end:

    --cont

    b begin
    
    --exit
    --endexit

#endvuprog