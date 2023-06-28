#include "my_vcl.inc"
#vuprog VU1_SphereMappingStage2
.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter

    MatrixLoad{ globalMatrix, 4, vi00 }

    MatrixLoad{ globalViewMatrix, 16, vi00 }

    MatrixInverse{ normalMatrix, globalViewMatrix }

    sub.z normalMatrix[0], vf00, normalMatrix[0][z]
    sub.z normalMatrix[1], vf00, normalMatrix[1][z]
    sub.z normalMatrix[2], vf00, normalMatrix[2][z]
begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    iadd    stqData,       vertexData,  vertCount
    iadd    normData,      stqData,     vertCount


    iadd vertexCounter, iBase, vertCount ; loop vertCount times
    vertexLoop:

        add.xyz stq1, vf00, vf00
        add.xyz stq2, vf00, vf00
        add.xyz stq3, vf00, vf00

        lq vertex1, 0(vertexData)
        lq vertex2, 1(vertexData)
        lq vertex3, 2(vertexData)

        lq norm1, 0(normData)
        lq norm2, 1(normData)
        lq norm3, 2(normData)

        MatrixMultiplyVertex{ gvertex1, globalMatrix, vertex1 }
        MatrixMultiplyVertex{ gvertex2, globalMatrix, vertex2 }
        MatrixMultiplyVertex{ gvertex3, globalMatrix, vertex3 }

        sub.xyz        tw_vert12, gvertex2, gvertex1
        sub.xyz        tw_vert13, gvertex3, gvertex1
        opmula.xyz     ACC,       tw_vert12, tw_vert13
        opmsub.xyz     tw_normal, tw_vert13, tw_vert12

        sub.x            forward,  vf00,   globalViewMatrix[0][z]
        sub.y            forward,  vf00,   globalViewMatrix[1][z]
        sub.z            forward,  vf00,   globalViewMatrix[2][z]

        VectorDotProduct{ dot, tw_normal, forward }

        fsand res, 0x02

        ibeq res, vi00, store_stq

        MatrixMultiplyVertex{ incident, globalViewMatrix, vertex1 }

        Normalize{ incident, incident, temp2 }

        Matrix3MultiplyVertex3{ norm1, normalMatrix, norm1 }

        Normalize{ norm1, norm1, temp2 }

        Reflect{ reflect, norm1, incident }

        addw.z  reflect, reflect, vf00

        mul.xyz rsq, reflect, reflect

        addy.x rsq, rsq, rsq

        addz.x rsq, rsq, rsq

        sqrt  q, rsq[x]

        addq.x  rsq, vf00, q

        add.x   rsq, rsq, rsq

        div  q, vf00[w], rsq[x]

        loi 0.5

        mulq.xy stq1, reflect, q

        add.xy stq1, stq1, I

          MatrixMultiplyVertex{ incident, globalViewMatrix, vertex2 }

        Normalize{ incident, incident, temp2 }

        Matrix3MultiplyVertex3{ norm2, normalMatrix, norm2 }

        Normalize{ norm2, norm2, temp2 }

        Reflect{ reflect, norm2, incident }

        addw.z  reflect, reflect, vf00

        mul.xyz rsq, reflect, reflect

        addy.x rsq, rsq, rsq

        addz.x rsq, rsq, rsq

        sqrt  q, rsq[x]

        addq.x  rsq, vf00, q

        add.x   rsq, rsq, rsq

        div  q, vf00[w], rsq[x]

        loi 0.5

        mulq.xy stq2, reflect, q

        add.xy stq2, stq2, I

        MatrixMultiplyVertex{ incident, globalViewMatrix, vertex3 }

        Normalize{ incident, incident, temp2 }

        Matrix3MultiplyVertex3{ norm3, normalMatrix, norm3 }

        Normalize{ norm3, norm3, temp2 }

        Reflect{ reflect, norm3, incident }

        addw.z  reflect, reflect, vf00

        mul.xyz rsq, reflect, reflect

        addy.x rsq, rsq, rsq

        addz.x rsq, rsq, rsq



        sqrt  q, rsq[x]

        addq.x  rsq, vf00, q



        add.x   rsq, rsq, rsq

        div  q, vf00[w], rsq[x]

        loi 0.5

        mulq.xy stq3, reflect, q

        add.xy stq3, stq3, I
store_stq:


        sq.xy   stq1,  0(stqData)
        sq.xy   stq2,  1(stqData)
        sq.xy   stq3,  2(stqData)


        iaddiu          stqData,        stqData,     3
        iaddiu          normData,       normData,    3
        iaddiu          vertexData,     vertexData,  3

        iaddi   vertexCounter,  vertexCounter,  -3
        ibne    vertexCounter,  iBase,   vertexLoop
end:
        .vsm
           NOP             ilw.z   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm


    --exit
    --endexit

#endvuprog