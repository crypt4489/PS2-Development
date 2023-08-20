#include "my_vcl.inc"
#vuprog VU1_AnimTexStage2

START:

    MatrixLoad{ transformMatrix, 16, vi00 }

begin:
    xtop    iBase


    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,       end
    iadd    stqData,        vertexData, vertCount

    iadd vertexCounter, iBase, vertCount

    vertexLoop:


        lq stq,    0(stqData)

        MatrixMultiplyVertex{ stq, transformMatrix, stq }

        sq stq,      0(stqData)      ; STQ

        iaddiu          stqData,        stqData,        1


        iaddi   vertexCounter,  vertexCounter,  -1
        ibne    vertexCounter,  iBase,   vertexLoop

end:
        .vsm
           NOP             ilw.z   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm

#endvuprog
