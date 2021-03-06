/*******************************************************************************
* Copyright 2013-2018 Intel Corporation
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
//     AES-GCM
// 
//  Contents:
//        ippsAES_GCMReset()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesauthgcm.h"
#include "pcpaesm.h"
#include "pcptool.h"

/*F*
//    Name: ippsAES_GCMReset
//
// Purpose: Resets AES_GCM context.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState== NULL
//    ippStsContextMatchErr   pState points on invalid context
//    ippStsNoErr             no errors
//
// Parameters:
//    pState       pointer to the context
//
*F*/
IPPFUN(IppStatus, ippsAES_GCMReset,(IppsAES_GCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);

   /* use aligned context */
   pState = (IppsAES_GCMState*)( IPP_ALIGNED_PTR(pState, AESGCM_ALIGNMENT) );
   /* test context validity */
   IPP_BADARG_RET(!AESGCM_VALID_ID(pState), ippStsContextMatchErr);

   /* reset GCM */
   AESGCM_STATE(pState) = GcmInit;
   AESGCM_IV_LEN(pState) = CONST_64(0);
   AESGCM_AAD_LEN(pState) = CONST_64(0);
   AESGCM_TXT_LEN(pState) = CONST_64(0);

   AESGCM_BUFLEN(pState) = 0;
   PaddBlock(0, AESGCM_COUNTER(pState), BLOCK_SIZE);
   PaddBlock(0, AESGCM_ECOUNTER(pState), BLOCK_SIZE);
   PaddBlock(0, AESGCM_ECOUNTER0(pState), BLOCK_SIZE);
   PaddBlock(0, AESGCM_GHASH(pState), BLOCK_SIZE);

   return ippStsNoErr;
}
