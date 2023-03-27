#include "my_vcl.inc"
#vuprog VU1_AnimTexStage2
.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter


START:

    MatrixLoad{ transformMatrix, 16, vi00 }




begin:
    xtop    iBase


    iaddiu  vertexData,     iBase,      1           ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)              ; load vert count from scale vector
    ibeq    vertCount,      vi00,       end
    iadd    stqData,        vertexData, vertCount   ; pointer to stq



    iadd vertexCounter, iBase, vertCount ; loop vertCount times
    vertexLoop:


        lq stq,    0(stqData)

        MatrixMultiplyVertex{ stq, transformMatrix, stq }


        ;//////////// --- Store data --- ////////////
        sq stq,      0(stqData)      ; STQ



        iaddiu          stqData,        stqData,        1


        iaddi   vertexCounter,  vertexCounter,  -1	; decrement the loop counter
        ibne    vertexCounter,  iBase,   vertexLoop	; and repeat if needed

end:
        .vsm
           NOP             ilw.z   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm


    --exit
    --endexit

#endvuprog
