/*******************************************************************************
* Copyright 2016-2018 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     GF(p) methods
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpbnumisc.h"
#include "gsmodstuff.h"
#include "pcpgfpstuff.h"
#include "pcpgfpmethod.h"
#include "pcpecprime.h"

//tbcd: temporary excluded: #include <assert.h>

#if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)

#define      p256r1_add      OWNAPI(p256r1_add)
#define      p256r1_sub      OWNAPI(p256r1_sub)
#define      p256r1_neg      OWNAPI(p256r1_neg)
#define      p256r1_div_by_2 OWNAPI(p256r1_div_by_2)
#define      p256r1_mul_by_2 OWNAPI(p256r1_mul_by_2)
#define      p256r1_mul_by_3 OWNAPI(p256r1_mul_by_3)

/* arithmetic over P-256r1 NIST modulus */
BNU_CHUNK_T* p256r1_add(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_sub(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_neg(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_div_by_2 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_mul_by_2 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_mul_by_3 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);

#if(_IPP_ARCH ==_IPP_ARCH_EM64T)

#define      p256r1_mul_montl OWNAPI(p256r1_mul_montl)
#define      p256r1_mul_montx OWNAPI(p256r1_mul_montx)
#define      p256r1_sqr_montl OWNAPI(p256r1_sqr_montl)
#define      p256r1_sqr_montx OWNAPI(p256r1_sqr_montx)
#define      p256r1_to_mont   OWNAPI(p256r1_to_mont)
#define      p256r1_mont_back OWNAPI(p256r1_mont_back)

BNU_CHUNK_T* p256r1_mul_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_mul_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_sqr_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_sqr_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_to_mont  (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_mont_back(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
#endif
#if(_IPP_ARCH ==_IPP_ARCH_IA32)
#define      p256r1_mul_mont_slm OWNAPI(p256r1_mul_mont_slm)
#define      p256r1_sqr_mont_slm OWNAPI(p256r1_sqr_mont_slm)
#define      p256r1_mred         OWNAPI(p256r1_mred)

BNU_CHUNK_T* p256r1_mul_mont_slm(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_sqr_mont_slm(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* p256r1_mred(BNU_CHUNK_T* res, BNU_CHUNK_T* product);
#endif

#define OPERAND_BITSIZE (256)
#define LEN_P256        (BITS_BNU_CHUNK(OPERAND_BITSIZE))


/*
// ia32 multiplicative methods
*/
#if (_IPP_ARCH ==_IPP_ARCH_IA32 )
static BNU_CHUNK_T* p256r1_mul_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(NULL!=product);

   cpMulAdc_BNU_school(product, pA,LEN_P256, pB,LEN_P256);
   p256r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}

static BNU_CHUNK_T* p256r1_sqr_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //tbcd: temporary excluded: assert(NULL!=product);

   cpSqrAdc_BNU_school(product, pA,LEN_P256);
   p256r1_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}


/*
// Montgomery domain conversion constants
*/
static BNU_CHUNK_T RR[] = {
   0x00000003,0x00000000, 0xffffffff,0xfffffffb,
   0xfffffffe,0xffffffff, 0xfffffffd,0x00000004};

static BNU_CHUNK_T one[] = {
   1,0,0,0,0,0,0,0};

static BNU_CHUNK_T* p256r1_to_mont(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return p256r1_mul_montl(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

static BNU_CHUNK_T* p256r1_mont_back(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return p256r1_mul_montl(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}

static BNU_CHUNK_T* p256r1_to_mont_slm(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return p256r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

static BNU_CHUNK_T* p256r1_mont_back_slm(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return p256r1_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}
#endif /* _IPP >= _IPP_P8 */

/*
// return specific gf p256r1 arith methods,
//    p256r1 = 2^256 -2^224 +2^192 +2^96 -1 (NIST P256r1)
*/
static gsModMethod* gsArithGF_p256r1(void)
{
   static gsModMethod m = {
      p256r1_to_mont,
      p256r1_mont_back,
      p256r1_mul_montl,
      p256r1_sqr_montl,
      NULL,
      p256r1_add,
      p256r1_sub,
      p256r1_neg,
      p256r1_div_by_2,
      p256r1_mul_by_2,
      p256r1_mul_by_3,
   };

   #if(_IPP_ARCH==_IPP_ARCH_EM64T) && ((_ADCOX_NI_ENABLING_==_FEATURE_ON_) || (_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_))
   if(IsFeatureEnabled(ippCPUID_ADCOX)) {
      m.mul = p256r1_mul_montx;
      m.sqr = p256r1_sqr_montx;
   }
   #endif

   #if(_IPP_ARCH==_IPP_ARCH_IA32)
   if(IsFeatureEnabled(ippCPUID_SSSE3|ippCPUID_MOVBE) && !IsFeatureEnabled(ippCPUID_AVX)) {
      m.mul = p256r1_mul_mont_slm;
      m.sqr = p256r1_sqr_mont_slm;
      m.encode = p256r1_to_mont_slm;
      m.decode = p256r1_mont_back_slm;
   }
   #endif

   return &m;
}
#endif /* (_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7) */

/*F*
// Name: ippsGFpMethod_p256r1
//
// Purpose: Returns a reference to an implementation of
//          arithmetic operations over GF(q).
//
// Returns:  Pointer to a structure containing an implementation of arithmetic
//           operations over GF(q). q = 2^256 - 2^224 + 2^192 + 2^96 - 1
*F*/

IPPFUN( const IppsGFpMethod*, ippsGFpMethod_p256r1, (void) )
{
   static IppsGFpMethod method = {
      cpID_PrimeP256r1,
      256,
      secp256r1_p,
      NULL
   };

   #if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)
   method.arith = gsArithGF_p256r1();
   #else
   method.arith = gsArithGFp();
   #endif
   return &method;
}

#undef LEN_P256
#undef OPERAND_BITSIZE
