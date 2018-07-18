/*******************************************************************************
* Copyright 2005-2018 Intel Corporation
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
//     DL over Prime Finite Field (EC Key Generation, Validation and Set Up)
// 
//  Contents:
//        ippsDLPValidateKeyPair()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"

/*F*
// Name: ippsDLPValidateKeyPair
//
// Purpose: Validate DL Key Pair
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pDL
//                               NULL == pPrvKey
//                               NULL == pPubKey
//
//    ippStsContextMatchErr      invalid pDL->idCtx
//                               invalid pPrvKey->idCtx
//                               invalid pPubKey->idCtx
//
//    ippStsIncompleteContextErr
//                               incomplete context
//
//    ippStsNoErr                no error
//
// Parameters:
//    pPrvKey  pointer to the private key
//    pPubKey  pointer to the public  key
//    pResult  pointer to the result: ippDLValid/
//                                    ippDLInvalidPrivateKey/ippDLInvalidPublicKey/
//                                    ippDLInvalidKeyPair
//    pDL      pointer to the DL context
*F*/
IPPFUN(IppStatus, ippsDLPValidateKeyPair,(const IppsBigNumState* pPrvKey,
                                          const IppsBigNumState* pPubKey,
                                          IppDLResult* pResult,
                                          IppsDLPState* pDL))
{
   /* test DL context */
   IPP_BAD_PTR2_RET(pResult, pDL);
   pDL = (IppsDLPState*)( IPP_ALIGNED_PTR(pDL, DLP_ALIGNMENT) );
   IPP_BADARG_RET(!DLP_VALID_ID(pDL), ippStsContextMatchErr);

   /* test flag */
   IPP_BADARG_RET(!DLP_COMPLETE(pDL), ippStsIncompleteContextErr);

   {
      /* allocate BN resources */
      BigNumNode* pList = DLP_BNCTX(pDL);
      IppsBigNumState* pTmp = cpBigNumListGet(&pList);
      BNU_CHUNK_T* pT = BN_NUMBER(pTmp);

      /* assume keys are OK */
      *pResult = ippDLValid;

      /* private key validation request */
      if(pPrvKey) {
         cpSize lenR = BITS_BNU_CHUNK(DLP_BITSIZER(pDL));
         pPrvKey  = (IppsBigNumState*)( IPP_ALIGNED_PTR(pPrvKey, BN_ALIGNMENT) );
         IPP_BADARG_RET(!BN_VALID_ID(pPrvKey), ippStsContextMatchErr);

         /* test private key: 1 < pPrvKey < (R-1)  */
         cpDec_BNU(pT, DLP_R(pDL),lenR, 1);
         if( 0>=cpBN_cmp(pPrvKey, cpBN_OneRef()) ||
            cpCmp_BNU(BN_NUMBER(pPrvKey),BN_SIZE(pPrvKey), pT,lenR)>=0 ) {
            *pResult = ippDLInvalidPrivateKey;
            return ippStsNoErr;
         }
      }

      /* public key validation request */
      if(pPubKey) {
         cpSize lenP = BITS_BNU_CHUNK(DLP_BITSIZEP(pDL));
         pPubKey = (IppsBigNumState*)( IPP_ALIGNED_PTR(pPubKey, BN_ALIGNMENT) );
         IPP_BADARG_RET(!BN_VALID_ID(pPubKey), ippStsContextMatchErr);

         /* test public key: 1 < pPubKey < (P-1) */
         cpDec_BNU(pT, DLP_P(pDL),lenP, 1);
         if( 0>=cpBN_cmp(pPubKey, cpBN_OneRef()) ||
            cpCmp_BNU(BN_NUMBER(pPubKey),BN_SIZE(pPubKey), pT,lenP)>=0 ) {
            *pResult = ippDLInvalidPublicKey;
            return ippStsNoErr;
         }

         /* addition test: pPubKey = G^pPrvKey (mod P) */
         if(pPrvKey) {
            /* recompute public key */
            cpMontExpBin_BN_sscm(pTmp, DLP_GENC(pDL), pPrvKey, DLP_MONTP0(pDL));
            cpMontDec_BN(pTmp, pTmp, DLP_MONTP0(pDL));

            /* and compare */
            if( cpBN_cmp(pTmp, pPubKey) ) {
               *pResult = ippDLInvalidKeyPair;
               return ippStsNoErr;
            }
         }
      }
   }

   return ippStsNoErr;
}
