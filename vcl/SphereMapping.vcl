#include "my_vcl.inc"
#vuprog VU1_SphereMapStage2
.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter


    MatrixLoad{ globalMatrix, 4, vi00 }

    lq.xyz    camPos,            15(vi00)

begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1           ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)              ; load vert count from scale vector
    ibeq    vertCount,      vi00,       end
    iadd    stqData,       vertexData,  vertCount   ; pointer to stq
    iadd    normData,      stqData,     vertCount   ; pointer to stq


    iadd vertexCounter, iBase, vertCount ; loop vertCount times
    vertexLoop:

        add.xyz stq1, vf00, vf00


        lq vertex1, 0(vertexData)


        lq norm1, 0(normData)


        MatrixMultiplyVertex{ vertex1, globalMatrix, vertex1 }

        sub.xyz vertex1, vertex1, camPos

        Reflect{ reflect, norm1, vertex1 }

        addw.z  reflect, reflect, vf00

        mul.xyz rsq, reflect, reflect

        addy.x rsq, rsq, rsq

        addz.x rsq, rsq, rsq

        sqrt  q, rsq[x]

        loi 2.0

        addq.x  rsq, vf00, q

        muli.x rsq, rsq, I

        div  q, vf00[w], rsq[x]

        loi 0.5

        addq.x  rsq,  vf00,  q

        mulx.x stq1, reflect, rsq

        add.x stq1, stq1, I

        mulx.y stq1, reflect, rsq

        add.y stq1, stq1, I

        addw.z stq1, vf00, vf00

store_stq:


        sq.xyz    stq1,         0(stqData)


        iaddiu          stqData,        stqData,     1
        iaddiu          normData,       normData,    1
        iaddiu          vertexData,     vertexData,  1

        iaddi   vertexCounter,  vertexCounter,  -1
        ibne    vertexCounter,  iBase,   vertexLoop
end:
        .vsm
           NOP             ilw.z   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm


    --exit
    --endexit

#endvuprog