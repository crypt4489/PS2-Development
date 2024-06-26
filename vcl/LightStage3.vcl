#include "my_vcl.inc"

#vuprog VU1_LightStage3

START:
    ; do normal calculations in world space
    MatrixLoad{ globalMatrix, 4, vi00 }

    ilw.x lightCount,       20(vi00)
    iaddiu lightPointer, vi00, 21


begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)


    iadd    normData,       vertexData, vertCount
    iadd    normData,       normData, vertCount





    iadd vertexCounter, iBase, vertCount

    ibeq    vertexCounter,      iBase,       end

    iadd lightLoop, lightCount, vi00

    sub.w  normal, vf00, vf00




    vertexLoop:


        lq vertex, 0(vertexData)


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

        b ambient_light
directional_light:

        lq.xyz lightDir, 0(lightPointer)

        sub.xyz lightDir, vf00, lightDir

        VectorDotProduct{ tempColor, lightDir, normal }

        mini.x tempColor, tempColor, vf00[w]
        max.x tempColor, tempColor, vf00[x]
        lq.xyz lightColor, 1(lightPointer)
        mul.xyz tempColor, lightColor, tempColor[x]
        add.xyz outColor, outColor, tempColor
        iaddiu lightPointer, lightPointer, 2
        b compare_light_loop

 ambient_light:
        lq.xyz ambient, 0(lightPointer)
        add.xyz outColor, outColor, ambient
        iaddiu lightPointer, lightPointer, 1
        b compare_light_loop


spotlight:
        MatrixMultiplyVertex{ worldPos, globalMatrix, vertex }
        lq.xyz lightPos, 0(lightPointer)
        lq lightColor, 1(lightPointer)
        lq lightDir, 2(lightPointer)


        sub.xyz  lightVec, worldPos, lightPos
        Normalize{ lightVec, lightVec, temp }

        VectorDotProduct{ spot, lightVec, lightDir }

        sub.w  vf00, lightDir, spot[x]

        iaddiu  res, vi00, 0x88

        fmand res, res

        ibne res, vi00, finish_spot


 color_spot:
        add.xyz   atten, vf00, vf00

         sub.xyz  lightDir, lightPos, worldPos

         eleng    p,        lightDir

         mfp.x    dist, p



         Normalize{ lightDir, lightDir, temp }

         VectorDotProduct{ tempColor, lightDir, normal }

         max.x tempColor, tempColor, vf00[x]



        div         q,      dist[x],    lightColor[w]

        add.x       fact,   vf00,  q

        add.x       fact, fact, vf00[w]

       div         q,      vf00[w],    fact[x]

       add.xyz  atten, vf00, q

        mul.w  lightDir, lightDir, lightDir

        sub.w denom,  vf00, lightDir[w]

        sub.w spot, vf00, spot[x]

        div  q,  spot[w],  denom[w]

        sub.w  spot, vf00,  q

        max.w spot, spot, vf00[x]

       add.xyz spot, vf00, spot[w]



       mul.xyz tempColor, lightColor, spot
       mul.xyz tempColor, tempColor, atten
       add.xyz outColor, outColor, tempColor
finish_spot:
        iaddiu lightPointer, lightPointer, 3
        b compare_light_loop

 point_light:
        MatrixMultiplyVertex{ worldPos, globalMatrix, vertex }
        lq.xyz lightPos, 0(lightPointer)
        lq lightColor, 1(lightPointer)

        sub.xyz  lightDir, lightPos, worldPos

         eleng    p,        lightDir

        mfp.x    dist, p

         Normalize{ lightDir, lightDir, temp }

        VectorDotProduct{ tempColor, lightDir, normal }

        max.x tempColor, tempColor, vf00[x]



        div         q,      dist[x],    lightColor[w]

        add.x       fact,   vf00,  q

        add.x       fact, fact, vf00[w]

       div         q,      vf00[w],    fact[x]

       add.xyz  atten, vf00, q

       mul.xyz tempColor, lightColor, tempColor[x]
       mul.xyz tempColor, tempColor, atten
       add.xyz outColor, outColor, tempColor

finish_pl:

        iaddiu lightPointer, lightPointer, 2
        b compare_light_loop

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
           NOP             iadd   clipProg,     vi00, vi00
           NOP             NOP
        .endvsm


#endvuprog