#include "my_vcl.inc"
#vuprog VU1_EnvMapStage2


start:
    MatrixLoad{ globalMatrix, 4, vi00 }
    MatrixLoad{ EnvMapMatrix, 16, vi00 }




begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    ibeq    vertCount,      vi00,       end
    iadd    stqData,       vertexData, vertCount
    iadd    normData,    stqData,    vertCount


    iadd vertexCounter, iBase, vertCount
    vertexLoop:

        add.xyz stq1, vf00, vf00
        add.xyz stq2, vf00, vf00
        add.xyz stq3, vf00, vf00

        lq normal, 0(normData)

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }

        Normalize{ normal, normal, temp }

        add.xy          acc, EnvMapMatrix[3], vf00
        madd.xy         acc, EnvMapMatrix[0], normal[x]
        madd.xy         acc, EnvMapMatrix[1], normal[y]
        madd.xy         stq1, EnvMapMatrix[2], normal[z]

        lq normal, 1(normData)
        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }

        Normalize{ normal, normal, temp }
        add.xy          acc, EnvMapMatrix[3], vf00
        madd.xy         acc, EnvMapMatrix[0], normal[x]
        madd.xy         acc, EnvMapMatrix[1], normal[y]
        madd.xy         stq2, EnvMapMatrix[2], normal[z]




        lq normal, 2(normData)

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }

        Normalize{ normal, normal, temp }
        add.xy          acc, EnvMapMatrix[3], vf00
        madd.xy         acc, EnvMapMatrix[0], normal[x]
        madd.xy         acc, EnvMapMatrix[1], normal[y]
        madd.xy         stq3, EnvMapMatrix[2], normal[z]



store_stq:


        sq.xy    stq1,         0(stqData)
        sq.xy    stq2,         1(stqData)
        sq.xy    stq3,         2(stqData)

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

#endvuprog