/*******************************************************************************
* Copyright 2015-2018 Intel Corporation
* All Rights Reserved.
*
* If this  software was obtained  under the  Intel Simplified  Software License,
* the following terms apply:
*
* The source code,  information  and material  ("Material") contained  herein is
* owned by Intel Corporation or its  suppliers or licensors,  and  title to such
* Material remains with Intel  Corporation or its  suppliers or  licensors.  The
* Material  contains  proprietary  information  of  Intel or  its suppliers  and
* licensors.  The Material is protected by  worldwide copyright  laws and treaty
* provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
* modified, published,  uploaded, posted, transmitted,  distributed or disclosed
* in any way without Intel's prior express written permission.  No license under
* any patent,  copyright or other  intellectual property rights  in the Material
* is granted to  or  conferred  upon  you,  either   expressly,  by implication,
* inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
* property rights must be express and approved by Intel in writing.
*
* Unless otherwise agreed by Intel in writing,  you may not remove or alter this
* notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
* suppliers or licensors in any way.
*
*
* If this  software  was obtained  under the  Apache License,  Version  2.0 (the
* "License"), the following terms apply:
*
* You may  not use this  file except  in compliance  with  the License.  You may
* obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*
*
* Unless  required  by   applicable  law  or  agreed  to  in  writing,  software
* distributed under the License  is distributed  on an  "AS IS"  BASIS,  WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
* See the   License  for the   specific  language   governing   permissions  and
* limitations under the License.
*******************************************************************************/

/* 
// 
//  Purpose:
//     Cryptography Primitive.
//     AES-SIV Functions (RFC 5297)
// 
//  Contents:
//        ippsAES_SIVEncrypt()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_sivstuff.h"

/*F*
//    Name: ippsAES_SIVEncrypt
//
// Purpose: RFC 5297 authenticated encryption
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc==NULL, pDst==NULL
//                            pSIV == NULL
//                            pAuthKey==NULL, pConfKey==NULL
//                            pAD== NULL, pADlen==NULL
//                            pADlen[i]!=0 && pAD[i]==0
//    ippStsLengthErr         keyLen != 16
//                            keyLen != 24
//                            keyLen != 32
//                            pADlen[i]<0
//                            numAD<0
//                            len<=0
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc     pointer to plaintext
//    pDst     pointer to ciphertext
//    len      length (in bytes) of plaintext/ciphertext
//    pSIV     pointer to output SIV
//    pAuthKey pointer to the authentication key
//    pConfKey pointer to the confidendat key
//    keyLen   length of keys
//    pAD[]    array of pointers to input strings
//    pADlen[] array of input string lengths
//    numAD    number of pAD[] and pADlen[] terms
//
*F*/
IPPFUN(IppStatus, ippsAES_SIVEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                            Ipp8u* pSIV,
                                      const Ipp8u* pAuthKey, const Ipp8u* pConfKey, int keyLen,
                                      const Ipp8u* pAD[], const int pADlen[], int numAD))
{
   /* test ciphertext, plaintex and length */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   IPP_BADARG_RET(0>=len, ippStsLengthErr);

   /* test keys & keyLen */
   IPP_BAD_PTR2_RET(pAuthKey, pConfKey);
   IPP_BADARG_RET(keyLen!=16 && keyLen!=24 && keyLen!=32, ippStsLengthErr);

   /* test output vector */
   IPP_BAD_PTR1_RET(pSIV);

   /* test arrays of input AD[] */
   IPP_BAD_PTR2_RET(pAD, pADlen);
   IPP_BADARG_RET(0>numAD, ippStsLengthErr);
   #ifdef _IPP_DEBUG
   {
      int n;
      for(n=0; n<numAD; n++) {
         /* test input message and it's length */
         IPP_BADARG_RET((pADlen[n]<0), ippStsLengthErr);
         /* test source pointer */
         IPP_BADARG_RET((pADlen[n] && !pAD[n]), ippStsNullPtrErr);
      }
   }
   #endif

   {
      int n;

      /* iv and mask */
      Ipp8u iv[MBS_RIJ128];
      Ipp8u vmask[MBS_RIJ128] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                 0x7f,0xff,0xff,0xff,0x7f,0xff,0xff,0xff};
      /* AES context */
      Ipp8u aesBlob[sizeof(IppsAESSpec)+AES_ALIGNMENT];
      IppsAESSpec* paesCtx = (IppsAESSpec*)aesBlob;

      {
         Ipp8u ctxBlob[sizeof(IppsAES_CMACState) + AESCMAC_ALIGNMENT];
         IppsAES_CMACState* pCtx = (IppsAES_CMACState*)ctxBlob;
         cpAES_S2V_init(pSIV, pAuthKey, keyLen, pCtx, sizeof(ctxBlob));

         for(n=0; n<numAD; n++) {
            cpAES_S2V_update(pSIV, pAD[n], pADlen[n], pCtx);
         }
         cpAES_S2V_final(pSIV, pSrc, len, pCtx);

         PurgeBlock(&ctxBlob, sizeof(ctxBlob));
      }

      ippsAESInit(pConfKey, keyLen, paesCtx, sizeof(aesBlob));

      /*construct iv */
      for(n=0; n<MBS_RIJ128; n++) iv[n] = pSIV[n] & vmask[n];

      /* perform AES-CTR encryption */
      ippsAESEncryptCTR(pSrc, pDst, len, paesCtx, iv, BITSIZE(iv));

      PurgeBlock(&aesBlob, sizeof(aesBlob));
      return ippStsNoErr;
   }
}
