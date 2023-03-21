#include "my_vcl.inc"
#vuprog VU1_EnvMapStage2
.init_vf_all
.init_vi_all
.syntax new

.vu

--enter
--endenter

 
    MatrixLoad{ globalMatrix, 4, vi00 }
    MatrixLoad{ EnvMapMatrix, 16, vi00 }




begin:
    xtop    iBase

    iaddiu  vertexData,     iBase,      1           ; pointer to vertex data
    ilw.w   vertCount,      0(iBase)              ; load vert count from scale vector
    iadd    stqData,       vertexData, vertCount   ; pointer to stq
    iadd    normData,    stqData,    vertCount   ; pointer for XGKICK
  

    iadd vertexCounter, iBase, vertCount ; loop vertCount times
    vertexLoop:

        add.xyz stq1, vf00, vf00
        add.xyz stq2, vf00, vf00
        add.xyz stq3, vf00, vf00

        lq vertex1, 0(vertexData)
        lq vertex2, 1(vertexData)
        lq vertex3, 2(vertexData)

        sub.xyz        tw_vert12, vertex2, vertex1
        sub.xyz        tw_vert13, vertex3, vertex1
        opmula.xyz     ACC,       tw_vert12, tw_vert13
        opmsub.xyz     tw_normal, tw_vert13, tw_vert12

        add.x            forward,  vf00,   EnvMapMatrix[0][z]
        add.y            forward,  vf00,   EnvMapMatrix[1][z]
        add.z            forward,  vf00,   EnvMapMatrix[2][z]

        VectorDotProduct{ dot, tw_normal, forward }

        fsand res, 0x02

        ibeq res, vi00, store_stq 

        lq normal, 0(normData)

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }
       
        add.xy          acc, EnvMapMatrix[3], vf00
        madd.xy         acc, EnvMapMatrix[0], normal[x]
        madd.xy         acc, EnvMapMatrix[1], normal[y]
        madd.xy         stq1, EnvMapMatrix[2], normal[z]
        
        lq normal, 1(normData)

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }
       
        add.xy          acc, EnvMapMatrix[3], vf00
        madd.xy         acc, EnvMapMatrix[0], normal[x]
        madd.xy         acc, EnvMapMatrix[1], normal[y]
        madd.xy         stq2, EnvMapMatrix[2], normal[z]


        lq normal, 2(normData)

        Matrix3MultiplyVertex3{ normal, globalMatrix, normal }
       
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

        .vsm
           NOP             ilw.y   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm


    --exit
    --endexit

#endvuprog