

#include "my_vcl.inc"

#vuprog VU1_GenericMorphTargetStage13D

START:

    lq.xy      interpolate,     11(vi00)

    xtop    iBase

    iaddiu  vertexData,     iBase,      1
    ilw.w   vertCount,      0(iBase)
    iadd    stqData,        vertexData, vertCount
    iadd    colorData,    stqData,    vertCount
    iadd    morphVData,    colorData,  vertCount
    iadd    morphSTQData,  morphVData, vertCount
    iadd    morphColorData, morphSTQData, vertCount

    iadd vertexCounter, iBase, vertCount
    vertexLoop:


        lq vertex, 0(vertexData)
        lq stq,    0(stqData)
        lq color,  0(colorData)

        lq vertexM, 0(morphVData)
        lq stqM,  0(morphSTQData)
        lq colorM, 0(morphColorData)



        Lerp3CompXY{ vertex, vertexM, interpolate }

        Lerp2CompXY{ stq, stqM, interpolate }

        Lerp3CompXY{ color, colorM, interpolate }


        sq vertex, 0(vertexData)
        sq stq,    0(stqData)
        sq color, 0(colorData)


        iaddiu          vertexData,     vertexData,     1
        iaddiu          stqData,        stqData,        1
        iaddiu          colorData,      colorData,      1
        iaddiu          morphVData,     morphVData,     1
        iaddiu          morphSTQData,   morphSTQData,   1
        iaddiu          morphColorData, morphColorData, 1

        iaddi   vertexCounter,  vertexCounter,  -1
        ibne    vertexCounter,  iBase,   vertexLoop

        .vsm
           NOP             ilw.y   jmpProg,       0(iBase)
           NOP             NOP ; jr jmpProg
        .endvsm


#endvuprog
