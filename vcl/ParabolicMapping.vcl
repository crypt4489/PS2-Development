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

        lq vertex1, 0(vertexData)

        lq norm1, 0(normData)

        MatrixMultiplyVertex{ vertex1, globalMatrix, vertex1 }

        sub.xyz incident, vertex1, camPos

        Normalize{ incident, incident, temp1 }

        Matrix3MultiplyVertex3{ norm1, globalMatrix, norm1 }

        Normalize{ norm1, norm1, temp2 }

        Reflect{ reflect, norm1, incident }

        Normalize{ reflect, reflect, temp2 }

        iaddiu  comp, vi00, 0x0020

        fmand res, comp

        ibne res, comp, positive

        subz.z reflect, vf00, reflect
positive:
        addw.z  reflect, reflect, vf00

        loi 2.0

        muli.z rsq, reflect, I

        div  q, vf00[w], rsq[z]

        loi 0.5

        mulq.xy stq1, reflect, q

        add.xy stq1, stq1, I

store_stq:


        sq.xy    stq1,         0(stqData)


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