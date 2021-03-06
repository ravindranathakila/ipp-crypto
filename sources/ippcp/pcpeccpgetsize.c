/*******************************************************************************
* Copyright 2003-2018 Intel Corporation
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
//     EC over Prime Finite Field (initialization)
// 
//  Contents:
//        ippsECCPGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpeccp.h"

/*F*
//    Name: ippsECCPGetSize
//
// Purpose: Returns size of ECC context (bytes).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
//
//    ippStsSizeErr              2 > feBitSize
//                               feBitSize > EC_GFP_MAXBITSIZE
//    ippStsNoErr                no errors
//
// Parameters:
//    feBitSize   size of field element (bits)
//    pSize       pointer to the size of internal ECC context
//
*F*/
IPPFUN(IppStatus, ippsECCPGetSize, (int feBitSize, int *pSize))
{
   /* test size's pointer */
   IPP_BAD_PTR1_RET(pSize);

   /* test size of field element */
   IPP_BADARG_RET((2>feBitSize || feBitSize>EC_GFP_MAXBITSIZE), ippStsSizeErr);

   {
      /* size of GF context */
      //int gfCtxSize = cpGFpGetSize(feBitSize);
      int gfCtxSize = cpGFpGetSize(feBitSize, feBitSize+BITSIZE(BNU_CHUNK_T), GFP_POOL_SIZE);
      /* size of EC context */
      int ecCtxSize = cpGFpECGetSize(1, feBitSize);

      /* size of EC scratch buffer: 16 points of BITS_BNU_CHUNK(feBitSize)*3 length each */
      int ecScratchBufferSize = 16*(BITS_BNU_CHUNK(feBitSize)*3)*sizeof(BNU_CHUNK_T);

      *pSize = ecCtxSize            /* EC context */
              +ECGFP_ALIGNMENT
              +gfCtxSize            /* GF context */
              +GFP_ALIGNMENT
              +ecScratchBufferSize  /* *scratch buffer */
              +ecScratchBufferSize  /* should be enough for 2 tables */
              +CACHE_LINE_SIZE;

      return ippStsNoErr;
   }
}
