;===============================================================================
; Copyright 2014-2018 Intel Corporation
; All Rights Reserved.
;
; If this  software was obtained  under the  Intel Simplified  Software License,
; the following terms apply:
;
; The source code,  information  and material  ("Material") contained  herein is
; owned by Intel Corporation or its  suppliers or licensors,  and  title to such
; Material remains with Intel  Corporation or its  suppliers or  licensors.  The
; Material  contains  proprietary  information  of  Intel or  its suppliers  and
; licensors.  The Material is protected by  worldwide copyright  laws and treaty
; provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
; modified, published,  uploaded, posted, transmitted,  distributed or disclosed
; in any way without Intel's prior express written permission.  No license under
; any patent,  copyright or other  intellectual property rights  in the Material
; is granted to  or  conferred  upon  you,  either   expressly,  by implication,
; inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
; property rights must be express and approved by Intel in writing.
;
; Unless otherwise agreed by Intel in writing,  you may not remove or alter this
; notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
; suppliers or licensors in any way.
;
;
; If this  software  was obtained  under the  Apache License,  Version  2.0 (the
; "License"), the following terms apply:
;
; You may  not use this  file except  in compliance  with  the License.  You may
; obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
;
;
; Unless  required  by   applicable  law  or  agreed  to  in  writing,  software
; distributed under the License  is distributed  on an  "AS IS"  BASIS,  WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;
; See the   License  for the   specific  language   governing   permissions  and
; limitations under the License.
;===============================================================================

; 
; 
;     Purpose:  Cryptography Primitive.
;               Message block processing according to SHA512
; 
;     Content:
;        UpdateSHA512
; 
;

.686P
.XMM
.MODEL FLAT,C

INCLUDE asmdefs.inc
INCLUDE ia_emm.inc
INCLUDE pcpvariant.inc

IF (_ENABLE_ALG_SHA512_)
IF (_IPP GE _IPP_G9)

IFDEF IPP_PIC

LD_ADDR MACRO reg:REQ, addr:REQ
LOCAL LABEL
        call     LABEL
LABEL:  pop      reg
        sub      reg, LABEL-addr
ENDM

ELSE
LD_ADDR MACRO reg:REQ, addr:REQ
        lea      reg, addr
ENDM
ENDIF



;;
;; ENDIANNESS
;;
ENDIANNESS MACRO xmm:REQ, masks:REQ
   vpshufb  xmm, xmm, masks
ENDM

;;
;; Rotate Right
;;
PRORQ MACRO mm:REQ, nbits:REQ, tmp:REQ
   vpsllq   tmp, mm, (64-nbits)
   vpsrlq   mm, mm, nbits
   vpor     mm, mm,tmp
ENDM


;;
;; Init and Update W:
;;
;; j = 0-15
;; W[j] = ENDIANNESS(src)
;;
;; j = 16-79
;; W[j] = SIGMA1(W[j- 2]) + W[j- 7]
;;       +SIGMA0(W[j-15]) + W[j-16]
;;
;; SIGMA0(x) = ROR64(x,1) ^ROR64(x,8) ^LSR64(x,7)
;; SIGMA1(x) = ROR64(x,19)^ROR64(x,61)^LSR64(x,6)
;;
SIGMA0 MACRO sigma:REQ, x:REQ, t1:REQ,t2:REQ
   vpsrlq   sigma, x, 7
   vmovdqa  t1, x
   PRORQ    x, 1, t2
   vpxor    sigma, sigma, x
   PRORQ    t1,8, t2
   vpxor    sigma, sigma, t1
ENDM

SIGMA1 MACRO sigma:REQ, x:REQ, t1:REQ,t2:REQ
   vpsrlq   sigma, x, 6
   vmovdqa  t1, x
   PRORQ    x, 19, t2
   vpxor    sigma, sigma, x
   PRORQ    t1,61, t2
   vpxor    sigma, sigma, t1
ENDM


;;
;; SHA512 step
;;
;;    Ipp64u T1 = H + SUM1(E) + CHJ(E,F,G) + K_SHA512[t] + W[t];
;;    Ipp64u T2 =     SUM0(A) + MAJ(A,B,C);
;;    D+= T1;
;;    H = T1 + T2;
;;
;; where
;;    SUM1(x) = ROR64(x,14) ^ ROR64(x,18) ^ ROR64(x,41)
;;    SUM0(x) = ROR64(x,28) ^ ROR64(x,34) ^ ROR64(x,39)
;;
;;    CHJ(x,y,z) = (x & y) ^ (~x & z)                          => x&(y^z) ^z
;;    MAJ(x,y,z) = (x & y) ^ (x & z) ^ (y & z) = (x&y)^((x^y)&z)
;;
;; Input:
;;    A,B,C,D,E,F,G,H   - 8 digest's values
;;    pW                - pointer to the W array
;;    pK512             - pointer to the constants
;;    pBuffer           - temporary buffer
;; Output:
;;    A,B,C,D*,E,F,G,H* - 8 digest's values (D and H updated)
;;    pW                - pointer to the W array
;;    pK512             - pointer to the constants
;;    pBuffer           - temporary buffer (changed)
;;
CHJ MACRO   R,E,F,G, T
   vpxor       R, F,G   ; R=(f^g)
   vpand       R, R,E   ; R=e & (f^g)
   vpxor       R, R,G   ; R=e & (f^g) ^g
ENDM

MAJ MACRO   R,A,B,C, T
   vpxor       T, A,B   ; T=a^b
   vpand       R, A,B   ; R=a&b
   vpand       T, T,C   ; T=(a^b)&c
   vpxor       R, R,T   ; R=(a&b)^((a^b)&c)
ENDM

SUM0 MACRO R,X,tmp
   vmovdqa  R,X
   PRORQ    R,28,tmp             ; R=ROR(X,28)
   PRORQ    X,34,tmp             ; X=ROR(X,34)
   vpxor    R, R,X
   PRORQ    X,(39-34),tmp        ; X=ROR(x,39)
   vpxor    R, R,X
ENDM

SUM1 MACRO R,X,tmp
   vmovdqa  R,X
   PRORQ    R,14,tmp             ; R=ROR(X,14)
   PRORQ    X,18,tmp             ; X=ROR(X,18)
   vpxor    R, R,X
   PRORQ    X,(41-18),tmp        ; X=ROR(x,41)
   vpxor    R, R,X
ENDM

SHA512_STEP MACRO A,B,C,D,E,F,G,H, pW,pK512, pBuffer
   vmovdqa     oword ptr[pBuffer+0*sizeof(oword)],E   ; save E
   vmovdqa     oword ptr[pBuffer+1*sizeof(oword)],A   ; save A

   vmovdqa     oword ptr[pBuffer+2*sizeof(oword)],D   ; save D
   vmovdqa     oword ptr[pBuffer+3*sizeof(oword)],H   ; save H

   CHJ         D,E,F,G, H                             ; t1 = h+CHJ(e,f,g)+pW[]+pK512[]
   vmovq       H, qword ptr[pW]
   vpaddq      D, D,H                                 ; +[pW]
   vmovq       H, qword ptr[pK512]
   vpaddq      D, D,H                                 ; +[pK512]
   vpaddq      D, D,oword ptr[pBuffer+3*sizeof(oword)]
   vmovdqa     oword ptr[pBuffer+3*sizeof(oword)],D   ; save t1

   MAJ         H,A,B,C, D        ; t2 = MAJ(a,b,c)
   vmovdqa     oword ptr[pBuffer+4*sizeof(oword)],H   ; save t2

   SUM1        D,E,H             ; D = SUM1(e)
   vpaddq      D, D,oword ptr[pBuffer+3*sizeof(oword)]; t1 = h+CHJ(e,f,g)+pW[]+pK512[] + SUM1(e)

   SUM0        H,A,E             ; H = SUM0(a)
   vpaddq      H, H,oword ptr[pBuffer+4*sizeof(oword)]; t2 = MAJ(a,b,c)+SUM0(a)

   vpaddq      H, H,D            ; h = t1+t2
   vpaddq      D, D,oword ptr[pBuffer+2*sizeof(oword)]; d+= t1

   vmovdqa     E,oword ptr[pBuffer+0*sizeof(oword)]   ; restore E
   vmovdqa     A,oword ptr[pBuffer+1*sizeof(oword)]   ; restore A
ENDM


IPPCODE SEGMENT 'CODE' ALIGN (IPP_ALIGN_FACTOR)


IF _IPP GE _IPP_V8
ALIGN IPP_ALIGN_FACTOR
SWP_BYTE:
pByteSwp DB    7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8
ENDIF

;*******************************************************************************************
;* Purpose:     Update internal digest according to message block
;*
;* void UpdateSHA512(DigestSHA512 digest, const Ipp64u* mblk, int mlen, const void* pParam)
;*
;*******************************************************************************************

;;
;; Lib = W7, V8, P8
;;
;; Caller = ippsSHA512Update
;; Caller = ippsSHA512Final
;; Caller = ippsSHA512MessageDigest
;;
;; Caller = ippsSHA384Update
;; Caller = ippsSHA384Final
;; Caller = ippsSHA384MessageDigest
;;
;; Caller = ippsHMACSHA512Update
;; Caller = ippsHMACSHA512Final
;; Caller = ippsHMACSHA512MessageDigest
;;
;; Caller = ippsHMACSHA384Update
;; Caller = ippsHMACSHA384Final
;; Caller = ippsHMACSHA384MessageDigest
;;
ALIGN IPP_ALIGN_FACTOR
IPPASM UpdateSHA512 PROC NEAR C PUBLIC \
USES esi edi,\
digest:  PTR QWORD,\        ; digest address
mblk:    PTR BYTE,\         ; buffer address
mlen:    DWORD,\            ; buffer length
pSHA512: PTR QWORD          ; address of SHA constants

MBS_SHA512  equ   (128)          ; SHA512 block data size

sSize     =  5                   ; size of save area (oword)
dSize     =  8                   ; size of digest (oword)
wSize     = 80                   ; W values queue (qword)
stackSize = (sSize*sizeof(oword)+dSize*sizeof(oword)+wSize*sizeof(qword)+sizeof(dword))  ; stack size (bytes)

sOffset     = 0                           ; save area
dOffset     = sOffset+sSize*sizeof(oword) ; digest offset
wOffset     = dOffset+dSize*sizeof(oword) ; W values offset
acualOffset = wOffset+wSize*sizeof(qword) ; actual stack size offset

   mov      edi,digest           ; digest address
   mov      esi,mblk             ; source data address
   mov      eax,mlen             ; source data length

   mov      edx, pSHA512         ; table constant address

   sub      esp,stackSize        ; allocate local buffer (probably unaligned)
   mov      ecx,esp
   and      esp,-16              ; 16-byte aligned stack
   sub      ecx,esp
   add      ecx,stackSize        ; acual stack size (bytes)
   mov      [esp+acualOffset],ecx

   vmovq    xmm0,qword ptr[edi+sizeof(qword)*0]    ; A = digest[0]
   vmovq    xmm1,qword ptr[edi+sizeof(qword)*1]    ; B = digest[1]
   vmovq    xmm2,qword ptr[edi+sizeof(qword)*2]    ; C = digest[2]
   vmovq    xmm3,qword ptr[edi+sizeof(qword)*3]    ; D = digest[3]
   vmovq    xmm4,qword ptr[edi+sizeof(qword)*4]    ; E = digest[4]
   vmovq    xmm5,qword ptr[edi+sizeof(qword)*5]    ; F = digest[5]
   vmovq    xmm6,qword ptr[edi+sizeof(qword)*6]    ; G = digest[6]
   vmovq    xmm7,qword ptr[edi+sizeof(qword)*7]    ; H = digest[7]
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*0], xmm0
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*1], xmm1
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*2], xmm2
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*3], xmm3
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*4], xmm4
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*5], xmm5
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*6], xmm6
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*7], xmm7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; process next data block
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
sha512_block_loop:

;;
;; initialize the first 16 qwords in the array W (remember about endian)
;;
  ;vmovdqa  xmm1, oword ptr pByteSwp ; load shuffle mask
   LD_ADDR  ecx, SWP_BYTE
   movdqa   xmm1, oword ptr[ecx+(pByteSwp-SWP_BYTE)]
   mov      ecx,0
   ALIGN IPP_ALIGN_FACTOR
loop1:
   vmovdqu  xmm0, oword ptr [esi+ecx*sizeof(qword)]   ; swap input
   ENDIANNESS xmm0, xmm1
   vmovdqa  oword ptr[esp+wOffset+ecx*sizeof(qword)],xmm0
   add      ecx,sizeof(oword)/sizeof(qword)
   cmp      ecx,16
   jl       loop1

;;
;; initialize another 80-16 qwords in the array W
;;
     ALIGN IPP_ALIGN_FACTOR
loop2:
   vmovdqa  xmm1,oword ptr[esp+ecx*sizeof(qword)+wOffset- 2*sizeof(qword)]       ; xmm1 = W[j-2]
   SIGMA1   xmm0,xmm1,xmm2,xmm3

   vmovdqu  xmm5,oword ptr[esp+ecx*sizeof(qword)+wOffset-15*sizeof(qword)]       ; xmm5 = W[j-15]
   SIGMA0   xmm4,xmm5,xmm6,xmm3

   vmovdqu  xmm7,oword ptr[esp+ecx*sizeof(qword)+wOffset- 7*sizeof(qword)]       ; W[j-7]
   vpaddq   xmm0, xmm0,xmm4
   vpaddq   xmm7, xmm7,oword ptr[esp+ecx*sizeof(qword)+wOffset-16*sizeof(qword)] ; W[j-16]
   vpaddq   xmm0, xmm0,xmm7
   vmovdqa  oword ptr[esp+ecx*sizeof(qword)+wOffset],xmm0

   add      ecx,sizeof(oword)/sizeof(qword)
   cmp      ecx,80
   jl       loop2

;;
;; init A,B,C,D,E,F,G,H by the internal digest
;;
   vmovdqa  xmm0,oword ptr[esp+dOffset+sizeof(oword)*0]    ; A = digest[0]
   vmovdqa  xmm1,oword ptr[esp+dOffset+sizeof(oword)*1]    ; B = digest[1]
   vmovdqa  xmm2,oword ptr[esp+dOffset+sizeof(oword)*2]    ; C = digest[2]
   vmovdqa  xmm3,oword ptr[esp+dOffset+sizeof(oword)*3]    ; D = digest[3]
   vmovdqa  xmm4,oword ptr[esp+dOffset+sizeof(oword)*4]    ; E = digest[4]
   vmovdqa  xmm5,oword ptr[esp+dOffset+sizeof(oword)*5]    ; F = digest[5]
   vmovdqa  xmm6,oword ptr[esp+dOffset+sizeof(oword)*6]    ; G = digest[6]
   vmovdqa  xmm7,oword ptr[esp+dOffset+sizeof(oword)*7]    ; H = digest[7]

;;
;; perform 0-79 steps
;;
   xor      ecx,ecx
  ALIGN IPP_ALIGN_FACTOR
loop3:
;;             A,   B,   C,   D,   E,   F,   G,   H     W[],                                             K[],                                  buffer
;;             --------------------------------------------------------------------------------------------------------------------------------------
   SHA512_STEP xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*0>,<edx+ecx*sizeof(qword)+sizeof(qword)*0>, <esp>
   SHA512_STEP xmm7,xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*1>,<edx+ecx*sizeof(qword)+sizeof(qword)*1>, <esp>
   SHA512_STEP xmm6,xmm7,xmm0,xmm1,xmm2,xmm3,xmm4,xmm5, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*2>,<edx+ecx*sizeof(qword)+sizeof(qword)*2>, <esp>
   SHA512_STEP xmm5,xmm6,xmm7,xmm0,xmm1,xmm2,xmm3,xmm4, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*3>,<edx+ecx*sizeof(qword)+sizeof(qword)*3>, <esp>
   SHA512_STEP xmm4,xmm5,xmm6,xmm7,xmm0,xmm1,xmm2,xmm3, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*4>,<edx+ecx*sizeof(qword)+sizeof(qword)*4>, <esp>
   SHA512_STEP xmm3,xmm4,xmm5,xmm6,xmm7,xmm0,xmm1,xmm2, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*5>,<edx+ecx*sizeof(qword)+sizeof(qword)*5>, <esp>
   SHA512_STEP xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm0,xmm1, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*6>,<edx+ecx*sizeof(qword)+sizeof(qword)*6>, <esp>
   SHA512_STEP xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm0, <esp+ecx*sizeof(qword)+wOffset+sizeof(qword)*7>,<edx+ecx*sizeof(qword)+sizeof(qword)*7>, <esp>

   add      ecx,8
   cmp      ecx,80
   jl       loop3

;;
;; update digest
;;
   vpaddq   xmm0, xmm0,oword ptr[esp+dOffset+sizeof(oword)*0]    ; A += digest[0]
   vpaddq   xmm1, xmm1,oword ptr[esp+dOffset+sizeof(oword)*1]    ; B += digest[1]
   vpaddq   xmm2, xmm2,oword ptr[esp+dOffset+sizeof(oword)*2]    ; C += digest[2]
   vpaddq   xmm3, xmm3,oword ptr[esp+dOffset+sizeof(oword)*3]    ; D += digest[3]
   vpaddq   xmm4, xmm4,oword ptr[esp+dOffset+sizeof(oword)*4]    ; E += digest[4]
   vpaddq   xmm5, xmm5,oword ptr[esp+dOffset+sizeof(oword)*5]    ; F += digest[5]
   vpaddq   xmm6, xmm6,oword ptr[esp+dOffset+sizeof(oword)*6]    ; G += digest[6]
   vpaddq   xmm7, xmm7,oword ptr[esp+dOffset+sizeof(oword)*7]    ; H += digest[7]

   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*0],xmm0    ; digest[0] = A
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*1],xmm1    ; digest[1] = B
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*2],xmm2    ; digest[2] = C
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*3],xmm3    ; digest[3] = D
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*4],xmm4    ; digest[4] = E
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*5],xmm5    ; digest[5] = F
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*6],xmm6    ; digest[6] = G
   vmovdqa  oword ptr[esp+dOffset+sizeof(oword)*7],xmm7    ; digest[7] = H

   add         esi, MBS_SHA512
   sub         eax, MBS_SHA512
   jg          sha512_block_loop

   vmovq    qword ptr[edi+sizeof(qword)*0], xmm0    ; A = digest[0]
   vmovq    qword ptr[edi+sizeof(qword)*1], xmm1    ; B = digest[1]
   vmovq    qword ptr[edi+sizeof(qword)*2], xmm2    ; C = digest[2]
   vmovq    qword ptr[edi+sizeof(qword)*3], xmm3    ; D = digest[3]
   vmovq    qword ptr[edi+sizeof(qword)*4], xmm4    ; E = digest[4]
   vmovq    qword ptr[edi+sizeof(qword)*5], xmm5    ; F = digest[5]
   vmovq    qword ptr[edi+sizeof(qword)*6], xmm6    ; G = digest[6]
   vmovq    qword ptr[edi+sizeof(qword)*7], xmm7    ; H = digest[7]

   add      esp,[esp+acualOffset]
   ret
IPPASM UpdateSHA512 ENDP

ENDIF    ;; (_IPP GE _IPP_G9)
ENDIF    ;; _ENABLE_ALG_SHA512_
END
