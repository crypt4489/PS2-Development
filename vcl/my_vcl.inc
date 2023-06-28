#macro VectorDotProduct: dotproduct, vector1, vector2
   mul.xyz        dotproduct, vector1,    vector2
   add.x          dotproduct, dotproduct, dotproduct[y]
   add.x          dotproduct, dotproduct, dotproduct[z]
#endmacro

#macro SignedDistance: dotproduct, vector, plane
   mul.xyz        dotproduct, vector,    plane
   add.x          dotproduct, dotproduct, dotproduct[y]
   add.x          dotproduct, dotproduct, dotproduct[z]
   addw.x         dotproduct, dotproduct, plane
#endmacro

#macro SignedDistanceof3Vectors: outproduct, vector1, vector2, vector3, plane
   mul.xyz        dotproduct, vector1,    plane
   add.x          dotproduct, dotproduct, dotproduct[y]
   add.x          dotproduct, dotproduct, dotproduct[z]
   addw.x         outproduct, dotproduct, plane
   mul.xyz        dotproduct, vector2,    plane
   add.y          dotproduct, dotproduct, dotproduct[x]
   add.y          dotproduct, dotproduct, dotproduct[z]
   addw.y         outproduct, dotproduct, plane
   mul.xyz        dotproduct, vector3,    plane
   add.z          dotproduct, dotproduct, dotproduct[x]
   add.z          dotproduct, dotproduct, dotproduct[y]
   addw.z         outproduct, dotproduct, plane

#endmacro

#macro Normalize: input, output, vclsmlftemp
   mul.xyz        vclsmlftemp, input,     input
   add.x          vclsmlftemp, vclsmlftemp, vclsmlftemp[y]
   add.x          vclsmlftemp, vclsmlftemp, vclsmlftemp[z]
   rsqrt          q,           vf00[w],     vclsmlftemp[x]
   mul.xyz        output,  input,     q
#endmacro

#macro Reflect: output, in_normal, incident
   mul.xyz        dotproduct, in_normal,  incident
   add.x          dotproduct, dotproduct, dotproduct[y]
   add.x          dotproduct, dotproduct, dotproduct[z]

   add.x          dotproduct, dotproduct, dotproduct
   mul.xyz        output, in_normal, dotproduct[x]
   sub.xyz        output, incident, output
#endmacro

#macro  VectorCrossProduct: vector1, vector2, crossproduct
    opmula.xyz  acc,  vector1, vector2
    opmsub.xyz  crossproduct, vector2, vector1
    sub.w   crossproduct, vf00, vf00
#endmacro

#macro MatrixMultiplyVertex: vertexresult, matrix, vertex
   mul            acc,           matrix[0], vertex[x]
   madd           acc,           matrix[1], vertex[y]
   madd           acc,           matrix[2], vertex[z]
   madd           vertexresult,  matrix[3], vertex[w]
#endmacro

#macro Matrix3MultiplyVertex3: vertexResult, matrix, vertex
   mul.xyz            acc,           matrix[0], vertex[x]
   madd.xyz           acc,           matrix[1], vertex[y]
   madd.xyz           vertexResult,  matrix[2], vertex[z]
#endmacro

#macro AngleSinCos: angle, sincos
   move.xz        sincos, angle

   mul.w          sincos, vf00, sincos[z]
   add.y          sincos, vf00, sincos[x]

   loi            1.570796
   sub.xz         sincos, sincos, I

   abs            sincos, sincos

   max            Vector1111, vf00, vf00[w]

   loi            -0.159155
   mul            ACC,  sincos, I

   loi            12582912.0
   msub           ACC,  Vector1111, I
   madd           ACC,  Vector1111, I
   loi            -0.159155
   msub           ACC,  sincos, I

   loi            0.5
   msub           sincos, Vector1111, I

   abs            sincos, sincos

   loi            0.25
   sub            sincos, sincos, I

   mul            anglepower2, sincos, sincos

   loi            -76.574959
   mul            k4angle, sincos, I

   loi            -41.341675
   mul            k2angle, sincos, I

   loi            81.602226
   mul            k3angle, sincos, I
   mul            anglepower4, anglepower2, anglepower2
   mul            k4angle, k4angle, anglepower2
   mul            ACC,  k2angle, anglepower2

   loi            39.710659
   mul            k2angle, sincos, I
   mul            anglepower8, anglepower4, anglepower4
   madd           ACC,  k4angle, anglepower4
   madd           ACC,  k3angle, anglepower4
   loi            6.283185
   madd           ACC,  sincos, I
   madd           sincos, k2angle, anglepower8
#endmacro

#macro MatrixLoad: matrix, offset, vumem
    lq matrix[0], offset+0(vumem)
    lq matrix[1], offset+1(vumem)
    lq matrix[2], offset+2(vumem)
    lq matrix[3], offset+3(vumem)
#endmacro

#macro TriangleWinding: result, vert1, vert2, vert3, eyepos
   sub.xyz        tw_vert12, vert2, vert1
   sub.xyz        tw_vert13, vert3, vert1
   opmula.xyz     ACC,       tw_vert12, tw_vert13
   opmsub.xyz     tw_normal, tw_vert13, tw_vert12

   sub.xyz        tw_dot, eyepos, vert1

   mul.xyz        tw_dot, tw_dot, tw_normal
   add.x          tw_dot, tw_dot, tw_dot[y]
   add.x          tw_dot, tw_dot, tw_dot[z]

   fsand          result,   0x3
#endmacro

#macro Lerp3CompXY: lerpStart, lerpEnd, factor

      mul.xyz  lerpEnd, lerpEnd, factor[y]

      mul.xyz  lerpStart, lerpStart, factor[x]

      add.xyz lerpStart, lerpStart, lerpEnd

#endmacro

#macro Lerp2CompXY: lerpStart, lerpEnd, factor

      mul.xy  lerpEnd, lerpEnd, factor[y]

      mul.xy  lerpStart, lerpStart, factor[x]

      add.xy lerpStart, lerpStart, lerpEnd

#endmacro

#macro Lerp2Comp: lerpStart, lerpEnd, factor

      sub.xy  lerpEnd, lerpEnd, lerpStart

      mul.xy  lerpEnd, lerpEnd, factor[x]

      add.xy lerpStart, lerpStart, lerpEnd

#endmacro

#macro Lerp3Comp: lerpStart, lerpEnd, factor

      sub.xyz  lerpEnd, lerpEnd, lerpStart

      mul.xyz  lerpEnd, lerpEnd, factor[x]

      add.xyz lerpStart, lerpStart, lerpEnd

#endmacro

#macro MatrixTranspose: matresult, matrixsrc
   mr32.y         vclsmlftemp,   matrixsrc[1]
   add.z          matresult[1], vf00, matrixsrc[2][y]
   move.y         matresult[2], vclsmlftemp
   mr32.y         vclsmlftemp,   matrixsrc[0]
   add.z          matresult[0], vf00, matrixsrc[2][x]
   mr32.z         vclsmlftemp,   matrixsrc[1]
   mul.w          matresult[1], vf00, matrixsrc[3][y]
   mr32.x         vclsmlftemp,   matrixsrc[0]
   add.y          matresult[0], vf00, matrixsrc[1][x]
   move.x         matresult[1], vclsmlftemp
   mul.w          vclsmlftemp,   vf00, matrixsrc[3][z]
   mr32.z         matresult[3], matrixsrc[2]
   move.w         matresult[2], vclsmlftemp
   mr32.w         vclsmlftemp,   matrixsrc[3]
   add.x          matresult[3], vf00, matrixsrc[0][w]
   move.w         matresult[0], vclsmlftemp
   mr32.y         matresult[3], vclsmlftemp
   add.x          matresult[2], vf00, vclsmlftemp[y]

   add.x         matresult[0], matrixsrc[0], vf00[x]
   add.y         matresult[1], matrixsrc[1], vf00[x]
   add.z         matresult[2], matrixsrc[2], vf00[x]
   add.w         matresult[3], matrixsrc[3], vf00[x]
#endmacro

#macro  MatrixInverse: matdest, matsrc
   opmula.xyz     acc, matsrc[2], matsrc[3]
   opmsub.xyz     c0,  matsrc[3], matsrc[2]
   mul.xyz        acc, matsrc[2], matsrc[3][w]
   msub.xyz       c1,  matsrc[3], matsrc[2][w]

   mul.xyz        t0, c0, matsrc[1]

   opmula.xyz     acc, matsrc[1], c1
   madd.xyz       acc, c0, matsrc[1][w]
   opmsub.xyz     matdest[0], c1, matsrc[1]

   mul.w          acc,         vf00, vf00[x]
   msub.w         acc,         vf00, t0[x]
   msub.w         acc,         vf00, t0[y]
   msub.w         matdest[0], vf00, t0[z]

   mul.xyz        t0, c0, matsrc[0]

   opmula.xyz     acc, c1, matsrc[0]
   msub.xyz       acc, c0, matsrc[0][w]
   opmsub.xyz     matdest[1], matsrc[0], c1

   mul            det, matsrc[0], matdest[0]

   mul.w          acc,         vf00, t0[x]
   madd.w         acc,         vf00, t0[y]
   madd.w         matdest[1], vf00, t0[z]

   add.w          acc, det,   det[x]
   madd.w         acc, vf00,  det[y]
   madd.w         det, vf00,  det[z]

   opmula.xyz     acc, matsrc[0], matsrc[1]
   opmsub.xyz     c0,  matsrc[1], matsrc[0]
   mul.xyz        acc, matsrc[0], matsrc[1][w]
   msub.xyz       c1,  matsrc[1], matsrc[0][w]

   div            Q, vf00[w], det[w]

   mul.xyz        t0, c0, matsrc[3]

   opmula.xyz     acc, matsrc[3], c1
   madd.xyz       acc, c0, matsrc[3][w]
   opmsub.xyz     matdest[2], c1, matsrc[3]

   mul.w          acc,         vf00, vf00[x]
   msub.w         acc,         vf00, t0[x]
   msub.w         acc,         vf00, t0[y]
   msub.w         matdest[2], vf00, t0[z]

   mul.xyz        t0, c0, matsrc[2]

   opmula.xyz     acc, c1, matsrc[2]
   msub.xyz       acc, c0, matsrc[2][w]
   opmsub.xyz     matdest[3], matsrc[2], c1

   mul.w          acc,         vf00, t0[x]
   madd.w         acc,         vf00, t0[y]
   madd.w         matdest[3], vf00, t0[z]

   mul            matdest[0], matdest[0], Q
   mul            matdest[1], matdest[1], Q
   mul            matdest[2], matdest[2], Q
   mul            matdest[3], matdest[3], Q
#endmacro

#macro QuaternionToMatrix: matresult, quaternion
   mula.xyz       ACC,  quaternion, quaternion

   loi            1.414213562
   muli           vclsmlftemp, quaternion, I

   mr32.w         matresult[0], vf00
   mr32.w         matresult[1], vf00
   mr32.w         matresult[2], vf00
   move           matresult[3], vf00

   madd.xyz       vcl_2qq, quaternion, quaternion
   addw.xyz       Vector111, vf00, vf00

   opmula.xyz     ACC,  vclsmlftemp, vclsmlftemp
   msubw.xyz      vclsmlftemp2, vclsmlftemp, vclsmlftemp
   maddw.xyz      vclsmlftemp3, vclsmlftemp, vclsmlftemp
   addaw.xyz      ACC,  vf00, vf00
   msubax.yz      ACC,  Vector111, vcl_2qq

   msuby.z        matresult[2], Vector111, vcl_2qq
   msubay.x       ACC, Vector111, vcl_2qq
   msubz.y        matresult[1], Vector111, vcl_2qq
   mr32.y         matresult[0], vclsmlftemp2
   msubz.x        matresult[0], Vector111, vcl_2qq
   mr32.x         matresult[2], vclsmlftemp2
   addy.z         matresult[0], vf00, vclsmlftemp3
   mr32.w         vclsmlftemp, vclsmlftemp2
   mr32.z         matresult[1], vclsmlftemp
   addx.y         matresult[2], vf00, vclsmlftemp3
   mr32.y         vclsmlftemp3, vclsmlftemp3
   mr32.x         matresult[1], vclsmlftemp3

#endmacro

#macro GetForwardQuat: quaternion, out
   mula.xyz       ACC,  quaternion, quaternion

   loi            1.414213562
   muli           vclsmlftemp, quaternion, I

   mr32.w         out, vf00


   madd.xyz       vcl_2qq, quaternion, quaternion
   addw.xyz       Vector111, vf00, vf00

   opmula.xyz     ACC,  vclsmlftemp, vclsmlftemp
   msubw.xyz      vclsmlftemp2, vclsmlftemp, vclsmlftemp
   maddw.xyz      vclsmlftemp3, vclsmlftemp, vclsmlftemp
   addaw.xyz      ACC,  vf00, vf00
   msubax.yz      ACC,  Vector111, vcl_2qq

   msubay.x       ACC, Vector111, vcl_2qq
   msubz.y        out, Vector111, vcl_2qq
   mr32.w         vclsmlftemp, vclsmlftemp2
   mr32.z         out, vclsmlftemp
   mr32.y         vclsmlftemp3, vclsmlftemp3
   mr32.x         out, vclsmlftemp3

#endmacro

#macro QuatToGlobalMatrix: matresult, quaternion, pos
   mula.xyz       ACC,  quaternion, quaternion

   loi            1.414213562
   muli           vclsmlftemp, quaternion, I

   mr32.w         matresult[0], vf00
   mr32.w         matresult[1], vf00
   mr32.w         matresult[2], vf00
   move           matresult[3], vf00

   madd.xyz       vcl_2qq, quaternion, quaternion
   addw.xyz       Vector111, vf00, vf00

   opmula.xyz     ACC,  vclsmlftemp, vclsmlftemp
   msubw.xyz      vclsmlftemp2, vclsmlftemp, vclsmlftemp
   maddw.xyz      vclsmlftemp3, vclsmlftemp, vclsmlftemp
   addaw.xyz      ACC,  vf00, vf00
   msubax.yz      ACC,  Vector111, vcl_2qq

   msuby.z        matresult[2], Vector111, vcl_2qq
   msubay.x       ACC, Vector111, vcl_2qq
   msubz.y        matresult[1], Vector111, vcl_2qq
   mr32.y         matresult[0], vclsmlftemp2
   msubz.x        matresult[0], Vector111, vcl_2qq
   mr32.x         matresult[2], vclsmlftemp2
   addy.z         matresult[0], vf00, vclsmlftemp3
   mr32.w         vclsmlftemp, vclsmlftemp2
   mr32.z         matresult[1], vclsmlftemp
   addx.y         matresult[2], vf00, vclsmlftemp3
   mr32.y         vclsmlftemp3, vclsmlftemp3
   mr32.x         matresult[1], vclsmlftemp3
   move.xyz       matresult[3],  pos

#endmacro

#macro MatrixMultiply: matresult, matleft, matright
   mul            acc,           matright[0], matleft[0][x]
   madd           acc,           matright[1], matleft[0][y]
   madd           acc,           matright[2], matleft[0][z]
   madd           matresult[0],  matright[3], matleft[0][w]
   mul            acc,           matright[0], matleft[1][x]
   madd           acc,           matright[1], matleft[1][y]
   madd           acc,           matright[2], matleft[1][z]
   madd           matresult[1],  matright[3], matleft[1][w]
   mul            acc,           matright[0], matleft[2][x]
   madd           acc,           matright[1], matleft[2][y]
   madd           acc,           matright[2], matleft[2][z]
   madd           matresult[2],  matright[3], matleft[2][w]
   mul            acc,           matright[0], matleft[3][x]
   madd           acc,           matright[1], matleft[3][y]
   madd           acc,           matright[2], matleft[3][z]
   madd           matresult[3],  matright[3], matleft[3][w]
#endmacro