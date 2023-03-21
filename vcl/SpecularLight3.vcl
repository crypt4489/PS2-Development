#include "my_vcl.inc"


.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter


#vuprog VU1_SpecularLightStage3

START:
 ;//////////// --- Load data 1 --- /////////////
    ; Updated once per mesh
  
    MatrixLoad{ globalMatrix, 4, vi00 }

    lq.xyz   camPos, 15(vi00)

    ;/////////////////////////////////////////////


    ilw.x lightCount,       20(vi00)
    iaddiu lightPointer, vi00, 21


begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1          ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)
    
    iadd    normData,       vertexData, vertCount   ; pointer to stq
    iadd    normData,       normData, vertCount   ; pointer to stq





    iadd vertexCounter, iBase, vertCount ; loop vertCount times

    iadd lightLoop, lightCount, vi00

    vertexLoop:

        ;////////// --- Load loop data --- //////////
        lq vertex, 0(vertexData)    
    
        MatrixMultiplyVertex{ vertex, globalMatrix, vertex }
        
        lq.xyz normal, 0(normData)

        add.xyz outColor, vf00, vf00

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }

        Normalize{ normal, normal, temp }

        ibeq lightLoop, vi00, write_color

light_loop:       

        iaddi lightLoop, lightLoop, -1

        ilw.w lightType, 0(lightPointer)

        iaddi lightCheck, lightType, -1

        ibeq lightCheck, vi00, directional_light

        iaddi lightCheck, lightType, -2

        ibeq lightCheck, vi00, point_light

        iaddi lightCheck, lightType, -3

        ibeq lightCheck, vi00, spotlight

        iaddiu lightPointer, lightPointer, 1 ;skip ambient
compare_light_loop:
        ibne lightLoop, vi00, light_loop
        
write_color:
        loi 128
        addi.w outColor, vf00, I
        loi 255
        mini.xyz  outColor, outColor, I
        max.xyz   outColor, outColor, vf00[x]  

       
        sq outColor,    0(normData)      
      
        

        iaddiu          vertexData,     vertexData,     1
        iaddiu          normData,       normData,       1
        iadd            lightLoop,      lightCount,     vi00
        iaddiu          lightPointer,   vi00,           21

        iaddi   vertexCounter,  vertexCounter,  -1	; decrement the loop counter
        ibne    vertexCounter,  iBase,   vertexLoop	; and repeat if needed

    ;////////////////////////////////////////////
        .vsm
           NOP             ilw.z   clipProg,       0(iBase)
           NOP             NOP
        .endvsm
    --barrier

  

directional_light:

        lq.xyz lightDir, 0(lightPointer)
        sub.xyz lightDir, vf00, lightDir
        sub.xyz toCam, camPos, vertex
        Normalize{ toCam, toCam, temp }
        add.xyz half, lightDir, toCam
        Normalize{ half, half, temp }
        VectorDotProduct{ spec, normal, half }
        
        mul.x  spec, spec, spec
        mul.x  spec, spec, spec
        mul.x  spec, spec, spec
        mul.x  spec, spec, spec
        mul.x  spec, spec, spec
        mini.x spec, spec, vf00[w]
        max.x spec, spec, vf00[x]
        lq.xyz lightColor, 1(lightPointer)
        mul.xyz tempColor, lightColor, spec[x]
        add.xyz outColor, outColor, tempColor
        iaddiu lightPointer, lightPointer, 2
        b compare_light_loop



spotlight:
        iaddiu lightPointer, lightPointer, 3
        b compare_light_loop

 point_light:
        iaddiu lightPointer, lightPointer, 2
        b compare_light_loop





    --exit
    --endexit

#endvuprog