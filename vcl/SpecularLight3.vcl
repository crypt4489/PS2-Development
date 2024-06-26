#include "my_vcl.inc"

#vuprog VU1_SpecularLightStage3

START:

    MatrixLoad{ globalMatrix, 4, vi00 }

    lq.xyz   camPos, 15(vi00)

    ilw.x lightCount,       20(vi00)
    iaddiu lightPointer, vi00, 21


begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,       end

    iadd    normData,       vertexData, vertCount
    iadd    normData,       normData, vertCount


    iadd vertexCounter, iBase, vertCount

    iadd lightLoop, lightCount, vi00

    vertexLoop:

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

        b compare_light_loop

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

compare_light_loop:
        ibne lightLoop, vi00, light_loop

write_color:
        loi 128
        addi.w outColor, vf00, I
        loi 255
        mini.xyz  outColor, outColor, I
        max.xyz   outColor, outColor, vf00[x]
        ftoi0     outColor, outColor


        sq outColor,    0(normData)



        iaddiu          vertexData,     vertexData,     1
        iaddiu          normData,       normData,       1
        iadd            lightLoop,      lightCount,     vi00
        iaddiu          lightPointer,   vi00,           21

        iaddi   vertexCounter,  vertexCounter,  -1
        ibne    vertexCounter,  iBase,   vertexLoop
end:
        .vsm
           NOP             iadd     clipProg,   vi00, vi00
           NOP             NOP
        .endvsm

#endvuprog