/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncSearch.cpp
 \brief    encoder search class
 */

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TEncSearch.h"

//! \ingroup TLibEncoder
//! \{

static const TComMv s_acMvRefineH[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static const TComMv s_acMvRefineQ[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static const UInt s_auiDFilter[9] =
{
  0, 1, 0,
  2, 3, 2,
  0, 1, 0
};

TEncSearch::TEncSearch()
{
  m_ppcQTTempCoeffY  = NULL;
  m_ppcQTTempCoeffCb = NULL;
  m_ppcQTTempCoeffCr = NULL;
  m_pcQTTempCoeffY   = NULL;
  m_pcQTTempCoeffCb  = NULL;
  m_pcQTTempCoeffCr  = NULL;
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempArlCoeffY  = NULL;
  m_ppcQTTempArlCoeffCb = NULL;
  m_ppcQTTempArlCoeffCr = NULL;
  m_pcQTTempArlCoeffY   = NULL;
  m_pcQTTempArlCoeffCb  = NULL;
  m_pcQTTempArlCoeffCr  = NULL;
#endif
  m_puhQTTempTrIdx   = NULL;
  m_puhQTTempCbf[0] = m_puhQTTempCbf[1] = m_puhQTTempCbf[2] = NULL;
  m_pcQTTempTComYuv  = NULL;
  m_pcEncCfg = NULL;
  m_pcEntropyCoder = NULL;
  m_pTempPel = NULL;
  m_pSharedPredTransformSkip[0] = m_pSharedPredTransformSkip[1] = m_pSharedPredTransformSkip[2] = NULL;
  m_pcQTTempTUCoeffY   = NULL;
  m_pcQTTempTUCoeffCb  = NULL;
  m_pcQTTempTUCoeffCr  = NULL;
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempTUArlCoeffY  = NULL;
  m_ppcQTTempTUArlCoeffCb = NULL;
  m_ppcQTTempTUArlCoeffCr = NULL;
#endif
  m_puhQTTempTransformSkipFlag[0] = NULL;
  m_puhQTTempTransformSkipFlag[1] = NULL;
  m_puhQTTempTransformSkipFlag[2] = NULL;
  setWpScalingDistParam( NULL, -1, REF_PIC_LIST_X );
}

TEncSearch::~TEncSearch()
{
  if ( m_pTempPel )
  {
    delete [] m_pTempPel;
    m_pTempPel = NULL;
  }
  
  if ( m_pcEncCfg )
  {
    const UInt uiNumLayersAllocated = m_pcEncCfg->getQuadtreeTULog2MaxSize()-m_pcEncCfg->getQuadtreeTULog2MinSize()+1;
    for( UInt ui = 0; ui < uiNumLayersAllocated; ++ui )
    {
      delete[] m_ppcQTTempCoeffY[ui];
      delete[] m_ppcQTTempCoeffCb[ui];
      delete[] m_ppcQTTempCoeffCr[ui];
#if ADAPTIVE_QP_SELECTION
      delete[] m_ppcQTTempArlCoeffY[ui];
      delete[] m_ppcQTTempArlCoeffCb[ui];
      delete[] m_ppcQTTempArlCoeffCr[ui];
#endif
      m_pcQTTempTComYuv[ui].destroy();
    }
  }
  delete[] m_ppcQTTempCoeffY;
  delete[] m_ppcQTTempCoeffCb;
  delete[] m_ppcQTTempCoeffCr;
  delete[] m_pcQTTempCoeffY;
  delete[] m_pcQTTempCoeffCb;
  delete[] m_pcQTTempCoeffCr;
#if ADAPTIVE_QP_SELECTION
  delete[] m_ppcQTTempArlCoeffY;
  delete[] m_ppcQTTempArlCoeffCb;
  delete[] m_ppcQTTempArlCoeffCr;
  delete[] m_pcQTTempArlCoeffY;
  delete[] m_pcQTTempArlCoeffCb;
  delete[] m_pcQTTempArlCoeffCr;
#endif
  delete[] m_puhQTTempTrIdx;
  delete[] m_puhQTTempCbf[0];
  delete[] m_puhQTTempCbf[1];
  delete[] m_puhQTTempCbf[2];
  delete[] m_pcQTTempTComYuv;
  delete[] m_pSharedPredTransformSkip[0];
  delete[] m_pSharedPredTransformSkip[1];
  delete[] m_pSharedPredTransformSkip[2];
  delete[] m_pcQTTempTUCoeffY;
  delete[] m_pcQTTempTUCoeffCb;
  delete[] m_pcQTTempTUCoeffCr;
#if ADAPTIVE_QP_SELECTION
  delete[] m_ppcQTTempTUArlCoeffY;
  delete[] m_ppcQTTempTUArlCoeffCb;
  delete[] m_ppcQTTempTUArlCoeffCr;
#endif
  delete[] m_puhQTTempTransformSkipFlag[0];
  delete[] m_puhQTTempTransformSkipFlag[1];
  delete[] m_puhQTTempTransformSkipFlag[2];
  m_pcQTTempTransformSkipTComYuv.destroy();
  m_tmpYuvPred.destroy();
}

void TEncSearch::init(TEncCfg*      pcEncCfg,
                      TComTrQuant*  pcTrQuant,
                      Int           iSearchRange,
                      Int           bipredSearchRange,
                      Int           iFastSearch,
                      Int           iMaxDeltaQP,
                      TEncEntropy*  pcEntropyCoder,
                      TComRdCost*   pcRdCost,
                      TEncSbac*** pppcRDSbacCoder,
                      TEncSbac*   pcRDGoOnSbacCoder
                      )
{
  m_pcEncCfg             = pcEncCfg;
  m_pcTrQuant            = pcTrQuant;
  m_iSearchRange         = iSearchRange;
  m_bipredSearchRange    = bipredSearchRange;
  m_iFastSearch          = iFastSearch;
  m_iMaxDeltaQP          = iMaxDeltaQP;
  m_pcEntropyCoder       = pcEntropyCoder;
  m_pcRdCost             = pcRdCost;
  
  m_pppcRDSbacCoder     = pppcRDSbacCoder;
  m_pcRDGoOnSbacCoder   = pcRDGoOnSbacCoder;

  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iRefIdx = 0; iRefIdx < 33; iRefIdx++)
    {
      m_aaiAdaptSR[iDir][iRefIdx] = iSearchRange;
    }
  }
  
  m_puiDFilter = s_auiDFilter + 4;
  
  // initialize motion cost
#if !FIX203
  m_pcRdCost->initRateDistortionModel( m_iSearchRange << 2 );
#endif
  
  for( Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS+1; iNum++)
  {
    for( Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
    {
      if (iIdx < iNum)
        m_auiMVPIdxCost[iIdx][iNum] = xGetMvpIdxBits(iIdx, iNum);
      else
        m_auiMVPIdxCost[iIdx][iNum] = MAX_INT;
    }
  }
  
  initTempBuff();
  
  m_pTempPel = new Pel[g_uiMaxCUWidth*g_uiMaxCUHeight];
  
  const UInt uiNumLayersToAllocate = pcEncCfg->getQuadtreeTULog2MaxSize()-pcEncCfg->getQuadtreeTULog2MinSize()+1;
  m_ppcQTTempCoeffY  = new TCoeff*[uiNumLayersToAllocate];
  m_ppcQTTempCoeffCb = new TCoeff*[uiNumLayersToAllocate];
  m_ppcQTTempCoeffCr = new TCoeff*[uiNumLayersToAllocate];
  m_pcQTTempCoeffY   = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight   ];
  m_pcQTTempCoeffCb  = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
  m_pcQTTempCoeffCr  = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempArlCoeffY  = new Int*[uiNumLayersToAllocate];
  m_ppcQTTempArlCoeffCb = new Int*[uiNumLayersToAllocate];
  m_ppcQTTempArlCoeffCr = new Int*[uiNumLayersToAllocate];
  m_pcQTTempArlCoeffY   = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight   ];
  m_pcQTTempArlCoeffCb  = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
  m_pcQTTempArlCoeffCr  = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#endif
  
  const UInt uiNumPartitions = 1<<(g_uiMaxCUDepth<<1);
  m_puhQTTempTrIdx   = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[0]  = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[1]  = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[2]  = new UChar  [uiNumPartitions];
  m_pcQTTempTComYuv  = new TComYuv[uiNumLayersToAllocate];
  for( UInt ui = 0; ui < uiNumLayersToAllocate; ++ui )
  {
    m_ppcQTTempCoeffY[ui]  = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight   ];
    m_ppcQTTempCoeffCb[ui] = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
    m_ppcQTTempCoeffCr[ui] = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempArlCoeffY[ui]  = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight   ];
    m_ppcQTTempArlCoeffCb[ui] = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
    m_ppcQTTempArlCoeffCr[ui] = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#endif
    m_pcQTTempTComYuv[ui].create( g_uiMaxCUWidth, g_uiMaxCUHeight );
  }
  m_pSharedPredTransformSkip[0] = new Pel[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_pSharedPredTransformSkip[1] = new Pel[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_pSharedPredTransformSkip[2] = new Pel[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_pcQTTempTUCoeffY  = new TCoeff[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_pcQTTempTUCoeffCb = new TCoeff[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_pcQTTempTUCoeffCr = new TCoeff[MAX_TS_WIDTH*MAX_TS_HEIGHT];
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempTUArlCoeffY  = new Int[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_ppcQTTempTUArlCoeffCb = new Int[MAX_TS_WIDTH*MAX_TS_HEIGHT];
  m_ppcQTTempTUArlCoeffCr = new Int[MAX_TS_WIDTH*MAX_TS_HEIGHT];
#endif
  m_pcQTTempTransformSkipTComYuv.create( g_uiMaxCUWidth, g_uiMaxCUHeight );

  m_puhQTTempTransformSkipFlag[0] = new UChar  [uiNumPartitions];
  m_puhQTTempTransformSkipFlag[1] = new UChar  [uiNumPartitions];
  m_puhQTTempTransformSkipFlag[2] = new UChar  [uiNumPartitions];
  m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE);
}

#if FASTME_SMOOTHER_MV
#define FIRSTSEARCHSTOP     1
#else
#define FIRSTSEARCHSTOP     0
#endif

#define TZ_SEARCH_CONFIGURATION                                                                                 \
const Int  iRaster                  = 5;  /* TZ soll von aussen ?ergeben werden */                            \
const Bool bTestOtherPredictedMV    = 0;                                                                      \
const Bool bTestZeroVector          = 1;                                                                      \
const Bool bTestZeroVectorStart     = 0;                                                                      \
const Bool bTestZeroVectorStop      = 0;                                                                      \
const Bool bFirstSearchDiamond      = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bFirstSearchStop         = FIRSTSEARCHSTOP;                                                        \
const UInt uiFirstSearchRounds      = 3;  /* first search stop X rounds after best match (must be >=1) */     \
const Bool bEnableRasterSearch      = 1;                                                                      \
const Bool bAlwaysRasterSearch      = 0;  /* ===== 1: BETTER but factor 2 slower ===== */                     \
const Bool bRasterRefinementEnable  = 0;  /* enable either raster refinement or star refinement */            \
const Bool bRasterRefinementDiamond = 0;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bStarRefinementEnable    = 1;  /* enable either star refinement or raster refinement */            \
const Bool bStarRefinementDiamond   = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bStarRefinementStop      = 0;                                                                      \
const UInt uiStarRefinementRounds   = 2;  /* star refinement stop X rounds after best match (must be >=1) */  \


__inline Void TEncSearch::xTZSearchHelp( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance )
{
  UInt  uiSad;
  
  Pel*  piRefSrch;
  
  piRefSrch = rcStruct.piRefY + iSearchY * rcStruct.iYStride + iSearchX;
  
  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefSrch, rcStruct.iYStride,  m_cDistParam );
  
  // fast encoder decision: use subsampled SAD when rows > 8 for integer ME
  if ( m_pcEncCfg->getUseFastEnc() )
  {
    if ( m_cDistParam.iRows > 8 )
    {
      m_cDistParam.iSubShift = 1;
    }
  }

  setDistParamComp(0);  // Y component

  // distortion
  m_cDistParam.bitDepth = g_bitDepthY;
  uiSad = m_cDistParam.DistFunc( &m_cDistParam );
  
  // motion cost
  uiSad += m_pcRdCost->getCost( iSearchX, iSearchY );
  
  if( uiSad < rcStruct.uiBestSad )
  {
    rcStruct.uiBestSad      = uiSad;
    rcStruct.iBestX         = iSearchX;
    rcStruct.iBestY         = iSearchY;
    rcStruct.uiBestDistance = uiDistance;
    rcStruct.uiBestRound    = 0;
    rcStruct.ucPointNr      = ucPointNr;
  }
}

__inline Void TEncSearch::xTZ2PointSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 2 point search,                   //   1 2 3
  // check only the 2 untested points  //   4 0 5
  // around the start point            //   6 7 8
  Int iStartX = rcStruct.iBestX;
  Int iStartY = rcStruct.iBestY;
  switch( rcStruct.ucPointNr )
  {
    case 1:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY, 0, 2 );
      }
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
    }
      break;
    case 2:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 3:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
    }
      break;
    case 4:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 5:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 6:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY , 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    case 7:
    {
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 8:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    default:
    {
      assert( false );
    }
      break;
  } // switch( rcStruct.ucPointNr )
}

__inline Void TEncSearch::xTZ8PointSquareSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;
  
  if ( iTop >= iSrchRngVerTop ) // check top
  {
    if ( iLeft >= iSrchRngHorLeft ) // check top left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iTop, 1, iDist );
    }
    // top middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
    
    if ( iRight <= iSrchRngHorRight ) // check top right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iTop, 3, iDist );
    }
  } // check top
  if ( iLeft >= iSrchRngHorLeft ) // check middle left
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
  }
  if ( iRight <= iSrchRngHorRight ) // check middle right
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
  }
  if ( iBottom <= iSrchRngVerBottom ) // check bottom
  {
    if ( iLeft >= iSrchRngHorLeft ) // check bottom left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iBottom, 6, iDist );
    }
    // check bottom middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
    
    if ( iRight <= iSrchRngHorRight ) // check bottom right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iBottom, 8, iDist );
    }
  } // check bottom
}

__inline Void TEncSearch::xTZ8PointDiamondSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert ( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;
  
  if ( iDist == 1 ) // iDist == 1
  {
    if ( iTop >= iSrchRngVerTop ) // check top
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
    }
    if ( iLeft >= iSrchRngHorLeft ) // check middle left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
    }
    if ( iRight <= iSrchRngHorRight ) // check middle right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
    }
    if ( iBottom <= iSrchRngVerBottom ) // check bottom
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
    }
  }
  else // if (iDist != 1)
  {
    if ( iDist <= 8 )
    {
      const Int iTop_2      = iStartY - (iDist>>1);
      const Int iBottom_2   = iStartY + (iDist>>1);
      const Int iLeft_2     = iStartX - (iDist>>1);
      const Int iRight_2    = iStartX + (iDist>>1);
      
      if (  iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iTop,      2, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iTop_2,    1, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2,    3, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,    iStartY,   4, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,   iStartY,   5, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iBottom_2, 6, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iBottom,   7, iDist    );
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
        }
        if ( iTop_2 >= iSrchRngVerTop ) // check half top
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iTop_2, 1, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2, 3, (iDist>>1) );
          }
        } // check half top
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
        }
        if ( iBottom_2 <= iSrchRngVerBottom ) // check half bottom
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iBottom_2, 6, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, (iDist>>1) );
          }
        } // check half bottom
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
        }
      } // check border
    }
    else // iDist > 8
    {
      if ( iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop,    0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,   iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,  iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
        }
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 0, iDist );
        }
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 0, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 0, iDist );
        }
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        }
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);
          
          if ( iPosYT >= iSrchRngVerTop ) // check top
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
            }
          } // check top
          if ( iPosYB <= iSrchRngVerBottom ) // check bottom
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
            }
          } // check bottom
        } // for ...
      } // check border
    } // iDist <= 8
  } // iDist == 1
}

//<--

UInt TEncSearch::xPatternRefinement( TComPattern* pcPatternKey,
                                    TComMv baseRefMv,
                                    Int iFrac, TComMv& rcMvFrac )
{
  UInt  uiDist;
  UInt  uiDistBest  = MAX_UINT;
  UInt  uiDirecBest = 0;
  
  Pel*  piRefPos;
  Int iRefStride = m_filteredBlock[0][0].getStride();
  m_pcRdCost->setDistParam( pcPatternKey, m_filteredBlock[0][0].getLumaAddr(), iRefStride, 1, m_cDistParam, m_pcEncCfg->getUseHADME() );

  const TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);

  for (UInt i = 0; i < 9; i++)
  {
    TComMv cMvTest = pcMvRefine[i];
    cMvTest += baseRefMv;
    
    Int horVal = cMvTest.getHor() * iFrac;
    Int verVal = cMvTest.getVer() * iFrac;
    piRefPos = m_filteredBlock[ verVal & 3 ][ horVal & 3 ].getLumaAddr();

    if ( horVal == 2 && ( verVal & 1 ) == 0 )
    {
      piRefPos += 1;
    }
    if ( ( horVal & 1 ) == 0 && verVal == 2 )
    {
      piRefPos += iRefStride;
    }
    cMvTest = pcMvRefine[i];
    cMvTest += rcMvFrac;

    setDistParamComp(0);  // Y component

    m_cDistParam.pCur = piRefPos;
    m_cDistParam.bitDepth = g_bitDepthY;

    uiDist = m_cDistParam.DistFunc( &m_cDistParam );
    uiDist += m_pcRdCost->getCost( cMvTest.getHor(), cMvTest.getVer() );

    if ( uiDist < uiDistBest )
    {
      uiDistBest  = uiDist;
      uiDirecBest = i;
    }
  }
  
  rcMvFrac = pcMvRefine[uiDirecBest];
  
  return uiDistBest;
}

Void
TEncSearch::xEncSubdivCbfQT( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma )
{
  UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
  UInt  uiTrMode        = pcCU->getTransformIdx( uiAbsPartIdx );
  UInt  uiSubdiv        = ( uiTrMode > uiTrDepth ? 1 : 0 );
  UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;

  if( pcCU->getPredictionMode(0) == MODE_INTRA && pcCU->getPartitionSize(0) == SIZE_NxN && uiTrDepth == 0 )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
  else
  {
    assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
    if( bLuma )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( uiSubdiv, 5 - uiLog2TrafoSize );
    }
  }
  
  if ( bChroma )
  {
    if( uiLog2TrafoSize > 2 )
    {
      if( uiTrDepth==0 || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth-1 ) )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
      }
      if( uiTrDepth==0 || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth-1 ) )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
      }
    }
  }

  if( uiSubdiv )
  {
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xEncSubdivCbfQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, bLuma, bChroma );
    }
    return;
  }
  
  //===== Cbfs =====
  if( bLuma )
  {
    m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
  }
}

Void
TEncSearch::xEncCoeffQT( TComDataCU*  pcCU,
                        UInt         uiTrDepth,
                        UInt         uiAbsPartIdx,
                        TextType     eTextType,
                        Bool         bRealCoeff )
{
  UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
  UInt  uiTrMode        = pcCU->getTransformIdx( uiAbsPartIdx );
  UInt  uiSubdiv        = ( uiTrMode > uiTrDepth ? 1 : 0 );
  UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;
  UInt  uiChroma        = ( eTextType != TEXT_LUMA ? 1 : 0 );
  
  if( uiSubdiv )
  {
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xEncCoeffQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, eTextType, bRealCoeff );
    }
    return;
  }
  
  if( eTextType != TEXT_LUMA && uiLog2TrafoSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }
  
  //===== coefficients =====
  UInt    uiWidth         = pcCU->getWidth  ( 0 ) >> ( uiTrDepth + uiChroma );
  UInt    uiHeight        = pcCU->getHeight ( 0 ) >> ( uiTrDepth + uiChroma );
  UInt    uiCoeffOffset   = ( pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight() * uiAbsPartIdx ) >> ( uiChroma << 1 );
  UInt    uiQTLayer       = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrafoSize;
  TCoeff* pcCoeff         = 0;
  switch( eTextType )
  {
    case TEXT_LUMA:     pcCoeff = ( bRealCoeff ? pcCU->getCoeffY () : m_ppcQTTempCoeffY [uiQTLayer] );  break;
    case TEXT_CHROMA_U: pcCoeff = ( bRealCoeff ? pcCU->getCoeffCb() : m_ppcQTTempCoeffCb[uiQTLayer] );  break;
    case TEXT_CHROMA_V: pcCoeff = ( bRealCoeff ? pcCU->getCoeffCr() : m_ppcQTTempCoeffCr[uiQTLayer] );  break;
    default:            assert(0);
  }
  pcCoeff += uiCoeffOffset;
  
  m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiFullDepth, eTextType );
}


Void
TEncSearch::xEncIntraHeader( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma )
{
  if( bLuma )
  {
    // CU header
    if( uiAbsPartIdx == 0 )
    {
      if( !pcCU->getSlice()->isIntra() )
      {
        if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
        {
          m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, 0, true );
        }
        m_pcEntropyCoder->encodeSkipFlag( pcCU, 0, true );
        m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
      }
      
      m_pcEntropyCoder  ->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );

      if (pcCU->isIntra(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N )
      {
        m_pcEntropyCoder->encodeIPCMInfo( pcCU, 0, true );

        if ( pcCU->getIPCMFlag (0))
        {
          return;
        }
      }
    }
    // luma prediction mode
    if( pcCU->getPartitionSize(0) == SIZE_2Nx2N )
    {
      if( uiAbsPartIdx == 0 )
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, 0 );
      }
    }
    else
    {
      UInt uiQNumParts = pcCU->getTotalNumPart() >> 2;
      if( uiTrDepth == 0 )
      {
        assert( uiAbsPartIdx == 0 );
        for( UInt uiPart = 0; uiPart < 4; uiPart++ )
        {
          m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiPart * uiQNumParts );
        }
      }
      else if( ( uiAbsPartIdx % uiQNumParts ) == 0 )
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiAbsPartIdx );
      }
    }
  }
  if( bChroma )
  {
    // chroma prediction mode
    if( uiAbsPartIdx == 0 )
    {
      m_pcEntropyCoder->encodeIntraDirModeChroma( pcCU, 0, true );
    }
  }
}


UInt
TEncSearch::xGetIntraBitsQT( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma,
                            Bool         bRealCoeff /* just for test */ )
{
  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma );
  xEncSubdivCbfQT ( pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma );
  
  if( bLuma )
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_LUMA,      bRealCoeff );
  }
  if( bChroma )
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_U,  bRealCoeff );
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_V,  bRealCoeff );
  }
  UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  return uiBits;
}

UInt
TEncSearch::xGetIntraBitsQTChroma( TComDataCU*  pcCU,
                                  UInt         uiTrDepth,
                                  UInt         uiAbsPartIdx,
                                  UInt         uiChromaId,
                                  Bool         bRealCoeff /* just for test */ )
{
  m_pcEntropyCoder->resetBits();
  if( uiChromaId == TEXT_CHROMA_U)
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_U,  bRealCoeff );
  }
  else if(uiChromaId == TEXT_CHROMA_V)
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_V,  bRealCoeff );
  }

  UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  return uiBits;
}

Void
TEncSearch::xIntraCodingLumaBlk( TComDataCU* pcCU,
                                UInt        uiTrDepth,
                                UInt        uiAbsPartIdx,
                                TComYuv*    pcOrgYuv, 
                                TComYuv*    pcPredYuv, 
                                TComYuv*    pcResiYuv, 
                                UInt&       ruiDist,
                                Int        default0Save1Load2 )
{
  UInt    uiLumaPredMode    = pcCU     ->getLumaIntraDir     ( uiAbsPartIdx );
  UInt    uiFullDepth       = pcCU     ->getDepth   ( 0 )  + uiTrDepth;
  UInt    uiWidth           = pcCU     ->getWidth   ( 0 ) >> uiTrDepth;
  UInt    uiHeight          = pcCU     ->getHeight  ( 0 ) >> uiTrDepth;
  UInt    uiStride          = pcOrgYuv ->getStride  ();
  Pel*    piOrg             = pcOrgYuv ->getLumaAddr( uiAbsPartIdx );
  Pel*    piPred            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piResi            = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piReco            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  
  UInt    uiLog2TrSize      = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  UInt    uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
  UInt    uiNumCoeffPerInc  = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeff           = m_ppcQTTempCoeffY[ uiQTLayer ] + uiNumCoeffPerInc * uiAbsPartIdx;
#if ADAPTIVE_QP_SELECTION
  Int*    pcArlCoeff        = m_ppcQTTempArlCoeffY[ uiQTLayer ] + uiNumCoeffPerInc * uiAbsPartIdx;
#endif
  Pel*    piRecQt           = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
  UInt    uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();
  
  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  Bool    useTransformSkip  = pcCU->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  if(default0Save1Load2 != 2)
  {
    pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx );
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
    //===== get prediction signal =====
    predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
    // save prediction 
    if(default0Save1Load2 == 1)
    {
      Pel*  pPred   = piPred;
      Pel*  pPredBuf = m_pSharedPredTransformSkip[0];
      Int k = 0;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pPredBuf[ k ++ ] = pPred[ uiX ];
        }
        pPred += uiStride;
      }
    }
  }
  else 
  {
    // load prediction
    Pel*  pPred   = piPred;
    Pel*  pPredBuf = m_pSharedPredTransformSkip[0];
    Int k = 0;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pPred[ uiX ] = pPredBuf[ k ++ ];
      }
      pPred += uiStride;
    }
  }
  //===== get residual signal =====
  {
    // get residual
    Pel*  pOrg    = piOrg;
    Pel*  pPred   = piPred;
    Pel*  pResi   = piResi;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pResi[ uiX ] = pOrg[ uiX ] - pPred[ uiX ];
      }
      pOrg  += uiStride;
      pResi += uiStride;
      pPred += uiStride;
    }
  }
  
  //===== transform and quantization =====
  //--- init rate estimation arrays for RDOQ ---
  if( useTransformSkip? m_pcEncCfg->getUseRDOQTS():m_pcEncCfg->getUseRDOQ())
  {
    m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, TEXT_LUMA );
  }
  //--- transform and quantization ---
  UInt uiAbsSum = 0;
  pcCU       ->setTrIdxSubParts ( uiTrDepth, uiAbsPartIdx, uiFullDepth );

  m_pcTrQuant->setQPforQuant    ( pcCU->getQP( 0 ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

#if RDOQ_CHROMA_LAMBDA 
  m_pcTrQuant->selectLambda     (TEXT_LUMA);  
#endif

  m_pcTrQuant->transformNxN     ( pcCU, piResi, uiStride, pcCoeff, 
#if ADAPTIVE_QP_SELECTION
    pcArlCoeff, 
#endif
    uiWidth, uiHeight, uiAbsSum, TEXT_LUMA, uiAbsPartIdx,useTransformSkip );
  
  //--- set coded block flag ---
  pcCU->setCbfSubParts          ( ( uiAbsSum ? 1 : 0 ) << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
  //--- inverse transform ---
  if( uiAbsSum )
  {
    Int scalingListType = 0 + g_eTTable[(Int)TEXT_LUMA];
    assert(scalingListType < SCALING_LIST_NUM);
    m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA,pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip );
  }
  else
  {
    Pel* pResi = piResi;
    memset( pcCoeff, 0, sizeof( TCoeff ) * uiWidth * uiHeight );
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      memset( pResi, 0, sizeof( Pel ) * uiWidth );
      pResi += uiStride;
    }
  }
  
  //===== reconstruction =====
  {
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = ClipY( pPred[ uiX ] + pResi[ uiX ] );
        pRecQt   [ uiX ] = pReco[ uiX ];
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  
  //===== update distortion =====
  ruiDist += m_pcRdCost->getDistPart(g_bitDepthY, piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight );
}

Void
TEncSearch::xIntraCodingChromaBlk( TComDataCU* pcCU,
                                  UInt        uiTrDepth,
                                  UInt        uiAbsPartIdx,
                                  TComYuv*    pcOrgYuv, 
                                  TComYuv*    pcPredYuv, 
                                  TComYuv*    pcResiYuv, 
                                  UInt&       ruiDist,
                                  UInt        uiChromaId,
                                  Int        default0Save1Load2 )
{
  UInt uiOrgTrDepth = uiTrDepth;
  UInt uiFullDepth  = pcCU->getDepth( 0 ) + uiTrDepth;
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  if( uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }
  
  TextType  eText             = ( uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U );
  UInt      uiChromaPredMode  = pcCU     ->getChromaIntraDir( uiAbsPartIdx );
  UInt      uiWidth           = pcCU     ->getWidth   ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiHeight          = pcCU     ->getHeight  ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiStride          = pcOrgYuv ->getCStride ();
  Pel*      piOrg             = ( uiChromaId > 0 ? pcOrgYuv ->getCrAddr( uiAbsPartIdx ) : pcOrgYuv ->getCbAddr( uiAbsPartIdx ) );
  Pel*      piPred            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piResi            = ( uiChromaId > 0 ? pcResiYuv->getCrAddr( uiAbsPartIdx ) : pcResiYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piReco            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  
  UInt      uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
  UInt      uiNumCoeffPerInc  = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) ) >> 2;
  TCoeff*   pcCoeff           = ( uiChromaId > 0 ? m_ppcQTTempCoeffCr[ uiQTLayer ] : m_ppcQTTempCoeffCb[ uiQTLayer ] ) + uiNumCoeffPerInc * uiAbsPartIdx;
#if ADAPTIVE_QP_SELECTION
  Int*      pcArlCoeff        = ( uiChromaId > 0 ? m_ppcQTTempArlCoeffCr[ uiQTLayer ] : m_ppcQTTempArlCoeffCb[ uiQTLayer ] ) + uiNumCoeffPerInc * uiAbsPartIdx;
#endif
  Pel*      piRecQt           = ( uiChromaId > 0 ? m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr( uiAbsPartIdx ) : m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr( uiAbsPartIdx ) );
  UInt      uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getCStride();
  
  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
  Bool      useTransformSkipChroma       = pcCU->getTransformSkip(uiAbsPartIdx, eText);
  //===== update chroma mode =====
  if( uiChromaPredMode == DM_CHROMA_IDX )
  {
    uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
  }
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  if( default0Save1Load2 != 2 )
  {
    pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );

    pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
    Int*  pPatChroma  = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_piYuvExt ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_piYuvExt ) );

    //===== get prediction signal =====
    {
      predIntraChromaAng( pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
    }
    // save prediction 
    if( default0Save1Load2 == 1 )
    {
      Pel*  pPred   = piPred;
      Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];
      Int k = 0;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pPredBuf[ k ++ ] = pPred[ uiX ];
        }
        pPred += uiStride;
      }
    }
  }
  else
  {
    // load prediction 
    Pel*  pPred   = piPred;
    Pel*  pPredBuf = m_pSharedPredTransformSkip[1 + uiChromaId];
    Int k = 0;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pPred[ uiX ] = pPredBuf[ k ++ ];
      }
      pPred += uiStride;
    }
  }
  //===== get residual signal =====
  {
    // get residual
    Pel*  pOrg    = piOrg;
    Pel*  pPred   = piPred;
    Pel*  pResi   = piResi;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pResi[ uiX ] = pOrg[ uiX ] - pPred[ uiX ];
      }
      pOrg  += uiStride;
      pResi += uiStride;
      pPred += uiStride;
    }
  }
  
  //===== transform and quantization =====
  {
    //--- init rate estimation arrays for RDOQ ---
    if( useTransformSkipChroma? m_pcEncCfg->getUseRDOQTS():m_pcEncCfg->getUseRDOQ())
    {
      m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, eText );
    }
    //--- transform and quantization ---
    UInt uiAbsSum = 0;

    Int curChromaQpOffset;
    if(eText == TEXT_CHROMA_U)
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
    }
    else
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
    }
    m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

#if RDOQ_CHROMA_LAMBDA 
    m_pcTrQuant->selectLambda(eText);
#endif
    m_pcTrQuant->transformNxN      ( pcCU, piResi, uiStride, pcCoeff, 
#if ADAPTIVE_QP_SELECTION
                                     pcArlCoeff, 
#endif
                                     uiWidth, uiHeight, uiAbsSum, eText, uiAbsPartIdx, useTransformSkipChroma );
    //--- set coded block flag ---
    pcCU->setCbfSubParts           ( ( uiAbsSum ? 1 : 0 ) << uiOrgTrDepth, eText, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth );
    //--- inverse transform ---
    if( uiAbsSum )
    {
      Int scalingListType = 0 + g_eTTable[(Int)eText];
      assert(scalingListType < SCALING_LIST_NUM);
      m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkipChroma );
    }
    else
    {
      Pel* pResi = piResi;
      memset( pcCoeff, 0, sizeof( TCoeff ) * uiWidth * uiHeight );
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        memset( pResi, 0, sizeof( Pel ) * uiWidth );
        pResi += uiStride;
      }
    }
  }
  
  //===== reconstruction =====
  {
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = ClipC( pPred[ uiX ] + pResi[ uiX ] );
        pRecQt   [ uiX ] = pReco[ uiX ];
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  
  //===== update distortion =====
  ruiDist += m_pcRdCost->getDistPart(g_bitDepthC, piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight, eText );
}



Void 
TEncSearch::xRecurIntraCodingQT( TComDataCU*  pcCU, 
                                UInt         uiTrDepth,
                                UInt         uiAbsPartIdx, 
                                Bool         bLumaOnly,
                                TComYuv*     pcOrgYuv, 
                                TComYuv*     pcPredYuv, 
                                TComYuv*     pcResiYuv, 
                                UInt&        ruiDistY,
                                UInt&        ruiDistC,
#if HHI_RQT_INTRA_SPEEDUP
                                Bool         bCheckFirst,
#endif
                                Double&      dRDCost )
{
  UInt    uiFullDepth   = pcCU->getDepth( 0 ) +  uiTrDepth;
  UInt    uiLog2TrSize  = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  Bool    bCheckFull    = ( uiLog2TrSize  <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
  Bool    bCheckSplit   = ( uiLog2TrSize  >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
  
#if HHI_RQT_INTRA_SPEEDUP
  Int maxTuSize = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
  Int isIntraSlice = (pcCU->getSlice()->getSliceType() == I_SLICE);
  // don't check split if TU size is less or equal to max TU size
  Bool noSplitIntraMaxTuSize = bCheckFull;
  if(m_pcEncCfg->getRDpenalty() && ! isIntraSlice)
  {
    // in addition don't check split if TU size is less or equal to 16x16 TU size for non-intra slice
    noSplitIntraMaxTuSize = ( uiLog2TrSize  <= min(maxTuSize,4) );

    // if maximum RD-penalty don't check TU size 32x32 
    if(m_pcEncCfg->getRDpenalty()==2)
    {
      bCheckFull    = ( uiLog2TrSize  <= min(maxTuSize,4));
    }
  }
  if( bCheckFirst && noSplitIntraMaxTuSize )
  {
    bCheckSplit = false;
  }
#else
  Int maxTuSize = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
  Int isIntraSlice = (pcCU->getSlice()->getSliceType() == I_SLICE);
  // if maximum RD-penalty don't check TU size 32x32 
  if((m_pcEncCfg->getRDpenalty()==2)  && !isIntraSlice)
  {
    bCheckFull    = ( uiLog2TrSize  <= min(maxTuSize,4));
  }
#endif
  Double  dSingleCost   = MAX_DOUBLE;
  UInt    uiSingleDistY = 0;
  UInt    uiSingleDistC = 0;
  UInt    uiSingleCbfY  = 0;
  UInt    uiSingleCbfU  = 0;
  UInt    uiSingleCbfV  = 0;
  Bool    checkTransformSkip  = pcCU->getSlice()->getPPS()->getUseTransformSkip();
  UInt    widthTransformSkip  = pcCU->getWidth ( 0 ) >> uiTrDepth;
  UInt    heightTransformSkip = pcCU->getHeight( 0 ) >> uiTrDepth;
  Int     bestModeId    = 0;
  Int     bestModeIdUV[2] = {0, 0};
  checkTransformSkip         &= (widthTransformSkip == 4 && heightTransformSkip == 4);
  checkTransformSkip         &= (!pcCU->getCUTransquantBypass(0));
  if ( m_pcEncCfg->getUseTransformSkipFast() )
  {
    checkTransformSkip       &= (pcCU->getPartitionSize(uiAbsPartIdx)==SIZE_NxN);
  }
  if( bCheckFull )
  {
    if(checkTransformSkip == true)
    {
      //----- store original entropy coding status -----
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );

      UInt   singleDistYTmp     = 0;
      UInt   singleDistCTmp     = 0;
      UInt   singleCbfYTmp      = 0;
      UInt   singleCbfUTmp      = 0;
      UInt   singleCbfVTmp      = 0;
      Double singleCostTmp      = 0;
      Int    default0Save1Load2 = 0;
      Int    firstCheckId       = 0;

      UInt   uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + (uiTrDepth - 1) ) << 1 );
      Bool   bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );

      for(Int modeId = firstCheckId; modeId < 2; modeId ++)
      {
        singleDistYTmp = 0;
        singleDistCTmp = 0;
        pcCU ->setTransformSkipSubParts ( modeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth ); 
        if(modeId == firstCheckId)
        {
          default0Save1Load2 = 1;
        }
        else
        {
          default0Save1Load2 = 2;
        }
        //----- code luma block with given intra prediction mode and store Cbf-----
        xIntraCodingLumaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistYTmp,default0Save1Load2); 
        singleCbfYTmp = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth );
        //----- code chroma blocks with given intra prediction mode and store Cbf-----
        if( !bLumaOnly )
        {
          if(bFirstQ)
          {
            pcCU ->setTransformSkipSubParts ( modeId, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth); 
            pcCU ->setTransformSkipSubParts ( modeId, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth); 
          }
          xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 0, default0Save1Load2); 
          xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, 1, default0Save1Load2); 
          singleCbfUTmp = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
          singleCbfVTmp = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
        }
        //----- determine rate and r-d cost -----
        if(modeId == 1 && singleCbfYTmp == 0)
        {
          //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
          singleCostTmp = MAX_DOUBLE; 
        }
        else
        {
          UInt uiSingleBits = xGetIntraBitsQT( pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false );
          singleCostTmp     = m_pcRdCost->calcRdCost( uiSingleBits, singleDistYTmp + singleDistCTmp );
        }

        if(singleCostTmp < dSingleCost)
        {
          dSingleCost   = singleCostTmp;
          uiSingleDistY = singleDistYTmp;
          uiSingleDistC = singleDistCTmp;
          uiSingleCbfY  = singleCbfYTmp;
          uiSingleCbfU  = singleCbfUTmp;
          uiSingleCbfV  = singleCbfVTmp;
          bestModeId    = modeId;
          if(bestModeId == firstCheckId)
          {
            xStoreIntraResultQT(pcCU, uiTrDepth, uiAbsPartIdx,bLumaOnly );
            m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
          }
        }
        if(modeId == firstCheckId)
        {
          m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
        }
      }

      pcCU ->setTransformSkipSubParts ( bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth ); 

      if(bestModeId == firstCheckId)
      {
        xLoadIntraResultQT(pcCU, uiTrDepth, uiAbsPartIdx,bLumaOnly );
        pcCU->setCbfSubParts  ( uiSingleCbfY << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
        if( !bLumaOnly )
        {
          if(bFirstQ)
          {
            pcCU->setCbfSubParts( uiSingleCbfU << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth( 0 ) + uiTrDepth - 1 );
            pcCU->setCbfSubParts( uiSingleCbfV << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth( 0 ) + uiTrDepth - 1 );
          }
        }
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
      }

      if( !bLumaOnly )
      {
        bestModeIdUV[0] = bestModeIdUV[1] = bestModeId;
        if(bFirstQ && bestModeId == 1)
        {
          //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
          if(uiSingleCbfU == 0)
          {
            pcCU ->setTransformSkipSubParts ( 0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth); 
            bestModeIdUV[0] = 0;
          }
          if(uiSingleCbfV == 0)
          {
            pcCU ->setTransformSkipSubParts ( 0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth); 
            bestModeIdUV[1] = 0;
          }
        }
      }
    }
    else
    {
      pcCU ->setTransformSkipSubParts ( 0, TEXT_LUMA, uiAbsPartIdx, uiFullDepth ); 
      //----- store original entropy coding status -----
      if( bCheckSplit )
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
      }
      //----- code luma block with given intra prediction mode and store Cbf-----
      dSingleCost   = 0.0;
      xIntraCodingLumaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistY ); 
      if( bCheckSplit )
      {
        uiSingleCbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth );
      }
      //----- code chroma blocks with given intra prediction mode and store Cbf-----
      if( !bLumaOnly )
      {
        pcCU ->setTransformSkipSubParts ( 0, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth ); 
        pcCU ->setTransformSkipSubParts ( 0, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth ); 
        xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 0 ); 
        xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 1 ); 
        if( bCheckSplit )
        {
          uiSingleCbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
          uiSingleCbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
        }
      }
      //----- determine rate and r-d cost -----
      UInt uiSingleBits = xGetIntraBitsQT( pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false );
      if(m_pcEncCfg->getRDpenalty() && (uiLog2TrSize==5) && !isIntraSlice)
      {
        uiSingleBits=uiSingleBits*4; 
      }
      dSingleCost       = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDistY + uiSingleDistC );
    }
  }
  
  if( bCheckSplit )
  {
    //----- store full entropy coding status, load original entropy coding status -----
    if( bCheckFull )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    else
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }

    //----- code splitted block -----
    Double  dSplitCost      = 0.0;
    UInt    uiSplitDistY    = 0;
    UInt    uiSplitDistC    = 0;
    UInt    uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    UInt    uiAbsPartIdxSub = uiAbsPartIdx;

    UInt    uiSplitCbfY = 0;
    UInt    uiSplitCbfU = 0;
    UInt    uiSplitCbfV = 0;

    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv )
    {
#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, bCheckFirst, dSplitCost );
#else
      xRecurIntraCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, dSplitCost );
#endif

      uiSplitCbfY |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_LUMA, uiTrDepth + 1 );
      if(!bLumaOnly)
      {
        uiSplitCbfU |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1 );
        uiSplitCbfV |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1 );
      }
    }

    for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
    {
      pcCU->getCbf( TEXT_LUMA )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfY << uiTrDepth );
    }
    if( !bLumaOnly )
    {
      for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
      {
        pcCU->getCbf( TEXT_CHROMA_U )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfU << uiTrDepth );
        pcCU->getCbf( TEXT_CHROMA_V )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfV << uiTrDepth );
      }
    }
    //----- restore context states -----
    m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );

    //----- determine rate and r-d cost -----
    UInt uiSplitBits = xGetIntraBitsQT( pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false );
    dSplitCost       = m_pcRdCost->calcRdCost( uiSplitBits, uiSplitDistY + uiSplitDistC );
    
    //===== compare and set best =====
    if( dSplitCost < dSingleCost )
    {
      //--- update cost ---
      ruiDistY += uiSplitDistY;
      ruiDistC += uiSplitDistC;
      dRDCost  += dSplitCost;
      return;
    }
    //----- set entropy coding status -----
    m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );
    
    //--- set transform index and Cbf values ---
    pcCU->setTrIdxSubParts( uiTrDepth, uiAbsPartIdx, uiFullDepth );
    pcCU->setCbfSubParts  ( uiSingleCbfY << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
    pcCU ->setTransformSkipSubParts  ( bestModeId, TEXT_LUMA, uiAbsPartIdx, uiFullDepth ); 
    if( !bLumaOnly )
    {
      pcCU->setCbfSubParts( uiSingleCbfU << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth );
      pcCU->setCbfSubParts( uiSingleCbfV << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth );
      pcCU->setTransformSkipSubParts ( bestModeIdUV[0], TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth); 
      pcCU->setTransformSkipSubParts ( bestModeIdUV[1], TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth); 
    }
    
    //--- set reconstruction for next intra prediction blocks ---
    UInt  uiWidth     = pcCU->getWidth ( 0 ) >> uiTrDepth;
    UInt  uiHeight    = pcCU->getHeight( 0 ) >> uiTrDepth;
    UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt  uiZOrder    = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*  piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
    UInt  uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();
    Pel*  piDes       = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
    UInt  uiDesStride = pcCU->getPic()->getPicYuvRec()->getStride  ();
    for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        piDes[ uiX ] = piSrc[ uiX ];
      }
    }
    if( !bLumaOnly )
    {
      uiWidth   >>= 1;
      uiHeight  >>= 1;
      piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr  ( uiAbsPartIdx );
      uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getCStride ();
      piDes       = pcCU->getPic()->getPicYuvRec()->getCbAddr ( pcCU->getAddr(), uiZOrder );
      uiDesStride = pcCU->getPic()->getPicYuvRec()->getCStride();
      for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
      piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr  ( uiAbsPartIdx );
      piDes       = pcCU->getPic()->getPicYuvRec()->getCrAddr ( pcCU->getAddr(), uiZOrder );
      for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
    }
  }
  ruiDistY += uiSingleDistY;
  ruiDistC += uiSingleDistC;
  dRDCost  += dSingleCost;
}


Void
TEncSearch::xSetIntraResultQT( TComDataCU* pcCU,
                              UInt        uiTrDepth,
                              UInt        uiAbsPartIdx,
                              Bool        bLumaOnly,
                              TComYuv*    pcRecoYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    
    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if( !bLumaOnly && uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
      bSkipChroma  = ( ( uiAbsPartIdx % uiQPDiv ) != 0 );
      bChromaSame  = true;
    }
    
    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    UInt uiNumCoeffIncY = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
    TCoeff* pcCoeffDstY = pcCU->getCoeffY ()              + ( uiNumCoeffIncY * uiAbsPartIdx );
    ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
    Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
    Int* pcArlCoeffDstY = pcCU->getArlCoeffY ()              + ( uiNumCoeffIncY * uiAbsPartIdx );
    ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
    if( !bLumaOnly && !bSkipChroma )
    {
      UInt uiNumCoeffC    = ( bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2 );
      UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
      TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
      ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
      Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
      ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    }
    
    //===== copy reconstruction =====
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartLuma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize );
    if( !bLumaOnly && !bSkipChroma )
    {
      UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
      m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
    }
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xSetIntraResultQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, bLumaOnly, pcRecoYuv );
    }
  }
}

Void
TEncSearch::xStoreIntraResultQT( TComDataCU* pcCU,
                                UInt        uiTrDepth,
                                UInt        uiAbsPartIdx,
                                Bool        bLumaOnly )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  assert(  uiTrMode == uiTrDepth );
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

  Bool bSkipChroma  = false;
  Bool bChromaSame  = false;
  if( !bLumaOnly && uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
    bSkipChroma  = ( ( uiAbsPartIdx % uiQPDiv ) != 0 );
    bChromaSame  = true;
  }

  //===== copy transform coefficients =====
  UInt uiNumCoeffY    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
  UInt uiNumCoeffIncY = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
  TCoeff* pcCoeffDstY = m_pcQTTempTUCoeffY;

  ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
  Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
  Int* pcArlCoeffDstY = m_ppcQTTempTUArlCoeffY;
  ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
  if( !bLumaOnly && !bSkipChroma )
  {
    UInt uiNumCoeffC    = ( bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2 );
    UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
    TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstU = m_pcQTTempTUCoeffCb;
    TCoeff* pcCoeffDstV = m_pcQTTempTUCoeffCr;
    ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
    ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
    Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstU = m_ppcQTTempTUArlCoeffCb;
    Int* pcArlCoeffDstV = m_ppcQTTempTUArlCoeffCr;
    ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
    ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
  }

  //===== copy reconstruction =====
  m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartLuma( &m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize );

  if( !bLumaOnly && !bSkipChroma )
  {
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma( &m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
  }
}

Void
TEncSearch::xLoadIntraResultQT( TComDataCU* pcCU,
                               UInt        uiTrDepth,
                               UInt        uiAbsPartIdx,
                               Bool        bLumaOnly )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  assert(  uiTrMode == uiTrDepth );
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

  Bool bSkipChroma  = false;
  Bool bChromaSame  = false;
  if( !bLumaOnly && uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
    bSkipChroma  = ( ( uiAbsPartIdx % uiQPDiv ) != 0 );
    bChromaSame  = true;
  }

  //===== copy transform coefficients =====
  UInt uiNumCoeffY    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
  UInt uiNumCoeffIncY = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeffDstY = m_ppcQTTempCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
  TCoeff* pcCoeffSrcY = m_pcQTTempTUCoeffY;

  ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
  Int* pcArlCoeffDstY = m_ppcQTTempArlCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
  Int* pcArlCoeffSrcY = m_ppcQTTempTUArlCoeffY;
  ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
  if( !bLumaOnly && !bSkipChroma )
  {
    UInt uiNumCoeffC    = ( bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2 );
    UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
    TCoeff* pcCoeffDstU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffSrcU = m_pcQTTempTUCoeffCb;
    TCoeff* pcCoeffSrcV = m_pcQTTempTUCoeffCr;
    ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
    ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
    Int* pcArlCoeffDstU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffSrcU = m_ppcQTTempTUArlCoeffCb;
    Int* pcArlCoeffSrcV = m_ppcQTTempTUArlCoeffCr;
    ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
    ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
  }

  //===== copy reconstruction =====
  m_pcQTTempTransformSkipTComYuv.copyPartToPartLuma( &m_pcQTTempTComYuv[ uiQTLayer ] , uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize );

  if( !bLumaOnly && !bSkipChroma )
  {
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTransformSkipTComYuv.copyPartToPartChroma( &m_pcQTTempTComYuv[ uiQTLayer ], uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
  }

  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  Pel*    piRecQt           = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
  UInt    uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();
  UInt    uiWidth           = pcCU     ->getWidth   ( 0 ) >> uiTrDepth;
  UInt    uiHeight          = pcCU     ->getHeight  ( 0 ) >> uiTrDepth;
  Pel* pRecQt     = piRecQt;
  Pel* pRecIPred  = piRecIPred;
  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      pRecIPred[ uiX ] = pRecQt   [ uiX ];
    }
    pRecQt    += uiRecQtStride;
    pRecIPred += uiRecIPredStride;
  }

  if( !bLumaOnly && !bSkipChroma )
  {
    piRecIPred = pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder );
    piRecQt    = m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr( uiAbsPartIdx );
    pRecQt     = piRecQt;
    pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pRecIPred[ uiX ] = pRecQt[ uiX ];
      }
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }

    piRecIPred = pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder );
    piRecQt    = m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr( uiAbsPartIdx );
    pRecQt     = piRecQt;
    pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pRecIPred[ uiX ] = pRecQt[ uiX ];
      }
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }
  }
}

Void
TEncSearch::xStoreIntraResultChromaQT( TComDataCU* pcCU,
                                      UInt        uiTrDepth,
                                      UInt        uiAbsPartIdx,
                                      UInt        stateU0V1Both2 )
{
  UInt uiFullDepth = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode    = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bChromaSame = false;
    if( uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      uiTrDepth --;
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth) << 1 );
      if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
      {
        return;
      }
      bChromaSame = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffC    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    if( !bChromaSame )
    {
      uiNumCoeffC     >>= 2;
    }
    UInt uiNumCoeffIncC = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) + 2 );
    if(stateU0V1Both2 == 0 || stateU0V1Both2 == 2)
    {
      TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstU = m_pcQTTempTUCoeffCb;
      ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );

#if ADAPTIVE_QP_SELECTION    
      Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstU = m_ppcQTTempTUArlCoeffCb;
      ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
#endif
    }
    if(stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
    {
      TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstV = m_pcQTTempTUCoeffCr;
      ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION    
      Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstV = m_ppcQTTempTUArlCoeffCr;
      ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    }

    //===== copy reconstruction =====
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma(&m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma, stateU0V1Both2 );
  }
}


Void
TEncSearch::xLoadIntraResultChromaQT( TComDataCU* pcCU,
                                     UInt        uiTrDepth,
                                     UInt        uiAbsPartIdx,
                                     UInt        stateU0V1Both2 )
{
  UInt uiFullDepth = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode    = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool bChromaSame = false;
    if( uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      uiTrDepth --;
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
      if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
      {
        return;
      }
      bChromaSame = true;
    }

    //===== copy transform coefficients =====
    UInt uiNumCoeffC = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    if( !bChromaSame )
    {
      uiNumCoeffC >>= 2;
    }
    UInt uiNumCoeffIncC = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) + 2 );

    if(stateU0V1Both2 ==0 || stateU0V1Both2 == 2)
    {
      TCoeff* pcCoeffDstU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffSrcU = m_pcQTTempTUCoeffCb;
      ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION    
      Int* pcArlCoeffDstU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffSrcU = m_ppcQTTempTUArlCoeffCb;
      ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
#endif
    }
    if(stateU0V1Both2 ==1 || stateU0V1Both2 == 2)
    {
      TCoeff* pcCoeffDstV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffSrcV = m_pcQTTempTUCoeffCr;
      ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION    
      Int* pcArlCoeffDstV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffSrcV = m_ppcQTTempTUArlCoeffCr;       
      ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    }

    //===== copy reconstruction =====
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTransformSkipTComYuv.copyPartToPartChroma( &m_pcQTTempTComYuv[ uiQTLayer ], uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma, stateU0V1Both2);

    UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    UInt    uiWidth           = pcCU->getWidth   ( 0 ) >> (uiTrDepth + 1);
    UInt    uiHeight          = pcCU->getHeight  ( 0 ) >> (uiTrDepth + 1);
    UInt    uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getCStride  ();
    UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride  ();

    if(stateU0V1Both2 ==0 || stateU0V1Both2 == 2)
    {
      Pel* piRecIPred = pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder );
      Pel* piRecQt    = m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr( uiAbsPartIdx );
      Pel* pRecQt     = piRecQt;
      Pel* pRecIPred  = piRecIPred;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pRecIPred[ uiX ] = pRecQt[ uiX ];
        }
        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
      }
    }
    if(stateU0V1Both2 == 1 || stateU0V1Both2 == 2)
    {
      Pel* piRecIPred = pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder );
      Pel* piRecQt    = m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr( uiAbsPartIdx );
      Pel* pRecQt     = piRecQt;
      Pel* pRecIPred  = piRecIPred;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pRecIPred[ uiX ] = pRecQt[ uiX ];
        }
        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
      }
    }
  }
}

Void 
TEncSearch::xRecurIntraChromaCodingQT( TComDataCU*  pcCU, 
                                      UInt         uiTrDepth,
                                      UInt         uiAbsPartIdx, 
                                      TComYuv*     pcOrgYuv, 
                                      TComYuv*     pcPredYuv, 
                                      TComYuv*     pcResiYuv, 
                                      UInt&        ruiDist )
{
  UInt uiFullDepth = pcCU->getDepth( 0 ) +  uiTrDepth;
  UInt uiTrMode    = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    Bool checkTransformSkip = pcCU->getSlice()->getPPS()->getUseTransformSkip();
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;

    UInt actualTrDepth = uiTrDepth;
    if( uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      actualTrDepth--;
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + actualTrDepth) << 1 );
      Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
      if( !bFirstQ )
      {
        return;
      }
    }

    checkTransformSkip &= (uiLog2TrSize <= 3);
    if ( m_pcEncCfg->getUseTransformSkipFast() )
    {
      checkTransformSkip &= (uiLog2TrSize < 3);
      if (checkTransformSkip)
      {
        Int nbLumaSkip = 0;
        for(UInt absPartIdxSub = uiAbsPartIdx; absPartIdxSub < uiAbsPartIdx + 4; absPartIdxSub ++)
        {
          nbLumaSkip += pcCU->getTransformSkip(absPartIdxSub, TEXT_LUMA);
        }
        checkTransformSkip &= (nbLumaSkip > 0);
      }
    }

    if(checkTransformSkip)
    {
      //use RDO to decide whether Cr/Cb takes TS
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT] );
      
      for(Int chromaId = 0; chromaId < 2; chromaId ++)
      {
        Double  dSingleCost    = MAX_DOUBLE;
        Int     bestModeId     = 0;
        UInt    singleDistC    = 0;
        UInt    singleCbfC     = 0;
        UInt    singleDistCTmp = 0;
        Double  singleCostTmp  = 0;
        UInt    singleCbfCTmp  = 0;
        
        Int     default0Save1Load2 = 0;
        Int     firstCheckId       = 0;
        
        for(Int chromaModeId = firstCheckId; chromaModeId < 2; chromaModeId ++)
        {
          pcCU->setTransformSkipSubParts ( chromaModeId, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth( 0 ) +  actualTrDepth);
          if(chromaModeId == firstCheckId)
          {
            default0Save1Load2 = 1;
          }
          else
          {
            default0Save1Load2 = 2;
          }
          singleDistCTmp = 0;
          xIntraCodingChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, singleDistCTmp, chromaId ,default0Save1Load2);
          singleCbfCTmp = pcCU->getCbf( uiAbsPartIdx, (TextType)(chromaId + 2), uiTrDepth);
          
          if(chromaModeId == 1 && singleCbfCTmp == 0)
          {
            //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
            singleCostTmp = MAX_DOUBLE;
          }
          else
          {
            UInt bitsTmp = xGetIntraBitsQTChroma( pcCU,uiTrDepth, uiAbsPartIdx,chromaId + 2, false );
            singleCostTmp  = m_pcRdCost->calcRdCost( bitsTmp, singleDistCTmp);
          }
          
          if(singleCostTmp < dSingleCost)
          {
            dSingleCost = singleCostTmp;
            singleDistC = singleDistCTmp;
            bestModeId  = chromaModeId;
            singleCbfC  = singleCbfCTmp;
            
            if(bestModeId == firstCheckId)
            {
              xStoreIntraResultChromaQT(pcCU, uiTrDepth, uiAbsPartIdx,chromaId);
              m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
            }
          }
          if(chromaModeId == firstCheckId)
          {
            m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
          }
        }
        
        if(bestModeId == firstCheckId)
        {
          xLoadIntraResultChromaQT(pcCU, uiTrDepth, uiAbsPartIdx,chromaId);
          pcCU->setCbfSubParts ( singleCbfC << uiTrDepth, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth(0) + actualTrDepth );
          m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
        }
        pcCU ->setTransformSkipSubParts( bestModeId, (TextType)(chromaId + 2), uiAbsPartIdx, pcCU->getDepth( 0 ) +  actualTrDepth );
        ruiDist += singleDistC;
        
        if(chromaId == 0)
        {
          m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT] );
        }
      }
    }
    else
    {
      pcCU ->setTransformSkipSubParts( 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth( 0 ) +  actualTrDepth );
      pcCU ->setTransformSkipSubParts( 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth( 0 ) +  actualTrDepth ); 
      xIntraCodingChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 0 ); 
      xIntraCodingChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 1 ); 
    }
  }
  else
  {
    UInt uiSplitCbfU     = 0;
    UInt uiSplitCbfV     = 0;
    UInt uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    UInt uiAbsPartIdxSub = uiAbsPartIdx;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv )
    {
      xRecurIntraChromaCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist );
      uiSplitCbfU |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1 );
      uiSplitCbfV |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
    {
      pcCU->getCbf( TEXT_CHROMA_U )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfU << uiTrDepth );
      pcCU->getCbf( TEXT_CHROMA_V )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfV << uiTrDepth );
    }
  }
}

Void
TEncSearch::xSetIntraResultChromaQT( TComDataCU* pcCU,
                                    UInt        uiTrDepth,
                                    UInt        uiAbsPartIdx,
                                    TComYuv*    pcRecoYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    
    Bool bChromaSame  = false;
    if( uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
      if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
      {
        return;
      }
      bChromaSame     = true;
    }
    
    //===== copy transform coefficients =====
    UInt uiNumCoeffC    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    if( !bChromaSame )
    {
      uiNumCoeffC     >>= 2;
    }
    UInt uiNumCoeffIncC = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) + 2 );
    TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
    ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION    
    Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
    ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    
    //===== copy reconstruction =====
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xSetIntraResultChromaQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv );
    }
  }
}


Void 
TEncSearch::preestChromaPredMode( TComDataCU* pcCU, 
                                 TComYuv*    pcOrgYuv, 
                                 TComYuv*    pcPredYuv )
{
  UInt  uiWidth     = pcCU->getWidth ( 0 ) >> 1;
  UInt  uiHeight    = pcCU->getHeight( 0 ) >> 1;
  UInt  uiStride    = pcOrgYuv ->getCStride();
  Pel*  piOrgU      = pcOrgYuv ->getCbAddr ( 0 );
  Pel*  piOrgV      = pcOrgYuv ->getCrAddr ( 0 );
  Pel*  piPredU     = pcPredYuv->getCbAddr ( 0 );
  Pel*  piPredV     = pcPredYuv->getCrAddr ( 0 );
  
  //===== init pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, 0, 0 );
  pcCU->getPattern()->initAdiPatternChroma( pcCU, 0, 0, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
  Int*  pPatChromaU = pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_piYuvExt );
  Int*  pPatChromaV = pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_piYuvExt );
  
  //===== get best prediction modes (using SAD) =====
  UInt  uiMinMode   = 0;
  UInt  uiMaxMode   = 4;
  UInt  uiBestMode  = MAX_UINT;
  UInt  uiMinSAD    = MAX_UINT;
  for( UInt uiMode  = uiMinMode; uiMode < uiMaxMode; uiMode++ )
  {
    //--- get prediction ---
    predIntraChromaAng( pPatChromaU, uiMode, piPredU, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
    predIntraChromaAng( pPatChromaV, uiMode, piPredV, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
    
    //--- get SAD ---
    UInt  uiSAD  = m_pcRdCost->calcHAD(g_bitDepthC, piOrgU, uiStride, piPredU, uiStride, uiWidth, uiHeight );
    uiSAD       += m_pcRdCost->calcHAD(g_bitDepthC, piOrgV, uiStride, piPredV, uiStride, uiWidth, uiHeight );
    //--- check ---
    if( uiSAD < uiMinSAD )
    {
      uiMinSAD   = uiSAD;
      uiBestMode = uiMode;
    }
  }
  
  //===== set chroma pred mode =====
  pcCU->setChromIntraDirSubParts( uiBestMode, 0, pcCU->getDepth( 0 ) );
}

Void 
TEncSearch::estIntraPredQT( TComDataCU* pcCU, 
                           TComYuv*    pcOrgYuv, 
                           TComYuv*    pcPredYuv, 
                           TComYuv*    pcResiYuv,
#if IT_RESIDUAL_FILE
                           TComYuv*    pcIntraResiYuv,
#endif
                           TComYuv*    pcRecoYuv,
                           UInt&       ruiDistC,
                           Bool        bLumaOnly )
{
  UInt    uiDepth        = pcCU->getDepth(0);
  UInt    uiNumPU        = pcCU->getNumPartitions();
  UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
  UInt    uiWidth        = pcCU->getWidth (0) >> uiInitTrDepth;
  UInt    uiHeight       = pcCU->getHeight(0) >> uiInitTrDepth;
  UInt    uiQNumParts    = pcCU->getTotalNumPart() >> 2;
  UInt    uiWidthBit     = pcCU->getIntraSizeIdx(0);
  UInt    uiOverallDistY = 0;
  UInt    uiOverallDistC = 0;
  UInt    CandNum;
  Double  CandCostList[ FAST_UDI_MAX_RDMODE_NUM ];
  
  //===== set QP and clear Cbf =====
  if ( pcCU->getSlice()->getPPS()->getUseDQP() == true)
  {
    pcCU->setQPSubParts( pcCU->getQP(0), 0, uiDepth );
  }
  else
  {
    pcCU->setQPSubParts( pcCU->getSlice()->getSliceQp(), 0, uiDepth );
  }
  
  //===== loop over partitions =====
  UInt uiPartOffset = 0;
  for( UInt uiPU = 0; uiPU < uiNumPU; uiPU++, uiPartOffset += uiQNumParts )
  {
    //===== init pattern for luma prediction =====
    Bool bAboveAvail = false;
    Bool bLeftAvail  = false;
    pcCU->getPattern()->initPattern   ( pcCU, uiInitTrDepth, uiPartOffset );
    pcCU->getPattern()->initAdiPattern( pcCU, uiPartOffset, uiInitTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
    
    //===== determine set of modes to be tested (using prediction signal only) =====
    Int numModesAvailable     = 35; //total number of Intra modes
    Pel* piOrg         = pcOrgYuv ->getLumaAddr( uiPU, uiWidth );
    Pel* piPred        = pcPredYuv->getLumaAddr( uiPU, uiWidth );
    UInt uiStride      = pcPredYuv->getStride();
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    Int numModesForFullRD = g_aucIntraModeNumFast[ uiWidthBit ];
    
    Bool doFastSearch = (numModesForFullRD != numModesAvailable);
    if (doFastSearch)
    {
      assert(numModesForFullRD < numModesAvailable);

      for( Int i=0; i < numModesForFullRD; i++ ) 
      {
        CandCostList[ i ] = MAX_DOUBLE;
      }
      CandNum = 0;
      
      for( Int modeIdx = 0; modeIdx < numModesAvailable; modeIdx++ )
      {
        UInt uiMode = modeIdx;

        predIntraLumaAng( pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
        
        // use hadamard transform here
        UInt uiSad = m_pcRdCost->calcHAD(g_bitDepthY, piOrg, uiStride, piPred, uiStride, uiWidth, uiHeight );
        
        UInt   iModeBits = xModeBitsIntra( pcCU, uiMode, uiPU, uiPartOffset, uiDepth, uiInitTrDepth );
        Double cost      = (Double)uiSad + (Double)iModeBits * m_pcRdCost->getSqrtLambda();
        
        CandNum += xUpdateCandList( uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList );
      }
    
#if FAST_UDI_USE_MPM
      Int uiPreds[3] = {-1, -1, -1};
      Int iMode = -1;
      Int numCand = pcCU->getIntraDirLumaPredictor( uiPartOffset, uiPreds, &iMode );
      if( iMode >= 0 )
      {
        numCand = iMode;
      }
      
      for( Int j=0; j < numCand; j++)
      {
        Bool mostProbableModeIncluded = false;
        Int mostProbableMode = uiPreds[j];
        
        for( Int i=0; i < numModesForFullRD; i++)
        {
          mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
        }
        if (!mostProbableModeIncluded)
        {
          uiRdModeList[numModesForFullRD++] = mostProbableMode;
        }
      }
#endif // FAST_UDI_USE_MPM
    }
    else
    {
      for( Int i=0; i < numModesForFullRD; i++)
      {
        uiRdModeList[i] = i;
      }
    }
    
    //===== check modes (using r-d costs) =====
#if HHI_RQT_INTRA_SPEEDUP_MOD
    UInt   uiSecondBestMode  = MAX_UINT;
    Double dSecondBestPUCost = MAX_DOUBLE;
#endif
    
    UInt    uiBestPUMode  = 0;
    UInt    uiBestPUDistY = 0;
    UInt    uiBestPUDistC = 0;
    Double  dBestPUCost   = MAX_DOUBLE;
    for( UInt uiMode = 0; uiMode < numModesForFullRD; uiMode++ )
    {
      // set luma prediction mode
      UInt uiOrgMode = uiRdModeList[uiMode];
      
      pcCU->setLumaIntraDirSubParts ( uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );
      
      // set context models
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
      
      // determine residual for partition
      UInt   uiPUDistY = 0;
      UInt   uiPUDistC = 0;
      Double dPUCost   = 0.0;
#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, true, dPUCost );
#else
      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, dPUCost );
#endif
      
      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
#if HHI_RQT_INTRA_SPEEDUP_MOD
        uiSecondBestMode  = uiBestPUMode;
        dSecondBestPUCost = dBestPUCost;
#endif
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        uiBestPUDistC = uiPUDistC;
        dBestPUCost   = dPUCost;
        
        xSetIntraResultQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv );
#if IT_RESIDUAL_FILE
        pcResiYuv->copyPartToPartLuma(pcIntraResiYuv,uiPartOffset, uiWidth, uiHeight);
#endif        
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[0], pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
      }
#if HHI_RQT_INTRA_SPEEDUP_MOD
      else if( dPUCost < dSecondBestPUCost )
      {
        uiSecondBestMode  = uiOrgMode;
        dSecondBestPUCost = dPUCost;
      }
#endif
    } // Mode loop
    
#if HHI_RQT_INTRA_SPEEDUP
#if HHI_RQT_INTRA_SPEEDUP_MOD
    for( UInt ui =0; ui < 2; ++ui )
#endif
    {
#if HHI_RQT_INTRA_SPEEDUP_MOD
      UInt uiOrgMode   = ui ? uiSecondBestMode  : uiBestPUMode;
      if( uiOrgMode == MAX_UINT )
      {
        break;
      }
#else
      UInt uiOrgMode = uiBestPUMode;
#endif
      
      pcCU->setLumaIntraDirSubParts ( uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );
      
      // set context models
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
      
      // determine residual for partition
      UInt   uiPUDistY = 0;
      UInt   uiPUDistC = 0;
      Double dPUCost   = 0.0;
      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, false, dPUCost );
      
      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        uiBestPUDistC = uiPUDistC;
        dBestPUCost   = dPUCost;
        
        xSetIntraResultQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv );
#if IT_RESIDUAL_FILE
        pcResiYuv->copyPartToPartLuma(pcIntraResiYuv,uiPartOffset, uiWidth, uiHeight);
#endif        
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[0], pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
      }
    } // Mode loop
#endif
    
    //--- update overall distortion ---
    uiOverallDistY += uiBestPUDistY;
    uiOverallDistC += uiBestPUDistC;
    
    //--- update transform index and cbf ---
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
    ::memcpy( pcCU->getTransformIdx()       + uiPartOffset, m_puhQTTempTrIdx,  uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, m_puhQTTempCbf[0], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, m_puhQTTempCbf[1], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, m_puhQTTempCbf[2], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, m_puhQTTempTransformSkipFlag[0], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, m_puhQTTempTransformSkipFlag[1], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, m_puhQTTempTransformSkipFlag[2], uiQPartNum * sizeof( UChar ) );
    //--- set reconstruction for next intra prediction blocks ---
    if( uiPU != uiNumPU - 1 )
    {
      Bool bSkipChroma  = false;
      Bool bChromaSame  = false;
      UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> ( pcCU->getDepth(0) + uiInitTrDepth ) ] + 2;
      if( !bLumaOnly && uiLog2TrSize == 2 )
      {
        assert( uiInitTrDepth  > 0 );
        bSkipChroma  = ( uiPU != 0 );
        bChromaSame  = true;
      }
      
      UInt    uiCompWidth   = pcCU->getWidth ( 0 ) >> uiInitTrDepth;
      UInt    uiCompHeight  = pcCU->getHeight( 0 ) >> uiInitTrDepth;
      UInt    uiZOrder      = pcCU->getZorderIdxInCU() + uiPartOffset;
      Pel*    piDes         = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
      UInt    uiDesStride   = pcCU->getPic()->getPicYuvRec()->getStride();
      Pel*    piSrc         = pcRecoYuv->getLumaAddr( uiPartOffset );
      UInt    uiSrcStride   = pcRecoYuv->getStride();
      for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
      if( !bLumaOnly && !bSkipChroma )
      {
        if( !bChromaSame )
        {
          uiCompWidth   >>= 1;
          uiCompHeight  >>= 1;
        }
        piDes         = pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder );
        uiDesStride   = pcCU->getPic()->getPicYuvRec()->getCStride();
        piSrc         = pcRecoYuv->getCbAddr( uiPartOffset );
        uiSrcStride   = pcRecoYuv->getCStride();
        for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
        {
          for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
          {
            piDes[ uiX ] = piSrc[ uiX ];
          }
        }
        piDes         = pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder );
        piSrc         = pcRecoYuv->getCrAddr( uiPartOffset );
        for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
        {
          for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
          {
            piDes[ uiX ] = piSrc[ uiX ];
          }
        }
      }
    }
    
    //=== update PU data ====
    pcCU->setLumaIntraDirSubParts     ( uiBestPUMode, uiPartOffset, uiDepth + uiInitTrDepth );
    pcCU->copyToPic                   ( uiDepth, uiPU, uiInitTrDepth );
  } // PU loop
  
  
  if( uiNumPU > 1 )
  { // set Cbf for all blocks
    UInt uiCombCbfY = 0;
    UInt uiCombCbfU = 0;
    UInt uiCombCbfV = 0;
    UInt uiPartIdx  = 0;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts )
    {
      uiCombCbfY |= pcCU->getCbf( uiPartIdx, TEXT_LUMA,     1 );
      uiCombCbfU |= pcCU->getCbf( uiPartIdx, TEXT_CHROMA_U, 1 );
      uiCombCbfV |= pcCU->getCbf( uiPartIdx, TEXT_CHROMA_V, 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++ )
    {
      pcCU->getCbf( TEXT_LUMA     )[ uiOffs ] |= uiCombCbfY;
      pcCU->getCbf( TEXT_CHROMA_U )[ uiOffs ] |= uiCombCbfU;
      pcCU->getCbf( TEXT_CHROMA_V )[ uiOffs ] |= uiCombCbfV;
    }
  }
  
  //===== reset context models =====
  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
  
  //===== set distortion (rate and r-d costs are determined later) =====
  ruiDistC                   = uiOverallDistC;
  pcCU->getTotalDistortion() = uiOverallDistY + uiOverallDistC;
}



Void 
TEncSearch::estIntraPredChromaQT( TComDataCU* pcCU, 
                                 TComYuv*    pcOrgYuv, 
                                 TComYuv*    pcPredYuv, 
                                 TComYuv*    pcResiYuv,
#if IT_RESIDUAL_FILE
                                 TComYuv*    pcIntraResiYuv,
#endif
                                 TComYuv*    pcRecoYuv,
                                 UInt        uiPreCalcDistC )
{
  UInt    uiDepth     = pcCU->getDepth(0);
  UInt    uiBestMode  = 0;
  UInt    uiBestDist  = 0;
  Double  dBestCost   = MAX_DOUBLE;
  
  //----- init mode list -----
  UInt  uiMinMode = 0;
  UInt  uiModeList[ NUM_CHROMA_MODE ];
  pcCU->getAllowedChromaDir( 0, uiModeList );
  UInt  uiMaxMode = NUM_CHROMA_MODE;

  //----- check chroma modes -----
  for( UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++ )
  {
    //----- restore context models -----
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
    
    //----- chroma coding -----
    UInt    uiDist = 0;
    pcCU->setChromIntraDirSubParts  ( uiModeList[uiMode], 0, uiDepth );
    xRecurIntraChromaCodingQT       ( pcCU,   0, 0, pcOrgYuv, pcPredYuv, pcResiYuv, uiDist );
    if( pcCU->getSlice()->getPPS()->getUseTransformSkip() )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
    }
    UInt    uiBits = xGetIntraBitsQT( pcCU,   0, 0, false, true, false );
    Double  dCost  = m_pcRdCost->calcRdCost( uiBits, uiDist );
    
    //----- compare -----
    if( dCost < dBestCost )
    {
      dBestCost   = dCost;
      uiBestDist  = uiDist;
      uiBestMode  = uiModeList[uiMode];
      UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 );
      xSetIntraResultChromaQT( pcCU, 0, 0, pcRecoYuv );
#if IT_RESIDUAL_FILE
      pcResiYuv->copyToPartChroma(pcIntraResiYuv,0);
#endif
      ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ), uiQPN * sizeof( UChar ) );
      ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ), uiQPN * sizeof( UChar ) );
      ::memcpy( m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip( TEXT_CHROMA_U ), uiQPN * sizeof( UChar ) );
      ::memcpy( m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip( TEXT_CHROMA_V ), uiQPN * sizeof( UChar ) );
    }
  }
  
  //----- set data -----
  UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 );
  ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ), m_puhQTTempCbf[1], uiQPN * sizeof( UChar ) );
  ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ), m_puhQTTempCbf[2], uiQPN * sizeof( UChar ) );
  ::memcpy( pcCU->getTransformSkip( TEXT_CHROMA_U ), m_puhQTTempTransformSkipFlag[1], uiQPN * sizeof( UChar ) );
  ::memcpy( pcCU->getTransformSkip( TEXT_CHROMA_V ), m_puhQTTempTransformSkipFlag[2], uiQPN * sizeof( UChar ) );
  pcCU->setChromIntraDirSubParts( uiBestMode, 0, uiDepth );
  pcCU->getTotalDistortion      () += uiBestDist - uiPreCalcDistC;
  
  //----- restore context models -----
  m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
}

/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param piOrg pointer to original sample arrays
 * \param piPCM pointer to PCM code arrays
 * \param piPred pointer to prediction signal arrays
 * \param piResi pointer to residual signal arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of the original/prediction/residual sample arrays
 * \param uiWidth block width
 * \param uiHeight block height
 * \param ttText texture component type
 * \returns Void
 */
Void TEncSearch::xEncPCM (TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Pel* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType eText )
{
  UInt uiX, uiY;
  UInt uiReconStride;
  Pel* pOrg  = piOrg;
  Pel* pPCM  = piPCM;
  Pel* pPred = piPred;
  Pel* pResi = piResi;
  Pel* pReco = piReco;
  Pel* pRecoPic;
  Int shiftPcm;

  if( eText == TEXT_LUMA)
  {
    uiReconStride = pcCU->getPic()->getPicYuvRec()->getStride();
    pRecoPic      = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    shiftPcm = g_bitDepthY - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
  }
  else
  {
    uiReconStride = pcCU->getPic()->getPicYuvRec()->getCStride();

    if( eText == TEXT_CHROMA_U )
    {
      pRecoPic = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    }
    else
    {
      pRecoPic = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    }
    shiftPcm = g_bitDepthC - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
  }

  // Reset pred and residual
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pPred[uiX] = 0;
      pResi[uiX] = 0;
    }
    pPred += uiStride;
    pResi += uiStride;
  }

  // Encode
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pPCM[uiX] = pOrg[uiX]>> shiftPcm;
    }
    pPCM += uiWidth;
    pOrg += uiStride;
  }

  pPCM  = piPCM;

  // Reconstruction
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pReco   [uiX] = pPCM[uiX]<< shiftPcm;
      pRecoPic[uiX] = pReco[uiX];
    }
    pPCM += uiWidth;
    pReco += uiStride;
    pRecoPic += uiReconStride;
  }
}

/**  Function for PCM mode estimation.
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \returns Void
 */
Void TEncSearch::IPCMSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv )
{
  UInt   uiDepth        = pcCU->getDepth(0);
  UInt   uiWidth        = pcCU->getWidth(0);
  UInt   uiHeight       = pcCU->getHeight(0);
  UInt   uiStride       = rpcPredYuv->getStride();
  UInt   uiStrideC      = rpcPredYuv->getCStride();
  UInt   uiWidthC       = uiWidth  >> 1;
  UInt   uiHeightC      = uiHeight >> 1;
  UInt   uiDistortion = 0;
  UInt   uiBits;

  Double dCost;

  Pel*    pOrig;
  Pel*    pResi;
  Pel*    pReco;
  Pel*    pPred;
  Pel*    pPCM;

  UInt uiAbsPartIdx = 0;

  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  // Luminance
  pOrig    = pcOrgYuv->getLumaAddr(0, uiWidth);
  pResi    = rpcResiYuv->getLumaAddr(0, uiWidth);
  pPred    = rpcPredYuv->getLumaAddr(0, uiWidth);
  pReco    = rpcRecoYuv->getLumaAddr(0, uiWidth);
  pPCM     = pcCU->getPCMSampleY() + uiLumaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStride, uiWidth, uiHeight, TEXT_LUMA );

  // Chroma U
  pOrig    = pcOrgYuv->getCbAddr();
  pResi    = rpcResiYuv->getCbAddr();
  pPred    = rpcPredYuv->getCbAddr();
  pReco    = rpcRecoYuv->getCbAddr();
  pPCM     = pcCU->getPCMSampleCb() + uiChromaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_U );

  // Chroma V
  pOrig    = pcOrgYuv->getCrAddr();
  pResi    = rpcResiYuv->getCrAddr();
  pPred    = rpcPredYuv->getCrAddr();
  pReco    = rpcRecoYuv->getCrAddr();
  pPCM     = pcCU->getPCMSampleCr() + uiChromaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_V );

  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiDepth, uiAbsPartIdx, true, false);
  uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

  dCost = m_pcRdCost->calcRdCost( uiBits, uiDistortion );

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  pcCU->getTotalBits()       = uiBits;
  pcCU->getTotalCost()       = dCost;
  pcCU->getTotalDistortion() = uiDistortion;

  pcCU->copyToPic(uiDepth, 0, 0);
}

Void TEncSearch::xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiErr, Bool bHadamard
#if IT_GT
		, Bool useGT
#endif
		)
{
  motionCompensation( pcCU, &m_tmpYuvPred,
#if IT_GT
		  useGT,
#endif
				REF_PIC_LIST_X, iPartIdx );

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );

  DistParam cDistParam;

  cDistParam.bApplyWeight = false;

  m_pcRdCost->setDistParam( cDistParam, g_bitDepthY,
                            pcYuvOrg->getLumaAddr( uiAbsPartIdx ), pcYuvOrg->getStride(), 
                            m_tmpYuvPred .getLumaAddr( uiAbsPartIdx ), m_tmpYuvPred .getStride(), 
                            iWidth, iHeight, m_pcEncCfg->getUseHADME() );
  ruiErr = cDistParam.DistFunc( &cDistParam );
}

/** estimation of best merge coding
 * \param pcCU
 * \param pcYuvOrg
 * \param iPUIdx
 * \param uiInterDir
 * \param pacMvField
 * \param uiMergeIndex
 * \param ruiCost
 * \param ruiBits
 * \param puhNeighCands
 * \param bValid 
 * \returns Void
 */
Void TEncSearch::xMergeEstimation(  TComDataCU*   pcCU,
                                    TComYuv*      pcYuvOrg,
                                    Int           iPUIdx,
                                    UInt&         uiInterDir,
                                    TComMvField*  pacMvField,
#if IT_GT
									TComMvField*    pacGT0Field,
									TComMvField*    pacGT1Field,
									TComMvField*    pacGT2Field,
									TComMvField*    pacGT3Field,
#endif
                                    UInt&         uiMergeIndex,
                                    UInt&         ruiCost,
                                    TComMvField*  cMvFieldNeighbours,
#if IT_GT
								  TComMvField* cGT0FieldNeighbours,
								  TComMvField* cGT1FieldNeighbours,
								  TComMvField* cGT2FieldNeighbours,
								  TComMvField* cGT3FieldNeighbours,
#endif
                                    UChar*        uhInterDirNeighbours,
                                    Int&          numValidMergeCand
#if IT_HOLOSS
                                    ,Bool&        bValMerge
#endif
#if IT_GT
								   , Bool bUseGT
#endif
                                 )
{
  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0; 

  pcCU->getPartIndexAndSize( iPUIdx, uiAbsPartIdx, iWidth, iHeight );
#if IT_HOLOSS
  Int riOffsetX, riOffsetY;
  Bool isFirstRow, isFirstCol;
  pcCU->getPartOffset(iPUIdx, uiAbsPartIdx, riOffsetX, riOffsetY, isFirstRow, isFirstCol );
#endif
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  PartSize partSize = pcCU->getPartitionSize( 0 );
  if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && pcCU->getWidth( 0 ) <= 8 )
  {
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
    if ( iPUIdx == 0 )
    {
      pcCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
    }
    pcCU->setPartSizeSubParts( partSize, 0, uiDepth );
  }
  else
  {
    pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
  }
  xRestrictBipredMergeCand( pcCU, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );

  ruiCost = MAX_UINT;
  for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
  {
    UInt uiCostCand = MAX_UINT;
    UInt uiBitsCand = 0;
    
    PartSize ePartSize = pcCU->getPartitionSize( 0 );

    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );

#if IT_HOLOSS
// Before anything, we have to guaratee that the vector candidate points to the previous coded area:
      // URGENTE: revisar os breaks e continue!!!!!!
      if ( ( pcCU->getSlice()->isIntraSS() || pcCU->getSlice()->isInterPSS() )
#if IT_SCALABLE_V1
            && ( !pcCU->getSlice()->isScalableSlice() )
#endif
        )
      {
        Int    iRefIdx = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx(uiAbsPartIdx);
        // SS reference is in the last position of the list 0:
        if ( (iRefIdx != -1) && ( iRefIdx == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1 ) )
        {
          TComMv cSSMv = pcCU->getCUMvField( REF_PIC_LIST_0 )->getMv(uiAbsPartIdx);
          pcCU->clipMv(cSSMv);
          TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, iRefIdx )->getPicYuvRec();
          if ( !m_pcRdCost->isValidPattern( pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx ), pcPicYuvRef->getStride(), cSSMv, iWidth, iHeight) )
          {
            uiCostCand = MAX_UINT;
            continue;
          }
        } // refIdx
      }
      bValMerge = true;
#endif

#if !IT_GT
    xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
#else
    xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME(), bUseGT );
#endif
    uiBitsCand = uiMergeCand + 1;
    if (uiMergeCand == m_pcEncCfg->getMaxNumMergeCand() -1)
    {
      uiBitsCand--;
    }
    uiCostCand = uiCostCand + m_pcRdCost->getCost( uiBitsCand );
    if ( uiCostCand < ruiCost )
    {
      ruiCost = uiCostCand;
      pacMvField[0] = cMvFieldNeighbours[0 + 2*uiMergeCand];
      pacMvField[1] = cMvFieldNeighbours[1 + 2*uiMergeCand];
      uiInterDir = uhInterDirNeighbours[uiMergeCand];
      uiMergeIndex = uiMergeCand;
    }
  }
}

/** convert bi-pred merge candidates to uni-pred
 * \param pcCU
 * \param puIdx
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param numValidMergeCand
 * \returns Void
 */
Void TEncSearch::xRestrictBipredMergeCand( TComDataCU* pcCU, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, Int numValidMergeCand )
{
  if ( pcCU->isBipredRestriction(puIdx) )
  {
    for( UInt mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand )
    {
      if ( interDirNeighbours[mergeCand] == 3 )
      {
        interDirNeighbours[mergeCand] = 1;
        mvFieldNeighbours[(mergeCand << 1) + 1].setMvField(TComMv(0,0), -1);
      }
    }
  }
}

/** search of the best candidate for inter prediction
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \param bUseRes
 * \returns Void
 */

Void TEncSearch::predInterSearch( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*&   rpcPredYuv,
                                  TComYuv*&   rpcResiYuv,
                                  TComYuv*&   rpcRecoYuv,
#if IT_HOLOSS
                                  Bool&       bNotValTmpCU,     
#endif
                                  Bool        bUseRes
#if AMP_MRG
                                 ,Bool       bUseMRG
#endif
                                )
{
  m_acYuvPred[0].clear();
  m_acYuvPred[1].clear();
  m_cYuvPredTemp.clear();
  rpcPredYuv->clear();
  
  if ( !bUseRes )
  {
    rpcResiYuv->clear();
  }
  
  rpcRecoYuv->clear();
  
  TComMv        cMvSrchRngLT;
  TComMv        cMvSrchRngRB;
  
  TComMv        cMvZero;
  TComMv        TempMv; //kolya
  
  TComMv        cMv[2];
  TComMv        cMvBi[2];
  TComMv        cMvTemp[2][33];
#if IT_GT
  Bool 			useGT_ME = true;
  Bool 			useGT_MRG = false;
  Bool			useGT = true;
  TComMv        cGT0[2];
  TComMv        cGT0Bi[2];
  TComMv        cGT0Temp[2][33];
  TComMv        cGT1[2];
  TComMv        cGT1Bi[2];
  TComMv        cGT1Temp[2][33];
  TComMv        cGT2[2];
  TComMv        cGT2Bi[2];
  TComMv        cGT2Temp[2][33];
  TComMv        cGT3[2];
  TComMv        cGT3Bi[2];
  TComMv        cGT3Temp[2][33];
  Bool 			gtFlag[2];
  Bool 			gtFlagBi[2];
  Bool 			gtFlagTemp[2][33];
#endif
  
  Int           iNumPart    = pcCU->getNumPartitions();
#if IT_HOLOSS
  Int           iNumPredDir = ( pcCU->getSlice()->isInterP()||pcCU->getSlice()->isIntraSS()||pcCU->getSlice()->isInterPSS() ) ? 1 : 2;
#else
  Int           iNumPredDir = pcCU->getSlice()->isInterP() ? 1 : 2;
#endif
  
  TComMv        cMvPred[2][33];
#if IT_GT
  TComMv        cGT0Pred[2][33];
  TComMv        cGT1Pred[2][33];
  TComMv        cGT2Pred[2][33];
  TComMv        cGT3Pred[2][33];
#endif
  
  TComMv        cMvPredBi[2][33];
#if IT_GT
  TComMv        cGT0PredBi[2][33];
  TComMv        cGT1PredBi[2][33];
  TComMv        cGT2PredBi[2][33];
  TComMv        cGT3PredBi[2][33];
#endif
  Int           aaiMvpIdxBi[2][33];
  
  Int           aaiMvpIdx[2][33];
  Int           aaiMvpNum[2][33];
  
  AMVPInfo aacAMVPInfo[2][33];
  
  Int           iRefIdx[2]={0,0}; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
#if IT_HOLOSS
  Int           iRefIdxIT[2]={-1,-1}; // Since we can't change the initial value, there is a new variable to test if is valid reference or not;
#endif
  Int           iRefIdxBi[2];
  
  UInt          uiPartAddr;
  Int           iRoiWidth, iRoiHeight;
  
  UInt          uiMbBits[3] = {1, 1, 0};
  
  UInt          uiLastMode = 0;
  Int           iRefStart, iRefEnd;
  
  PartSize      ePartSize = pcCU->getPartitionSize( 0 );

  Int           bestBiPRefIdxL1 = 0;
  Int           bestBiPMvpL1 = 0;
  UInt          biPDistTemp = MAX_INT;

#if ZERO_MVD_EST
  Int           aiZeroMvdMvpIdx[2] = {-1, -1};
  Int           aiZeroMvdRefIdx[2] = {0, 0};
  Int           iZeroMvdDir = -1;
#endif

  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
#if IT_GT
  TComMvField cGT0FieldNeighbours[MRG_MAX_NUM_CANDS << 1];
  TComMvField cGT1FieldNeighbours[MRG_MAX_NUM_CANDS << 1];
  TComMvField cGT2FieldNeighbours[MRG_MAX_NUM_CANDS << 1];
  TComMvField cGT3FieldNeighbours[MRG_MAX_NUM_CANDS << 1];
#endif
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0 ;

  for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )
  {
#if IT_GT
	  useGT_ME = true;
	  useGT_MRG = false;
	  useGT = true;
#endif
    UInt          uiCost[2] = { MAX_UINT, MAX_UINT };
    UInt          uiCostBi  =   MAX_UINT;
    UInt          uiCostTemp;
    
    UInt          uiBits[3];
    UInt          uiBitsTemp;
#if ZERO_MVD_EST
    UInt          uiZeroMvdCost = MAX_UINT;
    UInt          uiZeroMvdCostTemp;
    UInt          uiZeroMvdBitsTemp;
    UInt          uiZeroMvdDistTemp = MAX_UINT;
    UInt          auiZeroMvdBits[3];
#endif
    UInt          bestBiPDist = MAX_INT;

    UInt          uiCostTempL0[MAX_NUM_REF];
    for (Int iNumRef=0; iNumRef < MAX_NUM_REF; iNumRef++)
    {
      uiCostTempL0[iNumRef] = MAX_UINT;
    }
    UInt          uiBitsTempL0[MAX_NUM_REF];

    TComMv        mvValidList1;
    Int           refIdxValidList1 = 0;
    UInt          bitsValidList1 = MAX_UINT;
    UInt          costValidList1 = MAX_UINT;
#if IT_HOLOSS
    Bool          bNotValL0[IT_MAXREF];
    for (Int iVal = 0; iVal < IT_MAXREF;iVal++)  bNotValL0[iVal]=false;
#endif

#if IT_HOLOSS
    xGetBlkBits( ePartSize,( pcCU->getSlice()->isInterP()||pcCU->getSlice()->isIntraSS()||pcCU->getSlice()->isInterPSS() ), iPartIdx, uiLastMode, uiMbBits);
#else
    xGetBlkBits( ePartSize, pcCU->getSlice()->isInterP(), iPartIdx, uiLastMode, uiMbBits);
#endif
    
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
#if IT_GT_CU_SIZE_LIMIT > 0
   Int power = 5 - pcCU->getDepth(uiPartAddr);
   Int size_CU = 2 << power;
   useGT_ME = ( size_CU > IT_GT_CU_SIZE_LIMIT );
   useGT = useGT_ME;
#endif
    
#if AMP_MRG
    Bool bTestNormalMC = true;
    
    if ( bUseMRG && pcCU->getWidth( 0 ) > 8 && iNumPart == 2 )
    {
      bTestNormalMC = false;
    }
    
    if (bTestNormalMC)
    {
#endif

    //  Uni-directional prediction
    for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
    {
      RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      
      for ( Int iRefIdxTemp = 0; iRefIdxTemp < pcCU->getSlice()->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
      {
        uiBitsTemp = uiMbBits[iRefList];
        if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
        {
          uiBitsTemp += iRefIdxTemp+1;
          if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
        }
#if ZERO_MVD_EST
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp, &uiZeroMvdDistTemp);
#else
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp);
#endif
        aaiMvpIdx[iRefList][iRefIdxTemp] = pcCU->getMVPIdx(eRefPicList, uiPartAddr);
        aaiMvpNum[iRefList][iRefIdxTemp] = pcCU->getMVPNum(eRefPicList, uiPartAddr);
        
        if(pcCU->getSlice()->getMvdL1ZeroFlag() && iRefList==1 && biPDistTemp < bestBiPDist)
        {
          bestBiPDist = biPDistTemp;
          bestBiPMvpL1 = aaiMvpIdx[iRefList][iRefIdxTemp];
          bestBiPRefIdxL1 = iRefIdxTemp;
        }

        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
#if ZERO_MVD_EST
        if ( iRefList == 0 || pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp ) < 0 )
        {
          uiZeroMvdBitsTemp = uiBitsTemp;
          uiZeroMvdBitsTemp += 2; //zero mvd bits

          m_pcRdCost->getMotionCost( 1, 0 );
          uiZeroMvdCostTemp = uiZeroMvdDistTemp + m_pcRdCost->getCost(uiZeroMvdBitsTemp);

          if (uiZeroMvdCostTemp < uiZeroMvdCost)
          {
            uiZeroMvdCost = uiZeroMvdCostTemp;
            iZeroMvdDir = iRefList + 1;
            aiZeroMvdRefIdx[iRefList] = iRefIdxTemp;
            aiZeroMvdMvpIdx[iRefList] = aaiMvpIdx[iRefList][iRefIdxTemp];
            auiZeroMvdBits[iRefList] = uiZeroMvdBitsTemp;
          }          
        }
#endif
        
#if GPB_SIMPLE_UNI
        if ( iRefList == 1 )    // list 1
        {
          if ( pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp ) >= 0 )
          {
            cMvTemp[1][iRefIdxTemp] = cMvTemp[0][pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )];
            uiCostTemp = uiCostTempL0[pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )];
            /*first subtract the bit-rate part of the cost of the other list*/
            uiCostTemp -= m_pcRdCost->getCost( uiBitsTempL0[pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )] );
            /*correct the bit-rate part of the current ref*/
            m_pcRdCost->setPredictor  ( cMvPred[iRefList][iRefIdxTemp] );
            uiBitsTemp += m_pcRdCost->getBits( cMvTemp[1][iRefIdxTemp].getHor(), cMvTemp[1][iRefIdxTemp].getVer() );
            /*calculate the correct cost*/
            uiCostTemp += m_pcRdCost->getCost( uiBitsTemp );
          }
          else
          {
#if IT_HOLOSS
#if !IT_GT
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, bNotValL0[iRefIdxTemp] );
#else /// NOT USED YET
            Bool useTools = useGT && useGT_ME;
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp,bNotValL0[iRefIdxTemp],
            		useTools, cGT0Temp[iRefList][iRefIdxTemp], cGT1Temp[iRefList][iRefIdxTemp], cGT2Temp[iRefList][iRefIdxTemp], cGT3Temp[iRefList][iRefIdxTemp], gtFlagTemp[iRefList][iRefIdxTemp] );
#endif
#else
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
#endif
          }
        }
        else
        {
#if IT_HOLOSS
#if !IT_GT
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, bNotValL0[iRefIdxTemp] );
#else // USED CODE : UNI PRED ME W GT
            Bool useTools = useGT && useGT_ME;
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp,bNotValL0[iRefIdxTemp],
            		useTools, cGT0Temp[iRefList][iRefIdxTemp], cGT1Temp[iRefList][iRefIdxTemp], cGT2Temp[iRefList][iRefIdxTemp], cGT3Temp[iRefList][iRefIdxTemp], gtFlagTemp[iRefList][iRefIdxTemp] );
      // GUARDA GTS EM GTXTEMP[REF_LIST_0][0]
#endif
#else
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
#endif
        }
#else // GPB_SIMPLE_UNI
        xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
#endif // GPB_SIMPLE_UNI
        xCopyAMVPInfo(pcCU->getCUMvField(eRefPicList)->getAMVPInfo(), &aacAMVPInfo[iRefList][iRefIdxTemp]); // must always be done ( also when AMVP_MODE = AM_NONE )
        xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPred[iRefList][iRefIdxTemp], aaiMvpIdx[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

        if ( iRefList == 0 )
        {
          uiCostTempL0[iRefIdxTemp] = uiCostTemp;
          uiBitsTempL0[iRefIdxTemp] = uiBitsTemp;
        }
        if ( uiCostTemp < uiCost[iRefList] )
        {
          uiCost[iRefList] = uiCostTemp;
          uiBits[iRefList] = uiBitsTemp; // storing for bi-prediction

          // set motion
          cMv[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
#if IT_GT
          // SALVA GTS REF_LIST_0
          cGT0[iRefList]     = cGT0Temp[iRefList][iRefIdxTemp];
          cGT1[iRefList]     = cGT1Temp[iRefList][iRefIdxTemp];
          cGT2[iRefList]     = cGT2Temp[iRefList][iRefIdxTemp];
          cGT3[iRefList]     = cGT3Temp[iRefList][iRefIdxTemp];
          gtFlag[iRefList]	 = gtFlagTemp[iRefList][iRefIdxTemp];
#endif

#if IT_HOLOSS
          iRefIdxIT[iRefList] = iRefIdxTemp; // to guarantee reliable value in our tests
#endif
        }

        if ( iRefList == 1 && uiCostTemp < costValidList1 && pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp ) < 0 )
        {
          costValidList1 = uiCostTemp;
          bitsValidList1 = uiBitsTemp;

          // set motion
          mvValidList1     = cMvTemp[iRefList][iRefIdxTemp];
          refIdxValidList1 = iRefIdxTemp;
#if IT_HOLOSS
          iRefIdxIT[iRefList] = iRefIdxTemp; // to guarantee reliable value in our tests
#endif
        } // if uiCostTemp
      } // for iRefIdxTemp
    } // for iRefList
    //  Bi-directional prediction
    if ( (pcCU->getSlice()->isInterB()) && (pcCU->isBipredRestriction(iPartIdx) == false) )
    {
      // NOT USED YET
      cMvBi[0] = cMv[0];            cMvBi[1] = cMv[1];
      iRefIdxBi[0] = iRefIdx[0];    iRefIdxBi[1] = iRefIdx[1];
#if IT_GT
//      cGT0Bi[0] = cGT0[0];            cGT0Bi[1] = cGT0[1];
//      cGT1Bi[0] = cGT1[0];            cGT3Bi[1] = cGT1[1];
//      cGT2Bi[0] = cGT2[0];            cGT2Bi[1] = cGT2[1];
//      cGT3Bi[0] = cGT3[0];            cGT1Bi[1] = cGT3[1];
//      gtFlagBi[0] = gtFlag[0];		  gtFlagBi[1] = gtFlag[1];
#endif
      
      ::memcpy(cMvPredBi, cMvPred, sizeof(cMvPred));
      ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));
      
      UInt uiMotBits[2];

      if(pcCU->getSlice()->getMvdL1ZeroFlag())
      {
        xCopyAMVPInfo(&aacAMVPInfo[1][bestBiPRefIdxL1], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
        pcCU->setMVPIdxSubParts( bestBiPMvpL1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        aaiMvpIdxBi[1][bestBiPRefIdxL1] = bestBiPMvpL1;
        cMvPredBi[1][bestBiPRefIdxL1]   = pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo()->m_acMvCand[bestBiPMvpL1];

        cMvBi[1] = cMvPredBi[1][bestBiPRefIdxL1];
        iRefIdxBi[1] = bestBiPRefIdxL1;
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        TComYuv* pcYuvPred = &m_acYuvPred[1];
        motionCompensation( pcCU, pcYuvPred,
#if IT_GT
        		useGT && useGT_ME,
#endif
        		REF_PIC_LIST_1, iPartIdx );

        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiMbBits[1];

        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1 )
        {
          uiMotBits[1] += bestBiPRefIdxL1+1;
          if ( bestBiPRefIdxL1 == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1 ) uiMotBits[1]--;
        }

        uiMotBits[1] += m_auiMVPIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][AMVP_MAX_NUM_CANDS];

        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

        cMvTemp[1][bestBiPRefIdxL1] = cMvBi[1];
      }
      else
      {
        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiBits[1] - uiMbBits[1];
        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];
      }

      // 4-times iteration (default)
      Int iNumIter = 4;
      
      // fast encoder setting: only one iteration
      if ( m_pcEncCfg->getUseFastEnc() || pcCU->getSlice()->getMvdL1ZeroFlag())
      {
        iNumIter = 1;
      }
      
      for ( Int iIter = 0; iIter < iNumIter; iIter++ )
      {
        
        Int         iRefList    = iIter % 2;
        if ( m_pcEncCfg->getUseFastEnc() )
        {
          if( uiCost[0] <= uiCost[1] )
          {
            iRefList = 1;
          }
          else
          {
            iRefList = 0;
          }
        }
        else if ( iIter == 0 )
        {
          iRefList = 0;
        }
        if ( iIter == 0 && !pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          pcCU->getCUMvField(RefPicList(1-iRefList))->setAllMv( cMv[1-iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField(RefPicList(1-iRefList))->setAllRefIdx( iRefIdx[1-iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
          TComYuv*  pcYuvPred = &m_acYuvPred[1-iRefList];
          motionCompensation ( pcCU, pcYuvPred,
#if IT_GT
        		  useGT && useGT_ME,
#endif
				RefPicList(1-iRefList), iPartIdx );
        }
        RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

        if(pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          iRefList = 0;
          eRefPicList = REF_PIC_LIST_0;
        }

        Bool bChanged = false;
        
        iRefStart = 0;
        iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;
        
        for ( Int iRefIdxTemp = iRefStart; iRefIdxTemp <= iRefEnd; iRefIdxTemp++ )
        {
          uiBitsTemp = uiMbBits[2] + uiMotBits[1-iRefList];
          if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
          {
            uiBitsTemp += iRefIdxTemp+1;
            if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
          }
          uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
          // call ME
#if IT_HOLOSS
#if !IT_GT
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, bNotValL0[iRefIdxTemp], true );
#else
          Bool useTools = useGT && useGT_ME;
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, bNotValL0[iRefIdxTemp],
        		  useTools, cGT0Temp[iRefList][iRefIdxTemp], cGT1Temp[iRefList][iRefIdxTemp], cGT2Temp[iRefList][iRefIdxTemp], cGT3Temp[iRefList][iRefIdxTemp], gtFlagTemp[iRefList][iRefIdxTemp], true );
#endif
#else
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, true );
#endif
          xCopyAMVPInfo(&aacAMVPInfo[iRefList][iRefIdxTemp], pcCU->getCUMvField(eRefPicList)->getAMVPInfo());
          xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPredBi[iRefList][iRefIdxTemp], aaiMvpIdxBi[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

          if ( uiCostTemp < uiCostBi )
          {
            bChanged = true;
            
            cMvBi[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
            iRefIdxBi[iRefList] = iRefIdxTemp;
            
            uiCostBi            = uiCostTemp;
            uiMotBits[iRefList] = uiBitsTemp - uiMbBits[2] - uiMotBits[1-iRefList];
            uiBits[2]           = uiBitsTemp;
            
            if(iNumIter!=1)
            {
              //  Set motion
              pcCU->getCUMvField( eRefPicList )->setAllMv( cMvBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
              pcCU->getCUMvField( eRefPicList )->setAllRefIdx( iRefIdxBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );

              TComYuv* pcYuvPred = &m_acYuvPred[iRefList];
              motionCompensation( pcCU, pcYuvPred,
#if IT_GT
            		  useGT && useGT_ME,
#endif
				eRefPicList, iPartIdx );
            }
          }
        } // for loop-iRefIdxTemp
        
        if ( !bChanged )
        {
          if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] )
          {
            xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
            xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
            if(!pcCU->getSlice()->getMvdL1ZeroFlag())
            {
              xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
              xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
            }
          }
          break;
        }
      } // for loop-iter
    } // if (B_SLICE)
#if ZERO_MVD_EST
    if ( (pcCU->getSlice()->isInterB()) && (pcCU->isBipredRestriction(iPartIdx) == false) )
    {
      m_pcRdCost->getMotionCost( 1, 0 );

      for ( Int iL0RefIdxTemp = 0; iL0RefIdxTemp <= pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1; iL0RefIdxTemp++ )
      for ( Int iL1RefIdxTemp = 0; iL1RefIdxTemp <= pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1; iL1RefIdxTemp++ )
      {
        UInt uiRefIdxBitsTemp = 0;
        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 1 )
        {
          uiRefIdxBitsTemp += iL0RefIdxTemp+1;
          if ( iL0RefIdxTemp == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1 ) uiRefIdxBitsTemp--;
        }
        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1 )
        {
          uiRefIdxBitsTemp += iL1RefIdxTemp+1;
          if ( iL1RefIdxTemp == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1 ) uiRefIdxBitsTemp--;
        }

        Int iL0MVPIdx = 0;
        Int iL1MVPIdx = 0;

        for (iL0MVPIdx = 0; iL0MVPIdx < aaiMvpNum[0][iL0RefIdxTemp]; iL0MVPIdx++)
        {
          for (iL1MVPIdx = 0; iL1MVPIdx < aaiMvpNum[1][iL1RefIdxTemp]; iL1MVPIdx++)
          {
            uiZeroMvdBitsTemp = uiRefIdxBitsTemp;
            uiZeroMvdBitsTemp += uiMbBits[2];
            uiZeroMvdBitsTemp += m_auiMVPIdxCost[iL0MVPIdx][aaiMvpNum[0][iL0RefIdxTemp]] + m_auiMVPIdxCost[iL1MVPIdx][aaiMvpNum[1][iL1RefIdxTemp]];
            uiZeroMvdBitsTemp += 4; //zero mvd for both directions
            pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( aacAMVPInfo[0][iL0RefIdxTemp].m_acMvCand[iL0MVPIdx], iL0RefIdxTemp, ePartSize, uiPartAddr, iPartIdx, 0 );
            pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( aacAMVPInfo[1][iL1RefIdxTemp].m_acMvCand[iL1MVPIdx], iL1RefIdxTemp, ePartSize, uiPartAddr, iPartIdx, 0 );
  
            xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiZeroMvdDistTemp, m_pcEncCfg->getUseHADME() );
            uiZeroMvdCostTemp = uiZeroMvdDistTemp + m_pcRdCost->getCost( uiZeroMvdBitsTemp );
            if (uiZeroMvdCostTemp < uiZeroMvdCost)
            {
              uiZeroMvdCost = uiZeroMvdCostTemp;
              iZeroMvdDir = 3;
              aiZeroMvdMvpIdx[0] = iL0MVPIdx;
              aiZeroMvdMvpIdx[1] = iL1MVPIdx;
              aiZeroMvdRefIdx[0] = iL0RefIdxTemp;
              aiZeroMvdRefIdx[1] = iL1RefIdxTemp;
              auiZeroMvdBits[2] = uiZeroMvdBitsTemp;
            }
          }
        }
      }
    }
#endif

#if AMP_MRG
    } //end if bTestNormalMC
#endif
    //  Clear Motion Field
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
    pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->setGTFlag(iPartIdx, false);
    pcCU->setGTFlagSubParts(false, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
#endif
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    
    UInt uiMEBits = 0;
    // Set Motion Field_
    cMv[1] = mvValidList1;
#if IT_GT
    // ??
#endif
    iRefIdx[1] = refIdxValidList1;
    uiBits[1] = bitsValidList1;
    uiCost[1] = costValidList1;
#if AMP_MRG
    if (bTestNormalMC)
    {
#endif
#if ZERO_MVD_EST
    if (uiZeroMvdCost <= uiCostBi && uiZeroMvdCost <= uiCost[0] && uiZeroMvdCost <= uiCost[1])
    {
      if (iZeroMvdDir == 3)
      {
        uiLastMode = 2;

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( aacAMVPInfo[0][aiZeroMvdRefIdx[0]].m_acMvCand[aiZeroMvdMvpIdx[0]], aiZeroMvdRefIdx[0], ePartSize, uiPartAddr, iPartIdx, 0 );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( aacAMVPInfo[1][aiZeroMvdRefIdx[1]].m_acMvCand[aiZeroMvdMvpIdx[1]], aiZeroMvdRefIdx[1], ePartSize, uiPartAddr, iPartIdx, 0 );
  
        pcCU->setInterDirSubParts( 3, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[0], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[0][aiZeroMvdRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[1], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[1][aiZeroMvdRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[2];
      }
      else if (iZeroMvdDir == 1)
      {        
        uiLastMode = 0;

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( aacAMVPInfo[0][aiZeroMvdRefIdx[0]].m_acMvCand[aiZeroMvdMvpIdx[0]], aiZeroMvdRefIdx[0], ePartSize, uiPartAddr, iPartIdx, 0 );

        pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[0], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[0][aiZeroMvdRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[0];
      }
      else if (iZeroMvdDir == 2)
      {
        uiLastMode = 1;

        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( aacAMVPInfo[1][aiZeroMvdRefIdx[1]].m_acMvCand[aiZeroMvdMvpIdx[1]], aiZeroMvdRefIdx[1], ePartSize, uiPartAddr, iPartIdx, 0 );

        pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[1], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[1][aiZeroMvdRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[1];
      }
      else
      {
        assert(0);
      }
    }
    else
#endif
#if IT_HOLOSS
    if ( ( pcCU->getSlice()->isIntraSS() || pcCU->getSlice()->isInterPSS() )
#if IT_SCALABLE_V1
         && !pcCU->getSlice()->isScalableSlice()
#endif
         )
    {
      // Refresh notValid info with the chosen reference in List 0
      if ( iRefIdxIT[0] != -1 )  bNotValTmpCU = bNotValL0[iRefIdx[0]]; // there is more than one reference (SS and temporal)
      else                       bNotValTmpCU = bNotValL0[pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1]; // there isn't temporal reference
    }
    
    if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] && !bNotValTmpCU )
#else 
    if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
#endif
    {
      uiLastMode = 2;
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMvBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
      // NOT USED YET
//      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMv( cGT0Bi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMv( cGT0Bi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMv( cGT1Bi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMv( cGT1Bi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMv( cGT2Bi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMv( cGT2Bi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMv( cGT3Bi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMv( cGT3Bi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      //pcCU->setGTFlag(iPartIdx, gtFlagBi[0]);
//      pcCU->setGTFlagSubParts(gtFlagBi[0], uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
#if IT_GT_CU_SIZE_LIMIT > 0
      if(!(size_CU > IT_GT_CU_SIZE_LIMIT)){
    	  pcCU->setGTFlag(iPartIdx, false);
    	  useGT_ME = false;
    	  useGT = false;
      }
#endif
#endif
      
      TempMv = cMvBi[0] - cMvPredBi[0][iRefIdxBi[0]];
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cMvBi[1] - cMvPredBi[1][iRefIdxBi[1]];
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
//      TempMv = cGT0Bi[0] - cGT0PredBi[0][iRefIdxBi[0]];
//      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT0Bi[1] - cGT0PredBi[1][iRefIdxBi[1]];
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT1Bi[0] - cGT1PredBi[0][iRefIdxBi[0]];
//      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT1Bi[1] - cGT1PredBi[1][iRefIdxBi[1]];
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT1Bi[0] - cGT2PredBi[0][iRefIdxBi[0]];
//      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT2Bi[1] - cGT2PredBi[1][iRefIdxBi[1]];
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT3Bi[0] - cGT3PredBi[0][iRefIdxBi[0]];
//      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT3Bi[1] - cGT3PredBi[1][iRefIdxBi[1]];
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//     // pcCU->setGTFlag(iPartIdx, gtFlagBi[1]);
//      pcCU->setGTFlagSubParts(gtFlagBi[1], uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
#endif
      
      pcCU->setInterDirSubParts( 3, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[2];
    }
    else if ( uiCost[0] <= uiCost[1] ) // L0 > L1
    {
#if IT_HOLOSS
      if( (bNotValTmpCU)&&(iRefIdx[0] < 0) )
      {
        iRefIdx[0] = 0; // Just to avoid bug in array index (I think doesnt happen anymore)
      }
#endif
      uiLastMode = 0;
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMv[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cMv[0] - cMvPred[0][iRefIdx[0]];
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
      // ATRIBUIR GTS
      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMv( cGT0[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cGT0[0] - cGT0Pred[0][iRefIdx[0]];
      pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMv( cGT1[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cGT1[0] - cGT1Pred[0][iRefIdx[0]];
      pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMv( cGT2[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cGT2[0] - cGT2Pred[0][iRefIdx[0]];
      pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMv( cGT3[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cGT3[0] - cGT3Pred[0][iRefIdx[0]];
      pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );

     //pcCU->setGTFlag(iPartIdx, gtFlag[0]);
      pcCU->setGTFlagSubParts(gtFlag[0], uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr)); // REVER
#if IT_GT_CU_SIZE_LIMIT > 0
      if(!(size_CU > IT_GT_CU_SIZE_LIMIT)){
    	  pcCU->setGTFlag(iPartIdx, false);
    	  useGT_ME = false;
    	  useGT = false;
      }
#endif
#endif

      pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdx[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[0];
    }
    else
    {
    	// NOT USED YET
      uiLastMode = 1;
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMv[1], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
      TempMv = cMv[1] - cMvPred[1][iRefIdx[1]];
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMv( cGT0[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT0[1] - cGT0Pred[1][iRefIdx[1]];
//      pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMv( cGT1[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT1[1] - cGT1Pred[1][iRefIdx[1]];
//      pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMv( cGT2[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT2[1] - cGT2Pred[1][iRefIdx[1]];
//      pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMv( cGT3[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
//      TempMv = cGT3[1] - cGT3Pred[1][iRefIdx[1]];
//      pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
//      //pcCU->setGTFlag(iPartIdx, gtFlag[1]);
//      pcCU->setGTFlagSubParts(gtFlag[1], uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
#endif
      pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdx[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[1];
    }
#if AMP_MRG
    } // end if bTestNormalMC
#if IT_HOLOSS
    if (bNotValTmpCU && bTestNormalMC)
    {
      break;
    }
#endif
#else // AMP_MRG
#if IT_HOLOSS
  if (bNotValTmpCU)
  {
    break;
  }
#endif
#endif // AMP_MRG
    if ( pcCU->getPartitionSize( uiPartAddr ) != SIZE_2Nx2N )
    {
      UInt uiMRGInterDir = 0;     
      TComMvField cMRGMvField[2];
#if IT_GT
      TComMvField cMRGGT0Field[2];
      TComMvField cMRGGT1Field[2];
      TComMvField cMRGGT2Field[2];
      TComMvField cMRGGT3Field[2];
      Bool		  bMRGgtFlag[2];
#endif
      UInt uiMRGIndex = 0;

      UInt uiMEInterDir = 0;
      TComMvField cMEMvField[2];
#if IT_GT
      TComMvField cMEGT0Field[2];
      TComMvField cMEGT1Field[2];
      TComMvField cMEGT2Field[2];
      TComMvField cMEGT3Field[2];
      Bool		  bMEgtFlag[2];
#endif

      m_pcRdCost->getMotionCost( 1, 0 );
#if AMP_MRG
      // calculate ME cost
      UInt uiMEError = MAX_UINT;
      UInt uiMECost = MAX_UINT;

      if (bTestNormalMC)
      {
#if !IT_GT
        xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
#else
        xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME(), gtFlag[0]);
#endif
        uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
      }
#else
      // calculate ME cost
      UInt uiMEError = MAX_UINT;
      xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
      UInt uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
#endif 
      // save ME result.
      uiMEInterDir = pcCU->getInterDir( uiPartAddr );
      pcCU->getMvField( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEMvField[0] );
      pcCU->getMvField( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEMvField[1] );
#if IT_GT
      pcCU->getGT0Field( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEGT0Field[0] );
      pcCU->getGT0Field( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEGT0Field[1] );
      pcCU->getGT1Field( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEGT1Field[0] );
      pcCU->getGT1Field( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEGT1Field[1] );
      pcCU->getGT2Field( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEGT2Field[0] );
      pcCU->getGT2Field( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEGT2Field[1] );
      pcCU->getGT3Field( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEGT3Field[0] );
      pcCU->getGT3Field( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEGT3Field[1] );
      //bMEgtFlag[0] = pcCU->getGTFlag(iPartIdx);
      //bMEgtFlag[1] = pcCU->getGTFlag(iPartIdx);
#endif

      // find Merge result
      UInt uiMRGCost = MAX_UINT;
#if IT_HOLOSS
      Bool bValMerge = false;
#if !IT_GT
      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, bValMerge);
#else
      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, cMRGGT0Field, cMRGGT1Field, cMRGGT2Field, cMRGGT3Field,
    		  uiMRGIndex, uiMRGCost, cMvFieldNeighbours, cGT0FieldNeighbours, cGT1FieldNeighbours,cGT2FieldNeighbours,cGT3FieldNeighbours,
			  uhInterDirNeighbours, numValidMergeCand, bValMerge, useGT && useGT_MRG);
#endif
      if (!bValMerge)
      {
        uiMRGCost = MAX_UINT;
#if AMP_MRG
        if ( (!bTestNormalMC) // Means that there is no uiMECost
              ||
              (bNotValTmpCU) // Means that neither ME nor MERGE is valid (not valid SS vector)
            )
        {
          bNotValTmpCU = true; // if only the first "if statement" is true
          break; // URGENTE: confirmar se eh break ou continue
        }
#endif  // AMP_MRG 
      }
      else
      {
        bNotValTmpCU = false; // If there is a case where ME is not valid but MERGE is valid
      }
#else
      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
#endif
      if ( uiMRGCost < uiMECost )
      {
        // set Merge result
#if IT_DEBUG
    	  cout << "MRG is best" << endl;
#endif
        pcCU->setMergeFlagSubParts ( true,          uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setMergeIndexSubParts( uiMRGIndex,    uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts  ( uiMRGInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMRGMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMRGMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
        pcCU->getCUGT0Field( REF_PIC_LIST_0 )->setAllMvField( cMRGGT0Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT0Field( REF_PIC_LIST_1 )->setAllMvField( cMRGGT0Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT0Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT0Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field( REF_PIC_LIST_0 )->setAllMvField( cMRGGT1Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field( REF_PIC_LIST_1 )->setAllMvField( cMRGGT1Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field( REF_PIC_LIST_0 )->setAllMvField( cMRGGT2Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field( REF_PIC_LIST_1 )->setAllMvField( cMRGGT2Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field( REF_PIC_LIST_0 )->setAllMvField( cMRGGT3Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field( REF_PIC_LIST_1 )->setAllMvField( cMRGGT3Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
//        //pcCU->setGTFlag(iPartIdx, useGT_MRG);
        pcCU->setGTFlagSubParts ( useGT_MRG, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) ); // REVER
//        useGT = false;
#endif

        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      }
      else
      {
        // set ME result
#if IT_DEBUG
    	  cout << "ME is best" << endl;
#endif
        pcCU->setMergeFlagSubParts( false,        uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts ( uiMEInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMEMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMEMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );
#if IT_GT
        pcCU->getCUGT0Field( REF_PIC_LIST_0 )->setAllMvField( cMEGT0Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT0Field( REF_PIC_LIST_1 )->setAllMvField( cMEGT0Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field( REF_PIC_LIST_0 )->setAllMvField( cMEGT1Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT1Field( REF_PIC_LIST_1 )->setAllMvField( cMEGT1Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field( REF_PIC_LIST_0 )->setAllMvField( cMEGT2Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT2Field( REF_PIC_LIST_1 )->setAllMvField( cMEGT2Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field( REF_PIC_LIST_0 )->setAllMvField( cMEGT3Field[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUGT3Field( REF_PIC_LIST_1 )->setAllMvField( cMEGT3Field[1], ePartSize, uiPartAddr, 0, iPartIdx );
        //pcCU->setGTFlag(iPartIdx, bMEgtFlag[0]);
        if(!cMEGT0Field[0].getHor() && !cMEGT0Field[0].getVer() && !cMEGT1Field[0].getHor() && !cMEGT1Field[0].getVer() &&
           !cMEGT2Field[0].getHor() && !cMEGT2Field[0].getVer() && !cMEGT3Field[0].getHor() && !cMEGT3Field[0].getVer())
        	bMEgtFlag[0] = false;
        else
        	bMEgtFlag[0] = true;
        pcCU->setGTFlagSubParts( bMEgtFlag[0],        uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) ); // REVER
        useGT = bMEgtFlag[0];
#if IT_GT_CU_SIZE_LIMIT > 0
        if(!(size_CU > IT_GT_CU_SIZE_LIMIT)){
    	  pcCU->setGTFlag(uiPartAddr, false);
    	  useGT_ME = false;
    	  useGT = false;
      }
#endif
#endif
      }
    } // if ( pcCU->getPartitionSize( uiPartAddr ) != SIZE_2Nx2N )
#if IT_HOLOSS
    else
    {
      if (bNotValTmpCU)
      {
        break; // When it is SIZE_2Nx2N and there is not valid ME!!
        // To avoid motion compensation!!
      }
    }
#endif
    //  MC
    motionCompensation ( pcCU, rpcPredYuv,
#if IT_GT
    		useGT,
#endif
    		REF_PIC_LIST_X, iPartIdx );
    
  } //  end of for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )

  setWpScalingDistParam( pcCU, -1, REF_PIC_LIST_X );

  return;
}

// AMVP
#if ZERO_MVD_EST
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDistBiP, UInt* puiDist  )
#else
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDistBiP )
#endif
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();
  
  TComMv  cBestMv;
  Int     iBestIdx = 0;
  TComMv  cZeroMv;
  TComMv  cMvPred;
  UInt    uiBestCost = MAX_INT;
  UInt    uiPartAddr = 0;
  Int     iRoiWidth, iRoiHeight;
  Int     i;
  
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
  // Fill the MV Candidates
  if (!bFilled)
  {
    pcCU->fillMvpCand( uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pcAMVPInfo );
  }
  
  // initialize Mvp index & Mvp
  iBestIdx = 0;
  cBestMv  = pcAMVPInfo->m_acMvCand[0];
#if !ZERO_MVD_EST
  if (pcAMVPInfo->iN <= 1)
  {
    rcMvPred = cBestMv;
    
    pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));

    if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefPicList==REF_PIC_LIST_1)
    {
#if ZERO_MVD_EST
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif
    }
    return;
  }
#endif  
  if (bFilled)
  {
    assert(pcCU->getMVPIdx(eRefPicList,uiPartAddr) >= 0);
    rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList,uiPartAddr)];
    return;
  }
  
  m_cYuvPredTemp.clear();
#if ZERO_MVD_EST
  UInt uiDist;
#endif
  //-- Check Minimum Cost.
  for ( i = 0 ; i < pcAMVPInfo->iN; i++)
  {
    UInt uiTmpCost;
#if ZERO_MVD_EST
    uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
    uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif      
    if ( uiBestCost > uiTmpCost )
    {
      uiBestCost = uiTmpCost;
      cBestMv   = pcAMVPInfo->m_acMvCand[i];
      iBestIdx  = i;
      (*puiDistBiP) = uiTmpCost;
#if ZERO_MVD_EST
      (*puiDist) = uiDist;
#endif
    }
  }

  m_cYuvPredTemp.clear();
  
  // Setting Best MVP
  rcMvPred = cBestMv;
  pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  return;
}

UInt TEncSearch::xGetMvpIdxBits(Int iIdx, Int iNum)
{
  assert(iIdx >= 0 && iNum >= 0 && iIdx < iNum);
  
  if (iNum == 1)
  {
    return 0;
  }
  
  UInt uiLength = 1;
  Int iTemp = iIdx;
  if ( iTemp == 0 )
  {
    return uiLength;
  }
  
  Bool bCodeLast = ( iNum-1 > iTemp );
  
  uiLength += (iTemp-1);
  
  if( bCodeLast )
  {
    uiLength++;
  }
  
  return uiLength;
}

Void TEncSearch::xGetBlkBits( PartSize eCUMode, Bool bPSlice, Int iPartIdx, UInt uiLastMode, UInt uiBlkBit[3])
{
  if ( eCUMode == SIZE_2Nx2N )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else if ( (eCUMode == SIZE_2NxN || eCUMode == SIZE_2NxnU) || eCUMode == SIZE_2NxnD )
  {
    UInt aauiMbBits[2][3][3] = { { {0,0,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7,5,7}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( (eCUMode == SIZE_Nx2N || eCUMode == SIZE_nLx2N) || eCUMode == SIZE_nRx2N )
  {
    UInt aauiMbBits[2][3][3] = { { {0,2,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7-2,7-2,9-2}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( eCUMode == SIZE_NxN )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else
  {
    printf("Wrong!\n");
    assert( 0 );
  }
}

Void TEncSearch::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}

Void TEncSearch::xCheckBestMVP ( TComDataCU* pcCU, RefPicList eRefPicList, TComMv cMv, TComMv& rcMvPred, Int& riMVPIdx, UInt& ruiBits, UInt& ruiCost )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();
  
  assert(pcAMVPInfo->m_acMvCand[riMVPIdx] == rcMvPred);
  
  if (pcAMVPInfo->iN < 2) return;
  
  m_pcRdCost->getMotionCost( 1, 0 );
  m_pcRdCost->setCostScale ( 0    );
  
  Int iBestMVPIdx = riMVPIdx;
  
  m_pcRdCost->setPredictor( rcMvPred );
  Int iOrgMvBits  = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());
  iOrgMvBits += m_auiMVPIdxCost[riMVPIdx][AMVP_MAX_NUM_CANDS];
  Int iBestMvBits = iOrgMvBits;
  
  for (Int iMVPIdx = 0; iMVPIdx < pcAMVPInfo->iN; iMVPIdx++)
  {
    if (iMVPIdx == riMVPIdx) continue;
    
    m_pcRdCost->setPredictor( pcAMVPInfo->m_acMvCand[iMVPIdx] );
    
    Int iMvBits = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());
    iMvBits += m_auiMVPIdxCost[iMVPIdx][AMVP_MAX_NUM_CANDS];
    
    if (iMvBits < iBestMvBits)
    {
      iBestMvBits = iMvBits;
      iBestMVPIdx = iMVPIdx;
    }
  }
  
  if (iBestMVPIdx != riMVPIdx)  //if changed
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[iBestMVPIdx];
    
    riMVPIdx = iBestMVPIdx;
    UInt uiOrgBits = ruiBits;
    ruiBits = uiOrgBits - iOrgMvBits + iBestMvBits;
    ruiCost = (ruiCost - m_pcRdCost->getCost( uiOrgBits ))  + m_pcRdCost->getCost( ruiBits );
  }
}

UInt TEncSearch::xGetTemplateCost( TComDataCU* pcCU,
                                  UInt        uiPartIdx,
                                  UInt      uiPartAddr,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcTemplateCand,
                                  TComMv      cMvCand,
                                  Int         iMVPIdx,
                                  Int     iMVPNum,
                                  RefPicList  eRefPicList,
                                  Int         iRefIdx,
                                  Int         iSizeX,
                                  Int         iSizeY
                               #if ZERO_MVD_EST
                                , UInt&       ruiDist
                               #endif
                                  )
{
  UInt uiCost  = MAX_INT;
#if IT_GT
  TComMv cMvZero;
  cMvZero.set(0,0);
#endif
  
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();
#if IT_HOLOSS
  Int riOffsetX, riOffsetY;
  Bool isFirstRow, isFirstCol;
  pcCU->getPartOffset(uiPartIdx, uiPartAddr, riOffsetX, riOffsetY, isFirstRow, isFirstCol );
#endif
#if IT_HOLOSS
  if (
        // For ISS_SLICE and PSS_SLICE:
        ( pcCU->getSlice()->isIntraSS() || pcCU->getSlice()->isInterPSS() ) &&
#if IT_SCALABLE_V1
        ( !pcCU->getSlice()->isScalableSlice() ) &&
#endif
        // SS reference is in reference list 0:
        !eRefPicList  &&
        // SS reference is in the last position of the list 0:
        ( iRefIdx == pcCU->getSlice()->getRefIdxOfSS() ) && 
        // See if it is valid or not. Here, not to need to clip MV - it is already done on fillMvpCand:
        ( !m_pcRdCost->isValidPattern( pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ), pcPicYuvRef->getStride(), cMvCand, iSizeX, iSizeY) )
     )
  {
    return uiCost;
  }
#endif
  pcCU->clipMv( cMvCand );

  // prediction pattern
  if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, true
#if IT_GT
    , false, &cMvZero, &cMvZero, &cMvZero, &cMvZero
#endif
    		);
  }
  else
  {
    xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, false
#if IT_GT
    , false, &cMvZero, &cMvZero, &cMvZero, &cMvZero
#endif
    		 );
  }

  if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xWeightedPredictionUni( pcCU, pcTemplateCand, uiPartAddr, iSizeX, iSizeY, eRefPicList, pcTemplateCand, iRefIdx );
  }

  // calc distortion
#if ZERO_MVD_EST
  m_pcRdCost->getMotionCost( 1, 0 );
  DistParam cDistParam;
  m_pcRdCost->setDistParam( cDistParam, g_bitDepthY,
                            pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), 
                            pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), 
                            iSizeX, iSizeY, m_pcEncCfg->getUseHADME() );
  ruiDist = cDistParam.DistFunc( &cDistParam );
  uiCost = ruiDist + m_pcRdCost->getCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum] );
#else
  uiCost = m_pcRdCost->getDistPart(g_bitDepthY, pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), iSizeX, iSizeY, TEXT_LUMA, DF_SAD );
  uiCost = (UInt) m_pcRdCost->calcRdCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum], uiCost, false, DF_SAD );
#endif
  return uiCost;
}

Void TEncSearch::xMotionEstimation( TComDataCU* pcCU,
                                    TComYuv* pcYuvOrg,
                                    Int iPartIdx,
                                    RefPicList eRefPicList,
                                    TComMv* pcMvPred,
                                    Int iRefIdxPred,
                                    TComMv& rcMv,
                                    UInt& ruiBits,
                                    UInt& ruiCost,
#if IT_HOLOSS
                                    Bool&       bNotValCU,
#endif
#if IT_GT
									Bool&		  bUseGT,
									TComMv&		  rcGT0,
									TComMv&		  rcGT1,
									TComMv&		  rcGT2,
									TComMv&		  rcGT3,
									Bool&		  gtFlag,
#endif
                                    Bool bBi
                                  )
{
  UInt          uiPartAddr;
  Int           iRoiWidth;
  Int           iRoiHeight;
#if IT_HOLOSS
  Int           iOffsetX;
  Int           iOffsetY;
  Bool          bisFirstRow, bisFirstCol;
#endif
  
  TComMv        cMvHalf, cMvQter;
  TComMv        cMvSrchRngLT;
  TComMv        cMvSrchRngRB;
  
  TComYuv*      pcYuv = pcYuvOrg;
  m_iSearchRange = m_aaiAdaptSR[eRefPicList][iRefIdxPred];
  
  Int           iSrchRng      = ( bBi ? m_bipredSearchRange : m_iSearchRange );
  TComPattern*  pcPatternKey  = pcCU->getPattern        ();
  
  Double        fWeight       = 1.0;
  
  pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
#if IT_HOLOSS
  pcCU->getPartOffset(iPartIdx, uiPartAddr, iOffsetX, iOffsetY, bisFirstRow, bisFirstCol );
#endif

  if ( bBi )
  {
    TComYuv*  pcYuvOther = &m_acYuvPred[1-(Int)eRefPicList];
    pcYuv                = &m_cYuvPredTemp;
    
    pcYuvOrg->copyPartToPartYuv( pcYuv, uiPartAddr, iRoiWidth, iRoiHeight );
    
    pcYuv->removeHighFreq( pcYuvOther, uiPartAddr, iRoiWidth, iRoiHeight );
    
    fWeight = 0.5;
  }
  
  //  Search key pattern initialization
  pcPatternKey->initPattern( pcYuv->getLumaAddr( uiPartAddr ),
                            pcYuv->getCbAddr  ( uiPartAddr ),
                            pcYuv->getCrAddr  ( uiPartAddr ),
                            iRoiWidth,
                            iRoiHeight,
                            pcYuv->getStride(),
                            0, 0 );
  
  Pel*        piRefY      = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr );
  Int         iRefStride  = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getStride();
  
  TComMv      cMvPred = *pcMvPred;
  
  if ( bBi )  xSetSearchRange   ( pcCU, rcMv   , iSrchRng, cMvSrchRngLT, cMvSrchRngRB );
  else        xSetSearchRange   ( pcCU, cMvPred, iSrchRng, cMvSrchRngLT, cMvSrchRngRB );
  
  m_pcRdCost->getMotionCost ( 1, 0 );
  
  m_pcRdCost->setPredictor  ( *pcMvPred );
  m_pcRdCost->setCostScale  ( 2 );

  setWpScalingDistParam( pcCU, iRefIdxPred, eRefPicList );
  //  Do integer search
#if IT_HOLOSS
   // SS reference is in the last position of List 0
  Bool bIsSSE = ( pcCU->getSlice()->isIntraSS() ) || ( pcCU->getSlice()->isInterPSS() && !eRefPicList  && ( iRefIdxPred == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) );
#endif

#if IT_SCALABLE_V1
  bIsSSE = pcCU->getSlice()->isScalableSlice() ? false : bIsSSE;
#endif
  if ( !m_iFastSearch || bBi )
  {
#if IT_HOLOSS
    if ( bIsSSE )
    {
      xSetSearchRange   (pcCU, cMvSrchRngLT, cMvSrchRngRB, iOffsetX, iOffsetY, bisFirstRow, bisFirstCol);
    }
#if !IT_SS_NUMBER_OF_BEST_CAND
    xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost, iOffsetX, iOffsetY, bIsSSE );
#else
    xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost, iOffsetX, iOffsetY, pcCU->getSSBestCand() ,bIsSSE );
#endif
#else
    xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
#endif
  }
  else
  {
#if IT_HOLOSS
    if ( bIsSSE )
    {
      printf("IT development for non-scalable codec does not support fast search (TZ search)...");
      exit(0);
    }
#endif
    rcMv = *pcMvPred;
    xPatternSearchFast  ( pcCU, pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
  }
 #if IT_HOLOSS
  if ( bIsSSE )
  {
    if( ( ruiCost == MAX_UINT ) || 
        ( (rcMv.getHor() == 0)&&(rcMv.getVer() == 0) )  ||
        ( pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getBufY()[0x00] == NOT_VALID )
    )
    {
      // means that the vector is not valid and cannot continue with fractional estimation!
      bNotValCU = true;
      return;
    }
  }
#endif
  m_pcRdCost->getMotionCost( 1, 0 );
  m_pcRdCost->setCostScale ( 1 );

  xPatternSearchFracDIF( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost,bBi );

  m_pcRdCost->setCostScale( 0 );

#if IT_GT

//#if IT_GT_CU_SIZE_LIMIT > 0
//  if((size_CU > IT_GT_CU_SIZE_LIMIT)){
 // {
//#endif
  if(bUseGT)
  {
#if !IT_SS_NUMBER_OF_BEST_CAND
	  xPatternSearchGT( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, &cMvHalf, &cMvQter, &rcGT0, &rcGT1, &rcGT2, &rcGT3, gtFlag, ruiCost,bBi );
#else
	  xPatternSearchGT( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, &cMvHalf, &cMvQter, &rcGT0, &rcGT1, &rcGT2, &rcGT3, gtFlag, ruiCost,bBi, pcCU->getSSBestCand() );
#endif
  }
  else
  {
	  gtFlag = false;
	  rcGT0.set(0,0);
	  rcGT1.set(0,0);
	  rcGT2.set(0,0);
	  rcGT3.set(0,0);
  }
//#if IT_GT_CU_SIZE_LIMIT > 0
//  }else{
//	  gtFlag = false;
//	  rcGT0.set(0,0);
//	  rcGT1.set(0,0);
//	  rcGT2.set(0,0);
//	  rcGT3.set(0,0);
//  }
//#endif
#endif

  rcMv <<= 2;
  rcMv += (cMvHalf <<= 1);
  rcMv +=  cMvQter;

  UInt uiMvBits = m_pcRdCost->getBits( rcMv.getHor(), rcMv.getVer() );
  ruiBits      += uiMvBits;

#if IT_GT
  // GT FLAG
#if IT_GT_CU_SIZE_LIMIT > 0
  Int power = 5 - pcCU->getDepth(uiPartAddr);
  Int size_CU = 2 << power;
  if(size_CU > IT_GT_CU_SIZE_LIMIT)
	  ruiBits += 1;
#else
  ruiBits += 1;
#endif
  if(bUseGT)
    {// GT Vect
	  if(!(rcGT0.getHor() == rcGT0.getVer() == rcGT1.getHor() == rcGT1.getVer() ==
	     rcGT2.getHor() == rcGT2.getVer() == rcGT3.getHor() == rcGT3.getVer() ))
	  { // if there is a GT
		  ruiBits += m_pcRdCost->getBitsGT( rcGT0.getHor(), rcGT0.getVer(), rcGT1.getHor(), rcGT1.getVer(),
				  	  	  	  	  	  	    rcGT2.getHor(), rcGT2.getVer(), rcGT3.getHor(), rcGT3.getVer());
	  }
    }
#endif

  ruiCost       = (UInt)( floor( fWeight * ( (Double)ruiCost - (Double)m_pcRdCost->getCost( uiMvBits ) ) ) + (Double)m_pcRdCost->getCost( ruiBits ) );
}

#if IT_GT
Void TEncSearch::xPatternSearchGT(TComDataCU* pcCU,
                                       TComPattern* pcPatternKey,
                                       Pel* piRefY,
                                       Int iRefStride,
                                       TComMv* pcMvInt,
                                       TComMv* rcMvHalf,
                                       TComMv* rcMvQter,
                                       TComMv* rcGT0,
                                       TComMv* rcGT1,
									   TComMv* rcGT2,
									   TComMv* rcGT3,
									   Bool& gtFlag,
                                       UInt& ruiCost
                                       ,Bool biPred
#if IT_SS_NUMBER_OF_BEST_CAND
										 ,TComMv* bestSSCand
#endif
                                       )
{

	//gtFlag = true;
	rcGT0->set(0,0);
	rcGT1->set(0,0);
	rcGT2->set(0,0);
	rcGT3->set(0,0);

	TComPattern cPatternRoi;
	Short Ver = pcMvInt->getVer();
	Short Hor = pcMvInt->getHor();

	Ver <<= 1;
	Ver += rcMvHalf->getVer();
	Ver <<= 1;
	Ver += rcMvQter->getVer();

	Hor <<= 1;
	Hor += rcMvHalf->getHor();
	Hor <<= 1;
	Hor += rcMvQter->getHor();

	Int iRows = pcPatternKey->getROIYHeight();
	Int iCols = pcPatternKey->getROIYWidth();
	Int iOffset = pcMvInt->getHor() - iCols/2 + (pcMvInt->getVer() - iRows/2) * iRefStride;
	Int piRefSrchStride = m_filteredBlock[0][0].getStride();

	cPatternRoi.initPattern( piRefY +  iOffset, NULL, NULL, iCols * 2, iRows * 2, iRefStride, 0, 0 );

	xExtDIFUpSamplingH ( &cPatternRoi, biPred );
	xExtDIFUpSamplingQ ( &cPatternRoi, *rcMvHalf, biPred );

	Pel* piRefSrch = m_filteredBlock[ Ver & 3 ][ Hor & 3 ].getLumaAddr();
	if ( Hor == 2 && ( Ver & 1 ) == 0 )
	{
		piRefSrch += 1;
	}
	if ( ( Hor & 1 ) == 0 && Ver == 2 )
	{
		piRefSrch += m_filteredBlock[0][0].getStride();
	}

	piRefSrch += (iCols/2) + (iRows/2) * piRefSrchStride; // inside block (jump the margin pixels)

	// BMGT variables
	Pel* piAux = (Pel*)xMalloc(Pel, (iRows * iCols));
	Double dProjective[9];
	Int iBestCornerX[4], iBestCornerY[4];
	Int iCurrCornerX[4], iCurrCornerY[4];
	Int iBestNSSCenterX[4], iBestNSSCenterY[4];
	Int iCurrNSSCenterX[4], iCurrNSSCenterY[4];
	Int iNSSIteration[4], iMaxNSSIteration = IT_MAX_NSS_Iteration;
	Int iNSSWindow = ((iRows < iCols) ? (iRows) : (iCols)) >> 1; // 64x32 -> 32 -> Window 16x16
#if IT_GT_GRID_SIZE > 1
	iNSSWindow *= IT_GT_GRID_SIZE;
#endif
#if IT_GT_Iteration_Limit
	iMaxNSSIteration = log2((double)(iNSSWindow/IT_GT_GRID_SIZE));
#endif
	 Int lastIterationStep = iNSSWindow >> iMaxNSSIteration;
	if(lastIterationStep == 0)
		lastIterationStep = 1;

	Int i0, i1, i2, i3;
	Int marginX = 0, marginY = 0; // negative values allow causal area search
	UInt uiDist, uiDistBest = ruiCost; // REFERENCE COST!

	// init variables
	m_pcRdCost->setDistParam( pcPatternKey, piAux, iCols, 1, m_cDistParam, m_pcEncCfg->getUseHADME() );
	uiDist = uiDistBest;
	//Hor += pcMvInt->getHor() << 2;
	//Ver += pcMvInt->getVer() << 2;
	for (Int k = 0; k < 4; k++){ iBestCornerX[k] = 0; iBestCornerY[k] = 0; }
	dProjective[0] = 1.0; dProjective[1] = 0.0; dProjective[2] = 0.0;
	dProjective[3] = 0.0; dProjective[4] = 1.0; dProjective[5] = 0.0;
	dProjective[6] = 0.0; dProjective[7] = 0.0; dProjective[8] = 1.0;
#if IT_GT_GRID_SIZE == 1
	iCurrNSSCenterX[0] = iBestNSSCenterX[0] = 0;          iCurrNSSCenterY[0] = iBestNSSCenterY[0] = 0;
	iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols - 1;  iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
	iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols - 1;  iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows - 1;
	iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;          iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows - 1;
#else
	iCurrNSSCenterX[0] = iBestNSSCenterX[0] = 0;          					iCurrNSSCenterY[0] = iBestNSSCenterY[0] = 0;
	iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols*IT_GT_GRID_SIZE - 1;  	iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
	iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols*IT_GT_GRID_SIZE - 1;  	iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows*IT_GT_GRID_SIZE - 1;
	iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;         			 		iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows*IT_GT_GRID_SIZE - 1;
#endif

#if IT_DEBUG
	cout << "SS - " << Hor << " " << Ver << endl;
#endif

#if IT_GT_SEARCH == 0
	// SEARCH NSS
	iNSSIteration[0] = 1;
	for (Int j0 = iNSSWindow; (j0 > 1) && (iNSSIteration[0] <= iMaxNSSIteration); j0 /= 2){
		iNSSIteration[0]++;
		if (j0 == iNSSWindow){
			iCurrNSSCenterX[0] = iBestNSSCenterX[0] = 0;
			iCurrNSSCenterY[0] = iBestNSSCenterY[0] = 0;
#if IT_Independent_Iterations
			iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols - 1;
			iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
			iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols - 1;
			iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows - 1;
			iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;
			iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows - 1;
#endif
		}
		else{
			iCurrNSSCenterX[0] = iBestNSSCenterX[0];
			iCurrNSSCenterY[0] = iBestNSSCenterY[0];
#if IT_Independent_Iterations
			iCurrNSSCenterX[1] = iBestNSSCenterX[1];
			iCurrNSSCenterY[1] = iBestNSSCenterY[1];
			iCurrNSSCenterX[2] = iBestNSSCenterX[2];
			iCurrNSSCenterY[2] = iBestNSSCenterY[2];
			iCurrNSSCenterX[3] = iBestNSSCenterX[3];
			iCurrNSSCenterY[3] = iBestNSSCenterY[3];
#endif
		}//------------------------------------------ corner 0 ---------------------------------------------------//
		i0 = j0;
		for (Int y0 = i0 / 2; y0 >= -i0 / 2; y0 -= i0 / 2) {
			iCurrCornerY[0] = iCurrNSSCenterY[0] + y0;
			for (Int x0 = i0 / 2; x0 >= -i0 / 2; x0 -= i0 / 2) {
				iCurrCornerX[0] = iCurrNSSCenterX[0] + x0;
#if !IT_Independent_Iterations
				iNSSIteration[1] = 1;
				for (Int j1 = iNSSWindow; (j1 > 1) && (iNSSIteration[1] <= iMaxNSSIteration); j1 /= 2){
					iNSSIteration[1]++;
					if (j1 == iNSSWindow){
						iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols - 1;
						iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
					}
					else{
						iCurrNSSCenterX[1] = iBestNSSCenterX[1];
						iCurrNSSCenterY[1] = iBestNSSCenterY[1];
					}
					i1 = j1;
#else
					i1 = j0;
#endif
					//-------------------------------------- corner 1 ------------------------------------------------------//
					for (Int y1 = i1 / 2; y1 >= -i1 / 2; y1 -= i1 / 2) {
						iCurrCornerY[1] = iCurrNSSCenterY[1] + y1;
						for (Int x1 = i1 / 2; x1 >= -i1 / 2; x1 -= i1 / 2) {
							iCurrCornerX[1] = iCurrNSSCenterX[1] + x1;
#if !IT_Independent_Iterations
							iNSSIteration[2] = 1;
							for (Int j2 = iNSSWindow; (j2 > 1) && (iNSSIteration[2] <= iMaxNSSIteration); j2 /= 2){
								iNSSIteration[2]++;
								if (j2 == iNSSWindow){
									iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols - 1;
									iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows - 1;
								}
								else{
									iCurrNSSCenterX[2] = iBestNSSCenterX[2];
									iCurrNSSCenterY[2] = iBestNSSCenterY[2];
								}
								i2 = j2;
#else
								i2 = j0;
#endif
								//-------------------------------------- corner 2 ------------------------------------------------------//
								for (Int y2 = i2 / 2; y2 >= -i2 / 2; y2 -= i2 / 2) {
									iCurrCornerY[2] = iCurrNSSCenterY[2] + y2;
									for (Int x2 = i2 / 2; x2 >= -i2 / 2; x2 -= i2 / 2) {
										iCurrCornerX[2] = iCurrNSSCenterX[2] + x2;
#if !IT_Independent_Iterations
										iNSSIteration[3] = 1;
										for (Int j3 = iNSSWindow; (j3 > 1) && (iNSSIteration[3] <= iMaxNSSIteration); j3 /= 2){
											iNSSIteration[3]++;
											if (j3 == iNSSWindow){
												iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;
												iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows - 1;
											}
											else{
												iCurrNSSCenterX[3] = iBestNSSCenterX[3];
												iCurrNSSCenterY[3] = iBestNSSCenterY[3];
											}
											i3 = j3;
#else
											i3 = j0;
#endif
											//-------------------------------------- corner 3 ------------------------------------------------------//
											for (Int y3 = i3 / 2; y3 >= -i3 / 2; y3 -= i3 / 2) {
												iCurrCornerY[3] = iCurrNSSCenterY[3] + y3;
												for (Int x3 = i3 / 2; x3 >= -i3 / 2; x3 -= i3 / 2) {
													iCurrCornerX[3] = iCurrNSSCenterX[3] + x3;
													// if not translation
													if ( !(x0 == x1  && x0 == x2 && x0 == x3 && y0 == y1 && y0 == y2 && y0 == y3) )
													{
														// valid GT location
														if(((x0 + pcMvInt->getHor()        < -marginX  && y0 + pcMvInt->getVer()        <= -marginY)  || (x0 + pcMvInt->getHor()         >= -marginX  && y0 + pcMvInt->getVer()         < -marginY))  &&
																((x1 + pcMvInt->getHor() +iCols < -marginX  && y1 + pcMvInt->getVer()        <= -marginY)  || (x1 + pcMvInt->getHor() +iCols  >= -marginX  && y1 + pcMvInt->getVer()         < -marginY))  &&
																((x2 + pcMvInt->getHor() +iCols < -marginX  && y2 + pcMvInt->getVer() +iRows <= -marginY)  || (x2 + pcMvInt->getHor() +iCols  >= -marginX  && y2 + pcMvInt->getVer() +iRows  < -marginY))  &&
																((x3 + pcMvInt->getHor()        < -marginX  && y3 + pcMvInt->getVer() +iRows <= -marginY)  || (x3 + pcMvInt->getHor()         >= -marginX  && y3 + pcMvInt->getVer() +iRows  < -marginY))    )
														{
															// calculate gt param
															calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
															if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
																ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
																setDistParamComp(0);
															m_cDistParam.pCur = piAux;
																m_cDistParam.bitDepth = g_bitDepthY;
																uiDist = m_cDistParam.DistFunc(&m_cDistParam);
																uiDist += m_pcRdCost->getCost( Hor , Ver );
																uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
																		(iCurrCornerX[0])/lastIterationStep				, (iCurrCornerY[0])/lastIterationStep			,
																		(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])        /lastIterationStep   ,
																		(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep,
																		(iCurrCornerX[3])/lastIterationStep				, (iCurrCornerY[3] - iRows +1)/lastIterationStep)
																);
																// GT COST
																// check if best
																if (uiDist < uiDistBest)
																{
																	uiDistBest = uiDist;
																	iBestCornerX[0] = iCurrCornerX[0];
																	iBestCornerX[1] = iCurrCornerX[1];
																	iBestCornerX[2] = iCurrCornerX[2];
																	iBestCornerX[3] = iCurrCornerX[3];
																	iBestCornerY[0] = iCurrCornerY[0];
																	iBestCornerY[1] = iCurrCornerY[1];
																	iBestCornerY[2] = iCurrCornerY[2];
																	iBestCornerY[3] = iCurrCornerY[3];
																	iBestNSSCenterX[0] = iCurrCornerX[0];
																	iBestNSSCenterX[1] = iCurrCornerX[1];
																	iBestNSSCenterX[2] = iCurrCornerX[2];
																	iBestNSSCenterX[3] = iCurrCornerX[3];
																	iBestNSSCenterY[0] = iCurrCornerY[0];
																	iBestNSSCenterY[1] = iCurrCornerY[1];
																	iBestNSSCenterY[2] = iCurrCornerY[2];
																	iBestNSSCenterY[3] = iCurrCornerY[3];
																}
#if IT_GT_AFFINE
															}
#endif
														}
													}
												}
											} // CORNER 3
#if !IT_Independent_Iterations
										}
#endif
									}
								} // CORNER 2
#if !IT_Independent_Iterations
							}
#endif
						}
					} // CORNER 1
#if !IT_Independent_Iterations
				}
#endif
			}
		} // CORNER 0
	}

	calcParamProjective(iBestCornerX, iBestCornerY, dProjective, iCols, iRows);
	ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
	xFree(piAux);


	if (iBestCornerX[0] != 0 || iBestCornerY[0] != 0 ||
	    iBestCornerX[1] != 0 || iBestCornerY[1] != 0 ||
	    iBestCornerX[2] != 0 || iBestCornerY[2] != 0 ||
	    iBestCornerX[3] != 0 || iBestCornerY[3] != 0 )
	{
		gtFlag = true;
		rcGT0->set((iBestCornerX[0])        /lastIterationStep, (iBestCornerY[0])        /lastIterationStep);
		rcGT1->set((iBestCornerX[1] - iCols +1)/lastIterationStep, (iBestCornerY[1])        /lastIterationStep);
		rcGT2->set((iBestCornerX[2] - iCols +1)/lastIterationStep, (iBestCornerY[2] - iRows +1)/lastIterationStep);
		rcGT3->set((iBestCornerX[3])        /lastIterationStep, (iBestCornerY[3] - iRows +1)/lastIterationStep);
		ruiCost = uiDistBest;
	}else{
		gtFlag = false;
		rcGT0->set(0,0);
		rcGT1->set(0,0);
		rcGT2->set(0,0);
		rcGT3->set(0,0);
	}
#else
#if IT_GT_SEARCH == 1
	int N = 2;
	// SQUARE UNIT SEARCH
	for (Int y0 = -N; y0 <= N; y0++)
	{
		iCurrCornerY[0] = iCurrNSSCenterY[0] + y0;
		for (Int x0 = -N; x0 <= N; x0++)
		{
			iCurrCornerX[0] = iCurrNSSCenterX[0] + x0;
			for (Int y1 = -N; y1 <= N; y1++)
			{
				iCurrCornerY[1] = iCurrNSSCenterY[1] + y1;
				for (Int x1 = -N; x1 <= N; x1++)
				{
					iCurrCornerX[1] = iCurrNSSCenterX[1] + x1;
					for (Int y2 = -N; y2 <= N; y2++)
					{
						iCurrCornerY[2] = iCurrNSSCenterY[2] + y2;
						for (Int x2 = -N; x2 <= N; x2++)
						{
							iCurrCornerX[2] = iCurrNSSCenterX[2] + x2;
							for (Int y3 = -N; y3 <= N; y3++)
							{
								iCurrCornerY[3] = iCurrNSSCenterY[3] + y3;
								for (Int x3 = -N; x3 <= N; x3++)
								{
									iCurrCornerX[3] = iCurrNSSCenterX[3] + x3;
									// if not translation
									if ( !(x0 == x1  && x0 == x2 && x0 == x3 && y0 == y1 && y0 == y2 && y0 == y3) )
									{
										// valid GT location
										if(((x0 + pcMvInt->getHor()        < -marginX  && y0 + pcMvInt->getVer()        <= -marginY)  || (x0 + pcMvInt->getHor()         >= -marginX  && y0 + pcMvInt->getVer()         < -marginY))  &&
												((x1 + pcMvInt->getHor() +iCols < -marginX  && y1 + pcMvInt->getVer()        <= -marginY)  || (x1 + pcMvInt->getHor() +iCols  >= -marginX  && y1 + pcMvInt->getVer()         < -marginY))  &&
												((x2 + pcMvInt->getHor() +iCols < -marginX  && y2 + pcMvInt->getVer() +iRows <= -marginY)  || (x2 + pcMvInt->getHor() +iCols  >= -marginX  && y2 + pcMvInt->getVer() +iRows  < -marginY))  &&
												((x3 + pcMvInt->getHor()        < -marginX  && y3 + pcMvInt->getVer() +iRows <= -marginY)  || (x3 + pcMvInt->getHor()         >= -marginX  && y3 + pcMvInt->getVer() +iRows  < -marginY))    )
										{
											// calculate gt param
											calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
											if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
												ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
												setDistParamComp(0);
												m_cDistParam.pCur = piAux;
												m_cDistParam.bitDepth = g_bitDepthY;
												uiDist = m_cDistParam.DistFunc(&m_cDistParam);
												uiDist += m_pcRdCost->getCost( Hor , Ver );
												uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
														(iCurrCornerX[0])			  , (iCurrCornerY[0])			,
														(iCurrCornerX[1] - iCols +1)  , (iCurrCornerY[1])           ,
														(iCurrCornerX[2] - iCols +1)  , (iCurrCornerY[2] - iRows +1),
														(iCurrCornerX[3])			  , (iCurrCornerY[3] - iRows +1) )
												);
												// GT COST
												// check if best
												if (uiDist < uiDistBest)
												{
													uiDistBest = uiDist;
													iBestCornerX[0] = iCurrCornerX[0];
													iBestCornerX[1] = iCurrCornerX[1];
													iBestCornerX[2] = iCurrCornerX[2];
													iBestCornerX[3] = iCurrCornerX[3];
													iBestCornerY[0] = iCurrCornerY[0];
													iBestCornerY[1] = iCurrCornerY[1];
													iBestCornerY[2] = iCurrCornerY[2];
													iBestCornerY[3] = iCurrCornerY[3];
												}
#if IT_GT_AFFINE
											}
#endif
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	calcParamProjective(iBestCornerX, iBestCornerY, dProjective, iCols, iRows);
	ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
	xFree(piAux);


	if (iBestCornerX[0] != 0 || iBestCornerY[0] != 0 ||
			iBestCornerX[1] != 0 || iBestCornerY[1] != 0 ||
			iBestCornerX[2] != 0 || iBestCornerY[2] != 0 ||
			iBestCornerX[3] != 0 || iBestCornerY[3] != 0 )
	{
		gtFlag = true;
		rcGT0->set((iBestCornerX[0])           , (iBestCornerY[0])           );
		rcGT1->set((iBestCornerX[1] - iCols +1), (iBestCornerY[1])           );
		rcGT2->set((iBestCornerX[2] - iCols +1), (iBestCornerY[2] - iRows +1));
		rcGT3->set((iBestCornerX[3])           , (iBestCornerY[3] - iRows +1));
		ruiCost = uiDistBest;
	}else{
		gtFlag = false;
		rcGT0->set(0,0);
		rcGT1->set(0,0);
		rcGT2->set(0,0);
		rcGT3->set(0,0);
	}
#endif
#if IT_GT_SEARCH == 2 // DIAMOND

#if IT_SS_NUMBER_OF_BEST_CAND
	Int iBestSSX = 0;
	Int iBestSSY = 0;
	TComMv nullVect = TComMv (0,0);
#if IT_SS_USE_PREDICTORS
	AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo();
	Int numPred = pcAMVPInfo->iN;
	TComMv extraVects[numPred];
	for(Int i=0; i<numPred; i++)
		extraVects[i] = pcAMVPInfo->m_acMvCand[i]; // 1/4 pel
#endif
	for(Int b = 0; b < IT_SS_NUMBER_OF_BEST_CAND
#if IT_SS_USE_PREDICTORS
	+ numPred
#endif
	; b++)
	{
#if IT_SS_USE_PREDICTORS
		if(b<IT_SS_NUMBER_OF_BEST_CAND)
		{
#endif
			if(bestSSCand[b] == nullVect)
				continue;
			Ver = bestSSCand[b].getVer();
			Hor = bestSSCand[b].getHor(); // 1 pel
			iOffset = bestSSCand[b].getHor() - iCols/2 + (bestSSCand[b].getVer() - iRows/2) * iRefStride;

			Ver <<= 2;
			Hor <<= 2; // 1/4 pel
#if IT_SS_QUARTER_PEL
#if IT_SS_NUMBER_OF_BEST_CAND == 1
			Ver >>= 2;
			Hor >>= 2; // 1 pel

			Ver <<= 1;
			Ver += rcMvHalf->getVer();
			Ver <<= 1;
			Ver += rcMvQter->getVer();

			Hor <<= 1;
			Hor += rcMvHalf->getHor();
			Hor <<= 1;
			Hor += rcMvQter->getHor(); // 1/4 pel
#endif
#endif
#if IT_SS_USE_PREDICTORS
		}
		else
		{
			if(extraVects[b-IT_SS_NUMBER_OF_BEST_CAND] == nullVect)
				continue;
			Ver = extraVects[b-IT_SS_NUMBER_OF_BEST_CAND].getVer();
			Hor = extraVects[b-IT_SS_NUMBER_OF_BEST_CAND].getHor(); // 1/4 pel
			Ver >>=2;
			Hor >>=2; // 1 pel
			iOffset = Hor - iCols/2 + (Ver - iRows/2) * iRefStride;
#if !IT_SS_QUARTER_PEL
			Ver <<=2; // 1/4 pel
			Hor <<=2;
#else
			Ver = extraVects[b-IT_SS_NUMBER_OF_BEST_CAND].getVer();
			Hor = extraVects[b-IT_SS_NUMBER_OF_BEST_CAND].getHor(); // 1/4 pel
#endif
		}
#endif
		//cout << b << "/" << IT_SS_NUMBER_OF_BEST_CAND + numPred << " " << Ver << " " << Hor << endl;
		cPatternRoi.initPattern( piRefY +  iOffset, NULL, NULL, iCols * 2, iRows * 2, iRefStride, 0, 0 );
		xExtDIFUpSamplingH ( &cPatternRoi, biPred );
		xExtDIFUpSamplingQ ( &cPatternRoi, nullVect, biPred );
#if !IT_SS_QUARTER_PEL
		piRefSrch = m_filteredBlock[ 0 ][ 0 ].getLumaAddr(); // no interpolation
#else
		piRefSrch = m_filteredBlock[ Ver & 3 ][ Hor & 3 ].getLumaAddr();
		if ( Hor == 2 && ( Ver & 1 ) == 0 )
		{
			piRefSrch += 1;
		}
		if ( ( Hor & 1 ) == 0 && Ver == 2 )
		{
			piRefSrch += m_filteredBlock[0][0].getStride();
		}
#endif
		piRefSrch += (iCols/2) + (iRows/2) * piRefSrchStride; // inside block (jump the margin pixels)
#endif

		iNSSIteration[0] = 1;
		for (Int j0 = iNSSWindow; (j0 > 1) && (iNSSIteration[0] <= iMaxNSSIteration); j0 /= 2){
			iNSSIteration[0]++;
			if (j0 == iNSSWindow){
				iCurrNSSCenterX[0] = iBestNSSCenterX[0] = 0;
				iCurrNSSCenterY[0] = iBestNSSCenterY[0] = 0;
#if IT_Independent_Iterations
#if IT_GT_GRID_SIZE == 1
				iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols - 1;
				iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
				iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols - 1;
				iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows - 1;
				iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;
				iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows - 1;
#else
				iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols*IT_GT_GRID_SIZE - 1;
				iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
				iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols*IT_GT_GRID_SIZE - 1;
				iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows*IT_GT_GRID_SIZE - 1;
				iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;
				iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows*IT_GT_GRID_SIZE - 1;
#endif
#endif
			}
			else{
				iCurrNSSCenterX[0] = iBestNSSCenterX[0];
				iCurrNSSCenterY[0] = iBestNSSCenterY[0];
#if IT_Independent_Iterations
				iCurrNSSCenterX[1] = iBestNSSCenterX[1];
				iCurrNSSCenterY[1] = iBestNSSCenterY[1];
				iCurrNSSCenterX[2] = iBestNSSCenterX[2];
				iCurrNSSCenterY[2] = iBestNSSCenterY[2];
				iCurrNSSCenterX[3] = iBestNSSCenterX[3];
				iCurrNSSCenterY[3] = iBestNSSCenterY[3];
#endif
			}//------------------------------------------ corner 0 ---------------------------------------------------//
			i0 = j0;
			for (Int y0 = i0 / 2; y0 >= -i0 / 2; y0 -= i0 / 2) {
				iCurrCornerY[0] = iCurrNSSCenterY[0] + y0;
				for (Int x0 = i0 / 2; x0 >= -i0 / 2; x0 -= i0 / 2) {
					if((y0 != 0 && x0 == 0 ) || (y0 == 0 && x0 != 0 ) || (y0 == 0 && x0 == 0 )){
						iCurrCornerX[0] = iCurrNSSCenterX[0] + x0;
#if !IT_Independent_Iterations
						iNSSIteration[1] = 1;
						for (Int j1 = iNSSWindow; (j1 > 1) && (iNSSIteration[1] <= iMaxNSSIteration); j1 /= 2){
							iNSSIteration[1]++;
							if (j1 == iNSSWindow){
								iCurrNSSCenterX[1] = iBestNSSCenterX[1] = iCols - 1;
								iCurrNSSCenterY[1] = iBestNSSCenterY[1] = 0;
							}
							else{
								iCurrNSSCenterX[1] = iBestNSSCenterX[1];
								iCurrNSSCenterY[1] = iBestNSSCenterY[1];
							}
							i1 = j1;
#else
							i1 = j0;
#endif
							//-------------------------------------- corner 1 ------------------------------------------------------//
							for (Int y1 = i1 / 2; y1 >= -i1 / 2; y1 -= i1 / 2) {
								iCurrCornerY[1] = iCurrNSSCenterY[1] + y1;
								for (Int x1 = i1 / 2; x1 >= -i1 / 2; x1 -= i1 / 2) {
									if((y1 != 0 && x1 == 0 ) || (y1 == 0 && x1 != 0 ) || (y1 == 0 && x1 == 0 )){
										iCurrCornerX[1] = iCurrNSSCenterX[1] + x1;
#if !IT_Independent_Iterations
										iNSSIteration[2] = 1;
										for (Int j2 = iNSSWindow; (j2 > 1) && (iNSSIteration[2] <= iMaxNSSIteration); j2 /= 2){
											iNSSIteration[2]++;
											if (j2 == iNSSWindow){
												iCurrNSSCenterX[2] = iBestNSSCenterX[2] = iCols - 1;
												iCurrNSSCenterY[2] = iBestNSSCenterY[2] = iRows - 1;
											}
											else{
												iCurrNSSCenterX[2] = iBestNSSCenterX[2];
												iCurrNSSCenterY[2] = iBestNSSCenterY[2];
											}
											i2 = j2;
#else
											i2 = j0;
#endif
											//-------------------------------------- corner 2 ------------------------------------------------------//
											for (Int y2 = i2 / 2; y2 >= -i2 / 2; y2 -= i2 / 2) {
												iCurrCornerY[2] = iCurrNSSCenterY[2] + y2;
												for (Int x2 = i2 / 2; x2 >= -i2 / 2; x2 -= i2 / 2) {
													if((y2 != 0 && x2 == 0 ) || (y2 == 0 && x2 != 0 ) || (y2 == 0 && x2 == 0 )){
														iCurrCornerX[2] = iCurrNSSCenterX[2] + x2;
#if !IT_Independent_Iterations
														iNSSIteration[3] = 1;
														for (Int j3 = iNSSWindow; (j3 > 1) && (iNSSIteration[3] <= iMaxNSSIteration); j3 /= 2){
															iNSSIteration[3]++;
															if (j3 == iNSSWindow){
																iCurrNSSCenterX[3] = iBestNSSCenterX[3] = 0;
																iCurrNSSCenterY[3] = iBestNSSCenterY[3] = iRows - 1;
															}
															else{
																iCurrNSSCenterX[3] = iBestNSSCenterX[3];
																iCurrNSSCenterY[3] = iBestNSSCenterY[3];
															}
															i3 = j3;
#else
															i3 = j0;
#endif
															//-------------------------------------- corner 3 ------------------------------------------------------//
															for (Int y3 = i3 / 2; y3 >= -i3 / 2; y3 -= i3 / 2) {
																iCurrCornerY[3] = iCurrNSSCenterY[3] + y3;
																for (Int x3 = i3 / 2; x3 >= -i3 / 2; x3 -= i3 / 2) {
																	if((y3 != 0 && x3 == 0 ) || (y3 == 0 && x3 != 0 ) || (y3 == 0 && x3 == 0 )){
																		iCurrCornerX[3] = iCurrNSSCenterX[3] + x3;
																		// if not translation
																		if ( !(x0 == x1  && x0 == x2 && x0 == x3 && y0 == y1 && y0 == y2 && y0 == y3) )
																		{
#if IT_GT_GRID_SIZE < 2
																			// valid GT location
																			if(((x0 + (Hor >> 2)        < -marginX  && y0 + (Ver >> 2)        <= -marginY)  || (x0 + (Hor >> 2)         >= -marginX  && y0 + (Ver >> 2)         < -marginY))  &&
																				((x1 + (Hor >> 2) +iCols < -marginX  && y1 + (Ver >> 2)        <= -marginY)  || (x1 + (Hor >> 2) +iCols  >= -marginX  && y1 + (Ver >> 2)         < -marginY))  &&
																				((x2 + (Hor >> 2) +iCols < -marginX  && y2 + (Ver >> 2) +iRows <= -marginY)  || (x2 + (Hor >> 2) +iCols  >= -marginX  && y2 + (Ver >> 2) +iRows  < -marginY))  &&
																				((x3 + (Hor >> 2)        < -marginX  && y3 + (Ver >> 2) +iRows <= -marginY)  || (x3 + (Hor >> 2)         >= -marginX  && y3 + (Ver >> 2) +iRows  < -marginY))    )
																			{
#else
																				// valid GT location
																				/*if((((x0 / IT_GT_GRID_SIZE) + (Hor >> 2)        < -marginX  && (y0 / IT_GT_GRID_SIZE) + (Ver >> 2)        <= -marginY)  || ((x0 / IT_GT_GRID_SIZE) + (Hor >> 2)         >= -marginX  && (y0 / IT_GT_GRID_SIZE) + (Ver >> 2)         < -marginY))  &&
																				(((x1 / IT_GT_GRID_SIZE) + (Hor >> 2) +iCols 	< -marginX  && (y1 / IT_GT_GRID_SIZE) + (Ver >> 2)        <= -marginY)  || ((x1 / IT_GT_GRID_SIZE) + (Hor >> 2) +iCols  >= -marginX  && (y1 / IT_GT_GRID_SIZE) + (Ver >> 2)         < -marginY))  &&
																				(((x2 / IT_GT_GRID_SIZE) + (Hor >> 2) +iCols 	< -marginX  && (y2 / IT_GT_GRID_SIZE) + (Ver >> 2) +iRows <= -marginY)  || ((x2 / IT_GT_GRID_SIZE) + (Hor >> 2) +iCols  >= -marginX  && (y2 / IT_GT_GRID_SIZE) + (Ver >> 2) +iRows  < -marginY))  &&
																				(((x3 / IT_GT_GRID_SIZE) + (Hor >> 2)        	< -marginX  && (y3 / IT_GT_GRID_SIZE) + (Ver >> 2) +iRows <= -marginY)  || ((x3 / IT_GT_GRID_SIZE) + (Hor >> 2)         >= -marginX  && (y3 / IT_GT_GRID_SIZE) + (Ver >> 2) +iRows  < -marginY))    )*/
																				if(1)
																				{
#endif
// calculate gt param
#if IT_GT_GRID_SIZE < 2
#if !IT_GT_BILINEAR_TRANSFORMATION
																					calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#else
																					calcParamBilinear(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#endif
#else
#if !IT_GT_BILINEAR_TRANSFORMATION
																				calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE);
#else
																				calcParamBilinear(iCurrCornerX, iCurrCornerY, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE);
#endif
#endif
#if IT_GT_AFFINE
#if !IT_GT_BILINEAR_TRANSFORMATION
																				if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#else
																					if(dProjective[3]==0.0 && dProjective[7]==0.0){ // AFFINE
#endif
#endif
#if IT_GT_GRID_SIZE < 2
#if !IT_GT_BILINEAR_TRANSFORMATION
																					ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
#else
																					BilinearTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
#endif
#else
#if !IT_GT_BILINEAR_TRANSFORMATION
																					ProjectiveTransform(piRefSrch, piAux, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE, piRefSrchStride, iNSSWindow);
#else
																					BilinearTransform(piRefSrch, piAux, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE, piRefSrchStride, iNSSWindow);
#endif
#endif
																					setDistParamComp(0);
																					m_cDistParam.pCur = piAux;
																					m_cDistParam.bitDepth = g_bitDepthY;
																					uiDist = m_cDistParam.DistFunc(&m_cDistParam);
																					uiDist += m_pcRdCost->getCost( Hor , Ver );
																					uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
#if IT_GT_GRID_SIZE < 2
																							(iCurrCornerX[0])/lastIterationStep				, (iCurrCornerY[0])/lastIterationStep			,
																							(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])        /lastIterationStep   ,
																							(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep,
																							(iCurrCornerX[3])/lastIterationStep				, (iCurrCornerY[3] - iRows +1)/lastIterationStep)
#else
																							(iCurrCornerX[0])/lastIterationStep								, (iCurrCornerY[0])/lastIterationStep			,
																							(iCurrCornerX[1] - iCols*IT_GT_GRID_SIZE +1)/lastIterationStep  , (iCurrCornerY[1])        /lastIterationStep   ,
																							(iCurrCornerX[2] - iCols*IT_GT_GRID_SIZE +1)/lastIterationStep  , (iCurrCornerY[2] - iRows*IT_GT_GRID_SIZE +1)/lastIterationStep,
																							(iCurrCornerX[3])/lastIterationStep								, (iCurrCornerY[3] - iRows*IT_GT_GRID_SIZE +1)/lastIterationStep)
#endif
																					);
																					// GT COST
																					// check if best
																					if (uiDist < uiDistBest)
																					{
																						uiDistBest = uiDist;
																						iBestCornerX[0] = iCurrCornerX[0];
																						iBestCornerX[1] = iCurrCornerX[1];
																						iBestCornerX[2] = iCurrCornerX[2];
																						iBestCornerX[3] = iCurrCornerX[3];
																						iBestCornerY[0] = iCurrCornerY[0];
																						iBestCornerY[1] = iCurrCornerY[1];
																						iBestCornerY[2] = iCurrCornerY[2];
																						iBestCornerY[3] = iCurrCornerY[3];
																						iBestNSSCenterX[0] = iCurrCornerX[0];
																						iBestNSSCenterX[1] = iCurrCornerX[1];
																						iBestNSSCenterX[2] = iCurrCornerX[2];
																						iBestNSSCenterX[3] = iCurrCornerX[3];
																						iBestNSSCenterY[0] = iCurrCornerY[0];
																						iBestNSSCenterY[1] = iCurrCornerY[1];
																						iBestNSSCenterY[2] = iCurrCornerY[2];
																						iBestNSSCenterY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
																						iBestSSX = Hor;
																						iBestSSY = Ver;
#endif
																					}
#if IT_GT_AFFINE
																				}
#endif
																			}
																		}
																	}
																}
															} // CORNER 3
#if !IT_Independent_Iterations
														}
#endif
													}
												}
											} // CORNER 2
#if !IT_Independent_Iterations
										}
#endif
									}
								}
							} // CORNER 1
#if !IT_Independent_Iterations
						}
#endif
					}
				}
			} // CORNER 0
		}
#if IT_SS_NUMBER_OF_BEST_CAND
	}
#endif

#if IT_GT_GRID_SIZE < 2
#if !IT_GT_BILINEAR_TRANSFORMATION
	calcParamProjective(iBestCornerX, iBestCornerY, dProjective, iCols, iRows);
	ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
#else
	calcParamBilinear(iBestCornerX, iBestCornerY, dProjective, iCols, iRows);
	BilinearTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
#endif
#else
#if !IT_GT_BILINEAR_TRANSFORMATION
	calcParamProjective(iBestCornerX, iBestCornerY, dProjective, iCols * IT_GT_GRID_SIZE, iRows  * IT_GT_GRID_SIZE);
	ProjectiveTransform(piRefSrch, piAux, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE, piRefSrchStride, iNSSWindow);
#else
	calcParamBilinear(iBestCornerX, iBestCornerY, dProjective, iCols * IT_GT_GRID_SIZE, iRows  * IT_GT_GRID_SIZE);
	BilinearTransform(piRefSrch, piAux, dProjective, iCols * IT_GT_GRID_SIZE, iRows * IT_GT_GRID_SIZE, piRefSrchStride, iNSSWindow);
#endif
#endif
	xFree(piAux);


	if (iBestCornerX[0] != 0 || iBestCornerY[0] != 0 ||
			iBestCornerX[1] != 0 || iBestCornerY[1] != 0 ||
			iBestCornerX[2] != 0 || iBestCornerY[2] != 0 ||
			iBestCornerX[3] != 0 || iBestCornerY[3] != 0 )
	{
		gtFlag = true;
#if IT_GT_GRID_SIZE < 2
		rcGT0->set((iBestCornerX[0])        /lastIterationStep, (iBestCornerY[0])        /lastIterationStep);
		rcGT1->set((iBestCornerX[1] - iCols +1)/lastIterationStep, (iBestCornerY[1])        /lastIterationStep);
		rcGT2->set((iBestCornerX[2] - iCols +1)/lastIterationStep, (iBestCornerY[2] - iRows +1)/lastIterationStep);
		rcGT3->set((iBestCornerX[3])        /lastIterationStep, (iBestCornerY[3] - iRows +1)/lastIterationStep);
#else
		rcGT0->set((iBestCornerX[0])        /lastIterationStep, 					(iBestCornerY[0])        /lastIterationStep);
		rcGT1->set((iBestCornerX[1] - iCols*IT_GT_GRID_SIZE +1)/lastIterationStep,  (iBestCornerY[1])        /lastIterationStep);
		rcGT2->set((iBestCornerX[2] - iCols*IT_GT_GRID_SIZE +1)/lastIterationStep,  (iBestCornerY[2] - iRows*IT_GT_GRID_SIZE +1)/lastIterationStep);
		rcGT3->set((iBestCornerX[3])        /lastIterationStep, 					(iBestCornerY[3] - iRows*IT_GT_GRID_SIZE +1)/lastIterationStep);
#endif
		ruiCost = uiDistBest;
#if IT_SS_NUMBER_OF_BEST_CAND
		pcMvInt->set(iBestSSX >> 2,iBestSSY >> 2);
		rcMvHalf->set(0,0);
		rcMvQter->set(0,0);
		//cout << "best" << " " << iBestSSY << " " << iBestSSX << endl;
#endif
	}else{
		gtFlag = false;
		rcGT0->set(0,0);
		rcGT1->set(0,0);
		rcGT2->set(0,0);
		rcGT3->set(0,0);
	}
#endif
#if IT_GT_SEARCH == 4
	Int N = iNSSWindow >> 1;
#if IT_SS_NUMBER_OF_BEST_CAND
	Int iBestSSX = 0;
	Int iBestSSY = 0;
#if IT_SS_OVERWRITE_BEST_CAND
	Int W = 128;
	TComMv test;
	Int unusedCand = 0;
	Int i = 0;
	for(Int y = -W; y < 0; y+=4)
	{
		for(Int x = -W; x < W; x+=4)
		{
			test.set(x,y);
			if(m_pcRdCost->isValidPattern( piRefY, iRefStride, test, iCols, iRows))
			{
				bestSSCand[i].set(x,y);
				i++;
#if IT_DEBUG
				cout << x << " " << y << " | " ;
#endif
			}
			else
			{
				unusedCand++;
			}
		}
	}

#endif
#if IT_DEBUG
	cout << endl;
	cout << IT_SS_NUMBER_OF_BEST_CAND - unusedCand << endl;
#endif
	TComMv nullVect = TComMv(0,0);
	for(Int b = 0; b < IT_SS_NUMBER_OF_BEST_CAND
#if IT_SS_OVERWRITE_BEST_CAND
	- unusedCand
#endif
	; b++)
	{
		Ver = bestSSCand[b].getVer();
		Hor = bestSSCand[b].getHor();
		Ver <<= 2;
		Hor <<= 2;

		iOffset = bestSSCand[b].getHor() - iCols/2 + (bestSSCand[b].getVer() - iRows/2) * iRefStride;
		cPatternRoi.initPattern( piRefY +  iOffset, NULL, NULL, iCols * 2, iRows * 2, iRefStride, 0, 0 );
		xExtDIFUpSamplingH ( &cPatternRoi, biPred );
		xExtDIFUpSamplingQ ( &cPatternRoi, nullVect, biPred );
		piRefSrch = m_filteredBlock[ 0 ][ 0 ].getLumaAddr();
		piRefSrch += (iCols/2) + (iRows/2) * piRefSrchStride; // inside block (jump the margin pixels)
#endif
	// 0
	iCurrCornerY[0] = iCurrNSSCenterY[0] + N; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 1
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0] + N; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 2
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1] + N; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 3
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1] + N; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 4
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2] + N; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 5
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2] + N; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 6
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3] + N; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 7
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3] + N;
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 8
	iCurrCornerY[0] = iCurrNSSCenterY[0] - N; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 9
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0] - N; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 10
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1] - N; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 11
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1] - N; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 12
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2] - N; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 13
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2] - N; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 14
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3] - N; iCurrCornerX[3] = iCurrNSSCenterX[3];
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
	// 15
	iCurrCornerY[0] = iCurrNSSCenterY[0]; iCurrCornerX[0] = iCurrNSSCenterX[0]; iCurrCornerY[1] = iCurrNSSCenterY[1]; iCurrCornerX[1] = iCurrNSSCenterX[1]; iCurrCornerY[2] = iCurrNSSCenterY[2]; iCurrCornerX[2] = iCurrNSSCenterX[2]; iCurrCornerY[3] = iCurrNSSCenterY[3]; iCurrCornerX[3] = iCurrNSSCenterX[3] - N;
	// calculate gt param
	calcParamProjective(iCurrCornerX, iCurrCornerY, dProjective, iCols, iRows);
#if IT_GT_AFFINE
	if(dProjective[2]==0.0 && dProjective[5]==0.0){ // AFFINE
#endif
		ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
		setDistParamComp(0);
		m_cDistParam.pCur = piAux;
		m_cDistParam.bitDepth = g_bitDepthY;
		uiDist = m_cDistParam.DistFunc(&m_cDistParam);
		uiDist += m_pcRdCost->getCost( Hor , Ver );
		uiDist += m_pcRdCost->getCost(m_pcRdCost->getBitsGT(
				(iCurrCornerX[0])/lastIterationStep			    , (iCurrCornerY[0])/lastIterationStep			  ,
				(iCurrCornerX[1] - iCols +1)/lastIterationStep  , (iCurrCornerY[1])/lastIterationStep             ,
				(iCurrCornerX[2] - iCols +1)/lastIterationStep  , (iCurrCornerY[2] - iRows +1)/lastIterationStep  ,
				(iCurrCornerX[3])/lastIterationStep			    , (iCurrCornerY[3] - iRows +1)/lastIterationStep  )
		);
		// GT COST
		// check if best
		if (uiDist < uiDistBest)
		{
			uiDistBest = uiDist;
			iBestCornerX[0] = iCurrCornerX[0];
			iBestCornerX[1] = iCurrCornerX[1];
			iBestCornerX[2] = iCurrCornerX[2];
			iBestCornerX[3] = iCurrCornerX[3];
			iBestCornerY[0] = iCurrCornerY[0];
			iBestCornerY[1] = iCurrCornerY[1];
			iBestCornerY[2] = iCurrCornerY[2];
			iBestCornerY[3] = iCurrCornerY[3];
#if IT_SS_NUMBER_OF_BEST_CAND
			iBestSSX = Hor;
			iBestSSY = Ver;
#endif
		}
#if IT_GT_AFFINE
	}
#endif
#if IT_SS_NUMBER_OF_BEST_CAND
	}
#endif
	//calcParamProjective(iBestCornerX, iBestCornerY, dProjective, iCols, iRows);
	//ProjectiveTransform(piRefSrch, piAux, dProjective, iCols, iRows, piRefSrchStride, iNSSWindow);
	xFree(piAux);


	if (iBestCornerX[0] != 0 || iBestCornerY[0] != 0 ||
			iBestCornerX[1] != 0 || iBestCornerY[1] != 0 ||
			iBestCornerX[2] != 0 || iBestCornerY[2] != 0 ||
			iBestCornerX[3] != 0 || iBestCornerY[3] != 0 )
	{
		gtFlag = true;
		rcGT0->set((iBestCornerX[0])        /lastIterationStep, (iBestCornerY[0])        /lastIterationStep);
		rcGT1->set((iBestCornerX[1] - iCols +1)/lastIterationStep, (iBestCornerY[1])        /lastIterationStep);
		rcGT2->set((iBestCornerX[2] - iCols +1)/lastIterationStep, (iBestCornerY[2] - iRows +1)/lastIterationStep);
		rcGT3->set((iBestCornerX[3])        /lastIterationStep, (iBestCornerY[3] - iRows +1)/lastIterationStep);
		ruiCost = uiDistBest;
#if IT_SS_NUMBER_OF_BEST_CAND
		pcMvInt->set(iBestSSX >> 2,iBestSSY >> 2);
		rcMvHalf->set(0,0);
		rcMvQter->set(0,0);
#endif
#if IT_DEBUG
		cout << "SS + GT is best" << endl;
#endif
	}else{
		gtFlag = false;
		rcGT0->set(0,0);
		rcGT1->set(0,0);
		rcGT2->set(0,0);
		rcGT3->set(0,0);
	}

#endif
#endif
}
#endif



Void TEncSearch::xSetSearchRange ( TComDataCU* pcCU, TComMv& cMvPred, Int iSrchRng, TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB )
{
  Int  iMvShift = 2;
  TComMv cTmpMvPred = cMvPred;
  pcCU->clipMv( cTmpMvPred );

  rcMvSrchRngLT.setHor( cTmpMvPred.getHor() - (iSrchRng << iMvShift) );
  rcMvSrchRngLT.setVer( cTmpMvPred.getVer() - (iSrchRng << iMvShift) );
  
  rcMvSrchRngRB.setHor( cTmpMvPred.getHor() + (iSrchRng << iMvShift) );
  rcMvSrchRngRB.setVer( cTmpMvPred.getVer() + (iSrchRng << iMvShift) );
  pcCU->clipMv        ( rcMvSrchRngLT );
  pcCU->clipMv        ( rcMvSrchRngRB );
  
  rcMvSrchRngLT >>= iMvShift;
  rcMvSrchRngRB >>= iMvShift;
}

#if IT_HOLOSS
// ************** For IT development (non-scalable codec) *****************
Void TEncSearch::xSetSearchRange ( TComDataCU* pcCU, TComMv& pcMvSrchRngLT, TComMv& pcMvSrchRngRB, Int& riOffsetX, Int& riOffsetY, Bool  isFirstRow, Bool isFirstCol)
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT.getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB.getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT.getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB.getVer();

  if (isFirstCol && isFirstRow)
  {
    iSrchRngHorRight = iSrchRngHorLeft + 1;
    iSrchRngVerTop = iSrchRngVerBottom + 1;
  }
  else
  {
    iSrchRngVerBottom = (iSrchRngVerBottom > (-riOffsetY-4)) ? (-riOffsetY-4) : iSrchRngVerBottom;
    riOffsetX = - riOffsetX - (Int)pcCU->getWidth(0)  - 4;
    riOffsetY = - riOffsetY - (Int)pcCU->getHeight(0) - 4;
    iSrchRngVerBottom = ( isFirstCol && (iSrchRngVerBottom > riOffsetY) ) ? riOffsetY : iSrchRngVerBottom;
    iSrchRngHorRight  = ( isFirstRow && (iSrchRngHorRight > riOffsetX) )  ? riOffsetX : iSrchRngHorRight;
    iSrchRngHorRight  = (!isFirstRow && (pcCU->getAddr() < pcCU->getPic()->getFrameWidthInCU()) && ( iSrchRngHorRight > ( riOffsetX + (pcCU->getWidth(0)<<1) ) ) ) ? ( riOffsetX + (pcCU->getWidth(0)<<1) ) : iSrchRngHorRight;
  }

  pcMvSrchRngLT.setHor( iSrchRngHorLeft );
  pcMvSrchRngLT.setVer( iSrchRngVerTop );
  pcMvSrchRngRB.setHor( iSrchRngHorRight );
  pcMvSrchRngRB.setVer( iSrchRngVerBottom );

  // Clip again to avoid error. It needs a shift because the clip is done in 1/4 pel!!
  Int  iMvShift = 2;
  pcMvSrchRngLT <<= iMvShift;
  pcMvSrchRngRB <<= iMvShift;
  pcCU->clipMv        ( pcMvSrchRngLT ); 
  pcCU->clipMv        ( pcMvSrchRngRB );
  pcMvSrchRngLT >>= iMvShift;
  pcMvSrchRngRB >>= iMvShift;
}
#endif

Void TEncSearch::xPatternSearch(  TComPattern* pcPatternKey,
                                  Pel* piRefY,
                                  Int iRefStride,
                                  TComMv* pcMvSrchRngLT,
                                  TComMv* pcMvSrchRngRB,
                                  TComMv& rcMv,
                                  UInt& ruiSAD
#if IT_HOLOSS
                                ,Int     riOffsetX
                                ,Int     riOffsetY
#if IT_SS_NUMBER_OF_BEST_CAND
								,TComMv*		ssBestCand
#endif
								, Bool    isSSE
#endif
                               )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  UInt  uiSad;
  UInt  uiSadBest         = MAX_UINT;
  Int   iBestX = 0;
  Int   iBestY = 0;
  
  Pel*  piRefSrch;

#if IT_HOLOSS
  Bool isValid = false;
#endif
#if IT_SS_NUMBER_OF_BEST_CAND > 0
  Int usedBestCand = 0;
  Int bestCand = 0;
#endif

  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefY, iRefStride,  m_cDistParam );
  
  // fast encoder decision: use subsampled SAD for integer ME
  if ( m_pcEncCfg->getUseFastEnc() )
  {
    if ( m_cDistParam.iRows > 8 )
    {
      m_cDistParam.iSubShift = 1;
    }
  }
  
  piRefY += (iSrchRngVerTop * iRefStride);
  for ( Int y = iSrchRngVerTop; y <= iSrchRngVerBottom; y++ )
  {
    for ( Int x = iSrchRngHorLeft; x <= iSrchRngHorRight; x++ )
    {
      //  find min. distortion position
      piRefSrch = piRefY + x;
      m_cDistParam.pCur = piRefSrch;

      setDistParamComp(0);

      m_cDistParam.bitDepth = g_bitDepthY;
      uiSad = m_cDistParam.DistFunc( &m_cDistParam );
      
#if IT_HOLOSS
      if (isSSE)
      {
        if ( ( x >= riOffsetX ) && ( y > riOffsetY ) )
          continue;
        if( !m_pcRdCost->isValidPattern( &m_cDistParam,m_cDistParam.iCols) )
          continue;
      }
      isValid = true;
#endif      
      // motion cost
      uiSad += m_pcRdCost->getCost( x, y );
      
      if ( uiSad < uiSadBest )
      {
        uiSadBest = uiSad;
        iBestX    = x;
        iBestY    = y;
#if IT_SS_NUMBER_OF_BEST_CAND > 0
        ssBestCand[bestCand].set(x, y);
        usedBestCand++;
        bestCand++;
        if(bestCand == IT_SS_NUMBER_OF_BEST_CAND)
        	bestCand = 0;
#endif
      }
    } // for x
    piRefY += iRefStride;
  } // for y

#if IT_HOLOSS
  if (!isValid)
  {
    ruiSAD = MAX_UINT;
    return;
  }
#endif

  rcMv.set( iBestX, iBestY );
  
  ruiSAD = uiSadBest - m_pcRdCost->getCost( iBestX, iBestY );
#if IT_SS_NUMBER_OF_BEST_CAND > 0
  //cout << usedBestCand << " ";
#endif

  return;
}

Void TEncSearch::xPatternSearchFast( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD )
{
  pcCU->getMvPredLeft       ( m_acMvPredictors[0] );
  pcCU->getMvPredAbove      ( m_acMvPredictors[1] );
  pcCU->getMvPredAboveRight ( m_acMvPredictors[2] );

  switch ( m_iFastSearch )
  {
    case 1:
      xTZSearch( pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD );
      break;

    default:
      break;
  }
}

Void TEncSearch::xTZSearch( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  TZ_SEARCH_CONFIGURATION

  UInt uiSearchRange = m_iSearchRange;
  pcCU->clipMv( rcMv );
  rcMv >>= 2;
  // init TZSearchStruct
  IntTZSearchStruct cStruct;
  cStruct.iYStride    = iRefStride;
  cStruct.piRefY      = piRefY;
  cStruct.uiBestSad   = MAX_UINT;

  // set rcMv (Median predictor) as start point and as best point
  xTZSearchHelp( pcPatternKey, cStruct, rcMv.getHor(), rcMv.getVer(), 0, 0 );

  // test whether one of PRED_A, PRED_B, PRED_C MV is better start point than Median predictor
  if ( bTestOtherPredictedMV )
  {
    for ( UInt index = 0; index < 3; index++ )
    {
      TComMv cMv = m_acMvPredictors[index];
      pcCU->clipMv( cMv );
      cMv >>= 2;
      xTZSearchHelp( pcPatternKey, cStruct, cMv.getHor(), cMv.getVer(), 0, 0 );
    }
  }

  // test whether zero Mv is better start point than Median predictor
  if ( bTestZeroVector )
  {
    xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
  }

  // start search
  Int  iDist = 0;
  Int  iStartX = cStruct.iBestX;
  Int  iStartY = cStruct.iBestY;

  // first search
  for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
  {
    if ( bFirstSearchDiamond == 1 )
    {
      xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
    }
    else
    {
      xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
    }

    if ( bFirstSearchStop && ( cStruct.uiBestRound >= uiFirstSearchRounds ) ) // stop criterion
    {
      break;
    }
  }

  // test whether zero Mv is a better start point than Median predictor
  if ( bTestZeroVectorStart && ((cStruct.iBestX != 0) || (cStruct.iBestY != 0)) )
  {
    xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
    if ( (cStruct.iBestX == 0) && (cStruct.iBestY == 0) )
    {
      // test its neighborhood
      for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
      {
        xTZ8PointDiamondSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, 0, 0, iDist );
        if ( bTestZeroVectorStop && (cStruct.uiBestRound > 0) ) // stop criterion
        {
          break;
        }
      }
    }
  }

  // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
  if ( cStruct.uiBestDistance == 1 )
  {
    cStruct.uiBestDistance = 0;
    xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
  }

  // raster search if distance is too big
  if ( bEnableRasterSearch && ( ((Int)(cStruct.uiBestDistance) > iRaster) || bAlwaysRasterSearch ) )
  {
    cStruct.uiBestDistance = iRaster;
    for ( iStartY = iSrchRngVerTop; iStartY <= iSrchRngVerBottom; iStartY += iRaster )
    {
      for ( iStartX = iSrchRngHorLeft; iStartX <= iSrchRngHorRight; iStartX += iRaster )
      {
        xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, iRaster );
      }
    }
  }

  // raster refinement
  if ( bRasterRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      if ( cStruct.uiBestDistance > 1 )
      {
        iDist = cStruct.uiBestDistance >>= 1;
        if ( bRasterRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
      }

      // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }
  
  // start refinement
  if ( bStarRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      cStruct.uiBestDistance = 0;
      cStruct.ucPointNr = 0;
      for ( iDist = 1; iDist < (Int)uiSearchRange + 1; iDist*=2 )
      {
        if ( bStarRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        if ( bStarRefinementStop && (cStruct.uiBestRound >= uiStarRefinementRounds) ) // stop criterion
        {
          break;
        }
      }

      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }

  // write out best match
  rcMv.set( cStruct.iBestX, cStruct.iBestY );
  ruiSAD = cStruct.uiBestSad - m_pcRdCost->getCost( cStruct.iBestX, cStruct.iBestY );
}

Void TEncSearch::xPatternSearchFracDIF(TComDataCU* pcCU,
                                       TComPattern* pcPatternKey,
                                       Pel* piRefY,
                                       Int iRefStride,
                                       TComMv* pcMvInt,
                                       TComMv& rcMvHalf,
                                       TComMv& rcMvQter,
                                       UInt& ruiCost
                                       ,Bool biPred
                                       )
{
  //  Reference pattern initialization (integer scale)
  TComPattern cPatternRoi;
  Int         iOffset    = pcMvInt->getHor() + pcMvInt->getVer() * iRefStride;
  cPatternRoi.initPattern( piRefY +  iOffset,
                          NULL,
                          NULL,
                          pcPatternKey->getROIYWidth(),
                          pcPatternKey->getROIYHeight(),
                          iRefStride,
                          0, 0 );

#if IT_HOLOSS
  // DEBUG
  assert(m_pcRdCost->isValidPattern(&cPatternRoi));
#endif

  //  Half-pel refinement
  xExtDIFUpSamplingH ( &cPatternRoi, biPred );

  rcMvHalf = *pcMvInt;   rcMvHalf <<= 1;    // for mv-cost
  TComMv baseRefMv(0, 0);

  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 2, rcMvHalf   );

  m_pcRdCost->setCostScale( 0 );

  xExtDIFUpSamplingQ ( &cPatternRoi, rcMvHalf, biPred );
  baseRefMv = rcMvHalf;
  baseRefMv <<= 1;

  rcMvQter = *pcMvInt;   rcMvQter <<= 1;    // for mv-cost
  rcMvQter += rcMvHalf;  rcMvQter <<= 1;

  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 1, rcMvQter );

}

/** encode residual and calculate rate-distortion for a CU block
 * \param pcCU
 * \param pcYuvOrg
 * \param pcYuvPred
 * \param rpcYuvResi
 * \param rpcYuvResiBest
 * \param rpcYuvRec
 * \param bSkipRes
 * \returns Void
 */
Void TEncSearch::encodeResAndCalcRdInterCU( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred, TComYuv*& rpcYuvResi, TComYuv*& rpcYuvResiBest, TComYuv*& rpcYuvRec, Bool bSkipRes )
{
  if ( pcCU->isIntra(0) )
  {
    return;
  }
  
  Bool      bHighPass    = pcCU->getSlice()->getDepth() ? true : false;
  UInt      uiBits       = 0, uiBitsBest = 0;
  UInt      uiDistortion = 0, uiDistortionBest = 0;
  
  UInt      uiWidth      = pcCU->getWidth ( 0 );
  UInt      uiHeight     = pcCU->getHeight( 0 );
  
  //  No residual coding : SKIP mode
  if ( bSkipRes )
  {
    pcCU->setSkipFlagSubParts( true, 0, pcCU->getDepth(0) );

    rpcYuvResi->clear();
    
    pcYuvPred->copyToPartYuv( rpcYuvRec, 0 );
    
    uiDistortion = m_pcRdCost->getDistPart(g_bitDepthY, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
    + m_pcRdCost->getDistPart(g_bitDepthC, rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, TEXT_CHROMA_U )
    + m_pcRdCost->getDistPart(g_bitDepthC, rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, TEXT_CHROMA_V );

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);
    
    m_pcEntropyCoder->resetBits();
    if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, true );
    
    uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getTotalBits()       = uiBits;
    pcCU->getTotalDistortion() = uiDistortion;
    pcCU->getTotalCost()       = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
    
    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);
    
    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
    pcCU->setTrIdxSubParts( 0, 0, pcCU->getDepth(0) );
    
    return;
  }
  
  //  Residual coding.
  Int    qp, qpBest = 0, qpMin, qpMax;
  Double  dCost, dCostBest = MAX_DOUBLE;
  
  UInt uiTrLevel = 0;
  if( (pcCU->getWidth(0) > pcCU->getSlice()->getSPS()->getMaxTrSize()) )
  {
    while( pcCU->getWidth(0) > (pcCU->getSlice()->getSPS()->getMaxTrSize()<<uiTrLevel) ) uiTrLevel++;
  }
  UInt uiMaxTrMode = 1 + uiTrLevel;
  
  while((uiWidth>>uiMaxTrMode) < (g_uiMaxCUWidth>>g_uiMaxCUDepth)) uiMaxTrMode--;
  
  qpMin =  bHighPass ? Clip3( -pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, pcCU->getQP(0) - m_iMaxDeltaQP ) : pcCU->getQP( 0 );
  qpMax =  bHighPass ? Clip3( -pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, pcCU->getQP(0) + m_iMaxDeltaQP ) : pcCU->getQP( 0 );

  rpcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, uiWidth );

  for ( qp = qpMin; qp <= qpMax; qp++ )
  {
    dCost = 0.;
    uiBits = 0;
    uiDistortion = 0;
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_CURR_BEST ] );
    
    UInt uiZeroDistortion = 0;
    xEstimateResidualQT( pcCU, 0, 0, 0, rpcYuvResi,  pcCU->getDepth(0), dCost, uiBits, uiDistortion, &uiZeroDistortion );
    
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeQtRootCbfZero( pcCU );
    UInt zeroResiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    Double dZeroCost = m_pcRdCost->calcRdCost( zeroResiBits, uiZeroDistortion );
    if(pcCU->isLosslessCoded( 0 ))
    {  
      dZeroCost = dCost + 1;
    }
    if ( dZeroCost < dCost )
    {
      dCost        = dZeroCost;
      uiBits       = 0;
      uiDistortion = uiZeroDistortion;
      
      const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
      ::memset( pcCU->getTransformIdx()      , 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_LUMA )    , 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_CHROMA_U ), 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_CHROMA_V ), 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCoeffY()            , 0, uiWidth * uiHeight * sizeof( TCoeff )      );
      ::memset( pcCU->getCoeffCb()           , 0, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
      ::memset( pcCU->getCoeffCr()           , 0, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
      pcCU->setTransformSkipSubParts ( 0, 0, 0, 0, pcCU->getDepth(0) );
    }
    else
    {
      xSetResidualQTData( pcCU, 0, 0, 0, NULL, pcCU->getDepth(0), false );
    }
    
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST] );
    
    uiBits = 0;
    {
      TComYuv *pDummy = NULL;
      xAddSymbolBitsInter( pcCU, 0, 0, uiBits, pDummy, NULL, pDummy );
    }
    
    
    Double dExactCost = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
    dCost = dExactCost;
    
    if ( dCost < dCostBest )
    {
      if ( !pcCU->getQtRootCbf( 0 ) )
      {
        rpcYuvResiBest->clear();
      }
      else
      {
        xSetResidualQTData( pcCU, 0, 0, 0, rpcYuvResiBest, pcCU->getDepth(0), true );
      }
      
      if( qpMin != qpMax && qp != qpMax )
      {
        const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
        ::memcpy( m_puhQTTempTrIdx, pcCU->getTransformIdx(),        uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA ),     uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_pcQTTempCoeffY,  pcCU->getCoeffY(),  uiWidth * uiHeight * sizeof( TCoeff )      );
        ::memcpy( m_pcQTTempCoeffCb, pcCU->getCoeffCb(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
        ::memcpy( m_pcQTTempCoeffCr, pcCU->getCoeffCr(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
#if ADAPTIVE_QP_SELECTION
        ::memcpy( m_pcQTTempArlCoeffY,  pcCU->getArlCoeffY(),  uiWidth * uiHeight * sizeof( Int )      );
        ::memcpy( m_pcQTTempArlCoeffCb, pcCU->getArlCoeffCb(), uiWidth * uiHeight * sizeof( Int ) >> 2 );
        ::memcpy( m_pcQTTempArlCoeffCr, pcCU->getArlCoeffCr(), uiWidth * uiHeight * sizeof( Int ) >> 2 );
#endif
        ::memcpy( m_puhQTTempTransformSkipFlag[0], pcCU->getTransformSkip(TEXT_LUMA),     uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[1], pcCU->getTransformSkip(TEXT_CHROMA_U), uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempTransformSkipFlag[2], pcCU->getTransformSkip(TEXT_CHROMA_V), uiQPartNum * sizeof( UChar ) );
      }
      uiBitsBest       = uiBits;
      uiDistortionBest = uiDistortion;
      dCostBest        = dCost;
      qpBest           = qp;
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );
    }
  }
  
  assert ( dCostBest != MAX_DOUBLE );
  
  if( qpMin != qpMax && qpBest != qpMax )
  {
    assert( 0 ); // check
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );

    // copy best cbf and trIdx to pcCU
    const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
    ::memcpy( pcCU->getTransformIdx(),       m_puhQTTempTrIdx,  uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_LUMA ),     m_puhQTTempCbf[0], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ), m_puhQTTempCbf[1], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ), m_puhQTTempCbf[2], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCoeffY(),  m_pcQTTempCoeffY,  uiWidth * uiHeight * sizeof( TCoeff )      );
    ::memcpy( pcCU->getCoeffCb(), m_pcQTTempCoeffCb, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
    ::memcpy( pcCU->getCoeffCr(), m_pcQTTempCoeffCr, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
#if ADAPTIVE_QP_SELECTION
    ::memcpy( pcCU->getArlCoeffY(),  m_pcQTTempArlCoeffY,  uiWidth * uiHeight * sizeof( Int )      );
    ::memcpy( pcCU->getArlCoeffCb(), m_pcQTTempArlCoeffCb, uiWidth * uiHeight * sizeof( Int ) >> 2 );
    ::memcpy( pcCU->getArlCoeffCr(), m_pcQTTempArlCoeffCr, uiWidth * uiHeight * sizeof( Int ) >> 2 );
#endif
    ::memcpy( pcCU->getTransformSkip(TEXT_LUMA),     m_puhQTTempTransformSkipFlag[0], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getTransformSkip(TEXT_CHROMA_U), m_puhQTTempTransformSkipFlag[1], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getTransformSkip(TEXT_CHROMA_V), m_puhQTTempTransformSkipFlag[2], uiQPartNum * sizeof( UChar ) );
  }
  rpcYuvRec->addClip ( pcYuvPred, rpcYuvResiBest, 0, uiWidth );
  
  // update with clipped distortion and cost (qp estimation loop uses unclipped values)
    uiDistortionBest = m_pcRdCost->getDistPart(g_bitDepthY, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
    + m_pcRdCost->getDistPart(g_bitDepthC, rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, TEXT_CHROMA_U )
    + m_pcRdCost->getDistPart(g_bitDepthC, rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, TEXT_CHROMA_V );
  dCostBest = m_pcRdCost->calcRdCost( uiBitsBest, uiDistortionBest );
  
  pcCU->getTotalBits()       = uiBitsBest;
  pcCU->getTotalDistortion() = uiDistortionBest;
  pcCU->getTotalCost()       = dCostBest;
  
  if ( pcCU->isSkipped(0) )
  {
    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
  }
  
  pcCU->setQPSubParts( qpBest, 0, pcCU->getDepth(0) );
}

Void TEncSearch::xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, UInt &ruiDist, UInt *puiZeroDist )
{
  const UInt uiTrMode = uiDepth - pcCU->getDepth( 0 );
  
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;
  
  UInt SplitFlag = ((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && ( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N ));
  Bool bCheckFull;
  if ( SplitFlag && uiDepth == pcCU->getDepth(uiAbsPartIdx) && ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) ) )
  {
     bCheckFull = false;
  }
  else
  {
     bCheckFull =  ( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
  }
  
  const Bool bCheckSplit  = ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
  
  assert( bCheckFull || bCheckSplit );
  
  Bool  bCodeChroma   = true;
  UInt  uiTrModeC     = uiTrMode;
  UInt  uiLog2TrSizeC = uiLog2TrSize-1;
  if( uiLog2TrSize == 2 )
  {
    uiLog2TrSizeC++;
    uiTrModeC    --;
    UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
    bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
  }
  
  const UInt uiSetCbf = 1 << uiTrMode;
  // code full block
  Double dSingleCost = MAX_DOUBLE;
  UInt uiSingleBits = 0;
  UInt uiSingleDist = 0;
  UInt uiAbsSumY = 0, uiAbsSumU = 0, uiAbsSumV = 0;
  UInt uiBestTransformMode[3] = {0};

  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
  
  if( bCheckFull )
  {
    const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
#if ADAPTIVE_QP_SELECTION    
    Int *pcArlCoeffCurrY = m_ppcQTTempArlCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    Int *pcArlCoeffCurrU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    Int *pcArlCoeffCurrV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);   
#endif
    
    Int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
    UInt absTUPartIdxC = uiAbsPartIdx;

    trWidth  = trHeight  = 1 << uiLog2TrSize;
    trWidthC = trHeightC = 1 <<uiLog2TrSizeC;
    pcCU->setTrIdxSubParts( uiDepth - pcCU->getDepth( 0 ), uiAbsPartIdx, uiDepth );
    Double minCostY = MAX_DOUBLE;
    Double minCostU = MAX_DOUBLE;
    Double minCostV = MAX_DOUBLE;
    Bool checkTransformSkipY  = pcCU->getSlice()->getPPS()->getUseTransformSkip() && trWidth == 4 && trHeight == 4;
    Bool checkTransformSkipUV = pcCU->getSlice()->getPPS()->getUseTransformSkip() && trWidthC == 4 && trHeightC == 4;

    checkTransformSkipY         &= (!pcCU->isLosslessCoded(0));
    checkTransformSkipUV        &= (!pcCU->isLosslessCoded(0));

    pcCU->setTransformSkipSubParts ( 0, TEXT_LUMA, uiAbsPartIdx, uiDepth ); 
    if( bCodeChroma )
    {
      pcCU->setTransformSkipSubParts ( 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 
      pcCU->setTransformSkipSubParts ( 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 
    }

    if (m_pcEncCfg->getUseRDOQ())
    {
      m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidth, trHeight, TEXT_LUMA );        
    }

    m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

#if RDOQ_CHROMA_LAMBDA 
    m_pcTrQuant->selectLambda(TEXT_LUMA);  
#endif
    m_pcTrQuant->transformNxN( pcCU, pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride (), pcCoeffCurrY, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrY, 
#endif      
                                 trWidth,   trHeight,    uiAbsSumY, TEXT_LUMA,     uiAbsPartIdx );
    
    pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    
    if( bCodeChroma )
    {
      if (m_pcEncCfg->getUseRDOQ())
      {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidthC, trHeightC, TEXT_CHROMA );          
      }

      Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

#if RDOQ_CHROMA_LAMBDA 
      m_pcTrQuant->selectLambda(TEXT_CHROMA_U);
#endif

      m_pcTrQuant->transformNxN( pcCU, pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrU, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrU, 
#endif        
                                 trWidthC, trHeightC, uiAbsSumU, TEXT_CHROMA_U, uiAbsPartIdx );

      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );
      
#if RDOQ_CHROMA_LAMBDA
      m_pcTrQuant->selectLambda(TEXT_CHROMA_V);
#endif

      m_pcTrQuant->transformNxN( pcCU, pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrV,
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrV, 
#endif        
                                 trWidthC, trHeightC, uiAbsSumV, TEXT_CHROMA_V, uiAbsPartIdx );

      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }
    
    m_pcEntropyCoder->resetBits();
    
    m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    
    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx,  trWidth,  trHeight,    uiDepth, TEXT_LUMA );
    const UInt uiSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
    
    UInt uiSingleBitsU = 0;
    UInt uiSingleBitsV = 0;
    if( bCodeChroma )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U );
      uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();
      
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V );
      uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();
    }
    
    const UInt uiNumSamplesLuma = 1 << (uiLog2TrSize<<1);
    const UInt uiNumSamplesChro = 1 << (uiLog2TrSizeC<<1);
    
    ::memset( m_pTempPel, 0, sizeof( Pel ) * uiNumSamplesLuma ); // not necessary needed for inside of recursion (only at the beginning)
    
    UInt uiDistY = m_pcRdCost->getDistPart(g_bitDepthY, m_pTempPel, trWidth, pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride(), trWidth, trHeight ); // initialized with zero residual destortion

    if ( puiZeroDist )
    {
      *puiZeroDist += uiDistY;
    }
    if( uiAbsSumY )
    {
      Pel *pcResiCurrY = m_pcQTTempTComYuv[ uiQTTempAccessLayer ].getLumaAddr( absTUPartIdx );

      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

      Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
      assert(scalingListType < SCALING_LIST_NUM);
      m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA,REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType );//this is for inter mode only
      
      const UInt uiNonzeroDistY = m_pcRdCost->getDistPart(g_bitDepthY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( absTUPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),
      pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride(), trWidth,trHeight );
      if (pcCU->isLosslessCoded(0)) 
      {
        uiDistY = uiNonzeroDistY;
      }
      else
      {
        const Double singleCostY = m_pcRdCost->calcRdCost( uiSingleBitsY, uiNonzeroDistY );
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_LUMA,     uiTrMode );
        const UInt uiNullBitsY   = m_pcEntropyCoder->getNumberOfWrittenBits();
        const Double nullCostY   = m_pcRdCost->calcRdCost( uiNullBitsY, uiDistY );
        if( nullCostY < singleCostY )  
        {    
          uiAbsSumY = 0;
          ::memset( pcCoeffCurrY, 0, sizeof( TCoeff ) * uiNumSamplesLuma );
          if( checkTransformSkipY )
          {
            minCostY = nullCostY;
          }
        }
        else
        {
          uiDistY = uiNonzeroDistY;
          if( checkTransformSkipY )
          {
            minCostY = singleCostY;
          }
        }
      }
    }
    else if( checkTransformSkipY )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_LUMA, uiTrMode );
      const UInt uiNullBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
      minCostY = m_pcRdCost->calcRdCost( uiNullBitsY, uiDistY );
    }

    if( !uiAbsSumY )
    {
      Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( absTUPartIdx );
      const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride();
      for( UInt uiY = 0; uiY < trHeight; ++uiY )
      {
        ::memset( pcPtr, 0, sizeof( Pel ) * trWidth );
        pcPtr += uiStride;
      } 
    }
    
    UInt uiDistU = 0;
    UInt uiDistV = 0;
    if( bCodeChroma )
    {
      uiDistU = m_pcRdCost->getDistPart(g_bitDepthC, m_pTempPel, trWidthC, pcResi->getCbAddr( absTUPartIdxC ), pcResi->getCStride(), trWidthC, trHeightC
                                        , TEXT_CHROMA_U
                                        ); // initialized with zero residual destortion
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistU;
      }
      if( uiAbsSumU )
      {
        Pel *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC );

        Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
        assert(scalingListType < SCALING_LIST_NUM);
        m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA,REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType  );
        
        const UInt uiNonzeroDistU = m_pcRdCost->getDistPart(g_bitDepthC, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                            pcResi->getCbAddr( absTUPartIdxC), pcResi->getCStride(), trWidthC, trHeightC
                                                            , TEXT_CHROMA_U
                                                            );

        if(pcCU->isLosslessCoded(0))  
        {
          uiDistU = uiNonzeroDistU;
        }
        else
        {
          const Double dSingleCostU = m_pcRdCost->calcRdCost( uiSingleBitsU, uiNonzeroDistU );
          m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_CHROMA_U,     uiTrMode );
          const UInt uiNullBitsU    = m_pcEntropyCoder->getNumberOfWrittenBits();
          const Double dNullCostU   = m_pcRdCost->calcRdCost( uiNullBitsU, uiDistU );
          if( dNullCostU < dSingleCostU )
          {
            uiAbsSumU = 0;
            ::memset( pcCoeffCurrU, 0, sizeof( TCoeff ) * uiNumSamplesChro );
            if( checkTransformSkipUV )
            {
              minCostU = dNullCostU;
            }
          }
          else
          {
            uiDistU = uiNonzeroDistU;
            if( checkTransformSkipUV )
            {
              minCostU = dSingleCostU;
            }
          }
        }
      }
      else if( checkTransformSkipUV )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_CHROMA_U, uiTrMode );
        const UInt uiNullBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();
        minCostU = m_pcRdCost->calcRdCost( uiNullBitsU, uiDistU );
      }
      if( !uiAbsSumU )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC );
          const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < trHeightC; ++uiY )
        {
          ::memset( pcPtr, 0, sizeof(Pel) * trWidthC );
          pcPtr += uiStride;
        }
      }
      
      uiDistV = m_pcRdCost->getDistPart(g_bitDepthC, m_pTempPel, trWidthC, pcResi->getCrAddr( absTUPartIdxC), pcResi->getCStride(), trWidthC, trHeightC
                                        , TEXT_CHROMA_V
                                        ); // initialized with zero residual destortion
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistV;
      }
      if( uiAbsSumV )
      {
        Pel *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC );
        Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
        assert(scalingListType < SCALING_LIST_NUM);
        m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA,REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType );
        
        const UInt uiNonzeroDistV = m_pcRdCost->getDistPart(g_bitDepthC, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                            pcResi->getCrAddr( absTUPartIdxC ), pcResi->getCStride(), trWidthC, trHeightC
                                                            , TEXT_CHROMA_V
                                                            );
        if (pcCU->isLosslessCoded(0)) 
        {
          uiDistV = uiNonzeroDistV;
        }
        else
        {
          const Double dSingleCostV = m_pcRdCost->calcRdCost( uiSingleBitsV, uiNonzeroDistV );
          m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_CHROMA_V,     uiTrMode );
          const UInt uiNullBitsV    = m_pcEntropyCoder->getNumberOfWrittenBits();
          const Double dNullCostV   = m_pcRdCost->calcRdCost( uiNullBitsV, uiDistV );
          if( dNullCostV < dSingleCostV )
          {
            uiAbsSumV = 0;
            ::memset( pcCoeffCurrV, 0, sizeof( TCoeff ) * uiNumSamplesChro );
            if( checkTransformSkipUV )
            {
              minCostV = dNullCostV;
            }
          }
          else
          {
            uiDistV = uiNonzeroDistV;
            if( checkTransformSkipUV )
            {
              minCostV = dSingleCostV;
            }
          }
        }
      }
      else if( checkTransformSkipUV )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbfZero( pcCU, TEXT_CHROMA_V, uiTrMode );
        const UInt uiNullBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();
        minCostV = m_pcRdCost->calcRdCost( uiNullBitsV, uiDistV );
      }
      if( !uiAbsSumV )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC );
        const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < trHeightC; ++uiY )
        {   
          ::memset( pcPtr, 0, sizeof(Pel) * trWidthC );
          pcPtr += uiStride;
        }
      }
    }
    pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    if( bCodeChroma )
    {
      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }

    if( checkTransformSkipY )
    {
      UInt uiNonzeroDistY, uiAbsSumTransformSkipY;
      Double dSingleCostY;

      Pel *pcResiCurrY = m_pcQTTempTComYuv[ uiQTTempAccessLayer ].getLumaAddr( absTUPartIdx );
      UInt resiYStride = m_pcQTTempTComYuv[ uiQTTempAccessLayer ].getStride();

      TCoeff bestCoeffY[32*32];
      memcpy( bestCoeffY, pcCoeffCurrY, sizeof(TCoeff) * uiNumSamplesLuma );
      
#if ADAPTIVE_QP_SELECTION
      TCoeff bestArlCoeffY[32*32];
      memcpy( bestArlCoeffY, pcArlCoeffCurrY, sizeof(TCoeff) * uiNumSamplesLuma );
#endif

      Pel bestResiY[32*32];
      for ( Int i = 0; i < trHeight; ++i )
      {
        memcpy( &bestResiY[i*trWidth], pcResiCurrY+i*resiYStride, sizeof(Pel) * trWidth );
      }

      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );

      pcCU->setTransformSkipSubParts ( 1, TEXT_LUMA, uiAbsPartIdx, uiDepth );

      if (m_pcEncCfg->getUseRDOQTS())
      {
        m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, trWidth, trHeight, TEXT_LUMA );        
      }

      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

#if RDOQ_CHROMA_LAMBDA 
      m_pcTrQuant->selectLambda(TEXT_LUMA);
#endif
      m_pcTrQuant->transformNxN( pcCU, pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride (), pcCoeffCurrY, 
#if ADAPTIVE_QP_SELECTION
        pcArlCoeffCurrY, 
#endif      
        trWidth,   trHeight,    uiAbsSumTransformSkipY, TEXT_LUMA, uiAbsPartIdx, true );
      pcCU->setCbfSubParts( uiAbsSumTransformSkipY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );

      if( uiAbsSumTransformSkipY != 0 )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiTrMode );
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA );
        const UInt uiTsSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
        assert(scalingListType < SCALING_LIST_NUM);

        m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA,REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType, true );

        uiNonzeroDistY = m_pcRdCost->getDistPart(g_bitDepthY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( absTUPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),
          pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride(), trWidth, trHeight );

        dSingleCostY = m_pcRdCost->calcRdCost( uiTsSingleBitsY, uiNonzeroDistY );
      }

      if( !uiAbsSumTransformSkipY || minCostY < dSingleCostY )
      {
        pcCU->setTransformSkipSubParts ( 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
        memcpy( pcCoeffCurrY, bestCoeffY, sizeof(TCoeff) * uiNumSamplesLuma );
#if ADAPTIVE_QP_SELECTION
        memcpy( pcArlCoeffCurrY, bestArlCoeffY, sizeof(TCoeff) * uiNumSamplesLuma );
#endif
        for( Int i = 0; i < trHeight; ++i )
        {
          memcpy( pcResiCurrY+i*resiYStride, &bestResiY[i*trWidth], sizeof(Pel) * trWidth );
        }
      }
      else
      {
        uiDistY = uiNonzeroDistY;
        uiAbsSumY = uiAbsSumTransformSkipY;
        uiBestTransformMode[0] = 1;
      }

      pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    }

    if( bCodeChroma && checkTransformSkipUV  )
    {
      UInt uiNonzeroDistU, uiNonzeroDistV, uiAbsSumTransformSkipU, uiAbsSumTransformSkipV;
      Double dSingleCostU, dSingleCostV;

      Pel *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC );
      Pel *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC );
      UInt resiCStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();

      TCoeff bestCoeffU[32*32], bestCoeffV[32*32];
      memcpy( bestCoeffU, pcCoeffCurrU, sizeof(TCoeff) * uiNumSamplesChro );
      memcpy( bestCoeffV, pcCoeffCurrV, sizeof(TCoeff) * uiNumSamplesChro );

#if ADAPTIVE_QP_SELECTION
      TCoeff bestArlCoeffU[32*32], bestArlCoeffV[32*32];
      memcpy( bestArlCoeffU, pcArlCoeffCurrU, sizeof(TCoeff) * uiNumSamplesChro );
      memcpy( bestArlCoeffV, pcArlCoeffCurrV, sizeof(TCoeff) * uiNumSamplesChro );
#endif

      Pel bestResiU[32*32], bestResiV[32*32];
      for (Int i = 0; i < trHeightC; ++i )
      {
        memcpy( &bestResiU[i*trWidthC], pcResiCurrU+i*resiCStride, sizeof(Pel) * trWidthC );
        memcpy( &bestResiV[i*trWidthC], pcResiCurrV+i*resiCStride, sizeof(Pel) * trWidthC );
      }

      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );

      pcCU->setTransformSkipSubParts ( 1, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 
      pcCU->setTransformSkipSubParts ( 1, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );

      if (m_pcEncCfg->getUseRDOQTS())
      {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidthC, trHeightC, TEXT_CHROMA );          
      }

      Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

#if RDOQ_CHROMA_LAMBDA 
      m_pcTrQuant->selectLambda(TEXT_CHROMA_U);
#endif

      m_pcTrQuant->transformNxN( pcCU, pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrU, 
#if ADAPTIVE_QP_SELECTION
        pcArlCoeffCurrU, 
#endif        
        trWidthC, trHeightC, uiAbsSumTransformSkipU, TEXT_CHROMA_U, uiAbsPartIdx, true );
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );
#if RDOQ_CHROMA_LAMBDA
      m_pcTrQuant->selectLambda(TEXT_CHROMA_V);
#endif
      m_pcTrQuant->transformNxN( pcCU, pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrV,
#if ADAPTIVE_QP_SELECTION
        pcArlCoeffCurrV, 
#endif        
        trWidthC, trHeightC, uiAbsSumTransformSkipV, TEXT_CHROMA_V, uiAbsPartIdx, true );

      pcCU->setCbfSubParts( uiAbsSumTransformSkipU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumTransformSkipV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );

      uiSingleBitsU = 0;
      uiSingleBitsV = 0;

      if( uiAbsSumTransformSkipU )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U );
        uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits();    

        curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
        assert(scalingListType < SCALING_LIST_NUM);

        m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA,REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType, true  );

        uiNonzeroDistU = m_pcRdCost->getDistPart(g_bitDepthC, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                 pcResi->getCbAddr( absTUPartIdxC), pcResi->getCStride(), trWidthC, trHeightC
                                                 , TEXT_CHROMA_U
                                                 );

        dSingleCostU = m_pcRdCost->calcRdCost( uiSingleBitsU, uiNonzeroDistU );
      }

      if( !uiAbsSumTransformSkipU || minCostU < dSingleCostU )
      {
        pcCU->setTransformSkipSubParts ( 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 

        memcpy( pcCoeffCurrU, bestCoeffU, sizeof (TCoeff) * uiNumSamplesChro );
#if ADAPTIVE_QP_SELECTION
        memcpy( pcArlCoeffCurrU, bestArlCoeffU, sizeof (TCoeff) * uiNumSamplesChro );
#endif
        for( Int i = 0; i < trHeightC; ++i )
        {
          memcpy( pcResiCurrU+i*resiCStride, &bestResiU[i*trWidthC], sizeof(Pel) * trWidthC );
        }
      }
      else
      {
        uiDistU = uiNonzeroDistU;
        uiAbsSumU = uiAbsSumTransformSkipU;
        uiBestTransformMode[1] = 1;
      }

      if( uiAbsSumTransformSkipV )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V );
        uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits();

        curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
        assert(scalingListType < SCALING_LIST_NUM);

        m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_CHROMA,REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType, true );

        uiNonzeroDistV = m_pcRdCost->getDistPart(g_bitDepthC, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                 pcResi->getCrAddr( absTUPartIdxC ), pcResi->getCStride(), trWidthC, trHeightC
                                                 , TEXT_CHROMA_V
                                                 );

        dSingleCostV = m_pcRdCost->calcRdCost( uiSingleBitsV, uiNonzeroDistV );
      }

      if( !uiAbsSumTransformSkipV || minCostV < dSingleCostV )
      {
        pcCU->setTransformSkipSubParts ( 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 

        memcpy( pcCoeffCurrV, bestCoeffV, sizeof(TCoeff) * uiNumSamplesChro );
#if ADAPTIVE_QP_SELECTION
        memcpy( pcArlCoeffCurrV, bestArlCoeffV, sizeof(TCoeff) * uiNumSamplesChro );
#endif
        for( Int i = 0; i < trHeightC; ++i )
        {
          memcpy( pcResiCurrV+i*resiCStride, &bestResiV[i*trWidthC], sizeof(Pel) * trWidthC );
        }
      }
      else
      {
        uiDistV = uiNonzeroDistV;
        uiAbsSumV = uiAbsSumTransformSkipV;
        uiBestTransformMode[2] = 1;
      }

      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }

    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    m_pcEntropyCoder->resetBits();

    if( uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( 0, 5 - uiLog2TrSize );
    }

    if( bCodeChroma )
    {
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
    }

    m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );

    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA );

    if( bCodeChroma )
    {
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V );
    }

    uiSingleBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    uiSingleDist = uiDistY + uiDistU + uiDistV;
    dSingleCost = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDist );
  }  
  
  // code sub-blocks
  if( bCheckSplit )
  {
    if( bCheckFull )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    UInt uiSubdivDist = 0;
    UInt uiSubdivBits = 0;
    Double dSubdivCost = 0.0;
    
    const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
    for( UInt ui = 0; ui < 4; ++ui )
    {
      UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
      xEstimateResidualQT( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist );
    }
    
    UInt uiYCbf = 0;
    UInt uiUCbf = 0;
    UInt uiVCbf = 0;
    for( UInt ui = 0; ui < 4; ++ui )
    {
      uiYCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_LUMA,     uiTrMode + 1 );
      uiUCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_U, uiTrMode + 1 );
      uiVCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_V, uiTrMode + 1 );
    }
    for( UInt ui = 0; ui < 4 * uiQPartNumSubdiv; ++ui )
    {
      pcCU->getCbf( TEXT_LUMA     )[uiAbsPartIdx + ui] |= uiYCbf << uiTrMode;
      pcCU->getCbf( TEXT_CHROMA_U )[uiAbsPartIdx + ui] |= uiUCbf << uiTrMode;
      pcCU->getCbf( TEXT_CHROMA_V )[uiAbsPartIdx + ui] |= uiVCbf << uiTrMode;
    }
    
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    m_pcEntropyCoder->resetBits();
    
    xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, true,  TEXT_LUMA );
    xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_LUMA );
    xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_U );
    xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_V );
    
    uiSubdivBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    dSubdivCost  = m_pcRdCost->calcRdCost( uiSubdivBits, uiSubdivDist );
    
    if( uiYCbf || uiUCbf || uiVCbf || !bCheckFull )
    {
      if( dSubdivCost < dSingleCost )
      {
        rdCost += dSubdivCost;
        ruiBits += uiSubdivBits;
        ruiDist += uiSubdivDist;
        return;
      }
    }
    pcCU->setTransformSkipSubParts ( uiBestTransformMode[0], TEXT_LUMA, uiAbsPartIdx, uiDepth ); 
    if(bCodeChroma)
    {
      pcCU->setTransformSkipSubParts ( uiBestTransformMode[1], TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 
      pcCU->setTransformSkipSubParts ( uiBestTransformMode[2], TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC ); 
    }
    assert( bCheckFull );

    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
  }
  rdCost += dSingleCost;
  ruiBits += uiSingleBits;
  ruiDist += uiSingleDist;
  
  pcCU->setTrIdxSubParts( uiTrMode, uiAbsPartIdx, uiDepth );
  
  pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
  if( bCodeChroma )
  {
    pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
  }
}

Void TEncSearch::xEncodeResidualQT( TComDataCU* pcCU, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType )
{
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiCurrTrMode = uiDepth - pcCU->getDepth( 0 );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );
  
  const Bool bSubdiv = uiCurrTrMode != uiTrMode;
  
  const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;

  if( bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    m_pcEntropyCoder->encodeTransformSubdivFlag( bSubdiv, 5 - uiLog2TrSize );
  }

  assert( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA );
  if( bSubdivAndCbf )
  {
    const Bool bFirstCbfOfCU = uiCurrTrMode == 0;
    if( bFirstCbfOfCU || uiLog2TrSize > 2 )
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1 ) )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode );
      }
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1 ) )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode );
      }
    }
    else if( uiLog2TrSize == 2 )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1 ) );
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1 ) );
    }
  }
  
  if( !bSubdiv )
  {
    const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    
    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize-1;
    if( uiLog2TrSize == 2 )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }
    
    if( bSubdivAndCbf )
    {
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }
    else
    {
      if( eType == TEXT_LUMA     && pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA,     uiTrMode ) )
      {
        Int trWidth  = 1 << uiLog2TrSize;
        Int trHeight = 1 << uiLog2TrSize;
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA );
      }
      if( bCodeChroma )
      {
        Int trWidth  = 1 << uiLog2TrSizeC;
        Int trHeight = 1 << uiLog2TrSizeC;
        if( eType == TEXT_CHROMA_U && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( eType == TEXT_CHROMA_V && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
    }
  }
  else
  {
    if( bSubdivAndCbf || pcCU->getCbf( uiAbsPartIdx, eType, uiCurrTrMode ) )
    {
      const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
      for( UInt ui = 0; ui < 4; ++ui )
      {
        xEncodeResidualQT( pcCU, uiAbsPartIdx + ui * uiQPartNumSubdiv, uiDepth + 1, bSubdivAndCbf, eType );
      }
    }
  }
}

Void TEncSearch::xSetResidualQTData( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TComYuv* pcResi, UInt uiDepth, Bool bSpatial )
{
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiCurrTrMode = uiDepth - pcCU->getDepth( 0 );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );

  if( uiCurrTrMode == uiTrMode )
  {
    const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize-1;
    if( uiLog2TrSize == 2 )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }

    if( bSpatial )
    {      
      Int trWidth  = 1 << uiLog2TrSize;
      Int trHeight = 1 << uiLog2TrSize;
      m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartLuma    ( pcResi, absTUPartIdx, trWidth , trHeight );

      if( bCodeChroma )
      {
        m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartChroma( pcResi, uiAbsPartIdx, 1 << uiLog2TrSizeC, 1 << uiLog2TrSizeC );
      }
    }
    else
    {
      UInt    uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
      UInt    uiNumCoeffY = ( 1 << ( uiLog2TrSize << 1 ) );
      TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      TCoeff* pcCoeffDstY = pcCU->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
      Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      Int* pcArlCoeffDstY = pcCU->getArlCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
      if( bCodeChroma )
      {
        UInt    uiNumCoeffC = ( 1 << ( uiLog2TrSizeC << 1 ) );
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstU = pcCU->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstV = pcCU->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
        ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
        Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffDstU = pcCU->getArlCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffDstV = pcCU->getArlCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
        ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
      }
    }
  }
  else
  {
    const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
    for( UInt ui = 0; ui < 4; ++ui )
    {
      UInt nsAddr = uiAbsPartIdx + ui * uiQPartNumSubdiv;
      xSetResidualQTData( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, bSpatial );
    }
  }
}

UInt TEncSearch::xModeBitsIntra( TComDataCU* pcCU, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth )
{
  // Reload only contexts required for coding intra mode information
  m_pcRDGoOnSbacCoder->loadIntraDirModeLuma( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
  
  pcCU->setLumaIntraDirSubParts ( uiMode, uiPartOffset, uiDepth + uiInitTrDepth );
  
  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiPartOffset);
  
  return m_pcEntropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xUpdateCandList( UInt uiMode, Double uiCost, UInt uiFastCandNum, UInt * CandModeList, Double * CandCostList )
{
  UInt i;
  UInt shift=0;
  
  while ( shift<uiFastCandNum && uiCost<CandCostList[ uiFastCandNum-1-shift ] ) shift++;
  
  if( shift!=0 )
  {
    for(i=1; i<shift; i++)
    {
      CandModeList[ uiFastCandNum-i ] = CandModeList[ uiFastCandNum-1-i ];
      CandCostList[ uiFastCandNum-i ] = CandCostList[ uiFastCandNum-1-i ];
    }
    CandModeList[ uiFastCandNum-shift ] = uiMode;
    CandCostList[ uiFastCandNum-shift ] = uiCost;
    return 1;
  }
  
  return 0;
}

/** add inter-prediction syntax elements for a CU block
 * \param pcCU
 * \param uiQp
 * \param uiTrMode
 * \param ruiBits
 * \param rpcYuvRec
 * \param pcYuvPred
 * \param rpcYuvResi
 * \returns Void
 */
Void  TEncSearch::xAddSymbolBitsInter( TComDataCU* pcCU, UInt uiQp, UInt uiTrMode, UInt& ruiBits, TComYuv*& rpcYuvRec, TComYuv*pcYuvPred, TComYuv*& rpcYuvResi )
{
  if(pcCU->getMergeFlag( 0 ) && pcCU->getPartitionSize( 0 ) == SIZE_2Nx2N && !pcCU->getQtRootCbf( 0 ))
  {
    pcCU->setSkipFlagSubParts( true, 0, pcCU->getDepth(0) );

    m_pcEntropyCoder->resetBits();
    if(pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, true);
    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
  else
  {
    m_pcEntropyCoder->resetBits();
    if(pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag ( pcCU, 0, true );
    m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
    m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
    m_pcEntropyCoder->encodePredInfo( pcCU, 0, true );
    Bool bDummy = false;
    m_pcEntropyCoder->encodeCoeff   ( pcCU, 0, pcCU->getDepth(0), pcCU->getWidth(0), pcCU->getHeight(0), bDummy );
    
    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
}

/**
 * \brief Generate half-sample interpolated block
 *
 * \param pattern Reference picture ROI
 * \param biPred    Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingH( TComPattern* pattern, Bool biPred )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();
  
  Int intStride = m_filteredBlockTmp[0].getStride();
  Int dstStride = m_filteredBlock[0][0].getStride();
  Short *intPtr;
  Short *dstPtr;
  Int filterSize = NTAPS_LUMA;
  Int halfFilterSize = (filterSize>>1);
  Pel *srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;
  
  m_if.filterHorLuma(srcPtr, srcStride, m_filteredBlockTmp[0].getLumaAddr(), intStride, width+1, height+filterSize, 0, false);
  m_if.filterHorLuma(srcPtr, srcStride, m_filteredBlockTmp[2].getLumaAddr(), intStride, width+1, height+filterSize, 2, false);
  
  intPtr = m_filteredBlockTmp[0].getLumaAddr() + halfFilterSize * intStride + 1;  
  dstPtr = m_filteredBlock[0][0].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+0, height+0, 0, false, true);
  
  intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;  
  dstPtr = m_filteredBlock[2][0].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+0, height+1, 2, false, true);
  
  intPtr = m_filteredBlockTmp[2].getLumaAddr() + halfFilterSize * intStride;
  dstPtr = m_filteredBlock[0][2].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+1, height+0, 0, false, true);
  
  intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[2][2].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+1, height+1, 2, false, true);
}

/**
 * \brief Generate quarter-sample interpolated blocks
 *
 * \param pattern    Reference picture ROI
 * \param halfPelRef Half-pel mv
 * \param biPred     Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingQ( TComPattern* pattern, TComMv halfPelRef, Bool biPred )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();
  
  Pel *srcPtr;
  Int intStride = m_filteredBlockTmp[0].getStride();
  Int dstStride = m_filteredBlock[0][0].getStride();
  Short *intPtr;
  Short *dstPtr;
  Int filterSize = NTAPS_LUMA;
  
  Int halfFilterSize = (filterSize>>1);

  Int extHeight = (halfPelRef.getVer() == 0) ? height + filterSize : height + filterSize-1;
  
  // Horizontal filter 1/4
  srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
  intPtr = m_filteredBlockTmp[1].getLumaAddr();
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() >= 0)
  {
    srcPtr += 1;
  }
  m_if.filterHorLuma(srcPtr, srcStride, intPtr, intStride, width, extHeight, 1, false);
  
  // Horizontal filter 3/4
  srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;
  intPtr = m_filteredBlockTmp[3].getLumaAddr();
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() > 0)
  {
    srcPtr += 1;
  }
  m_if.filterHorLuma(srcPtr, srcStride, intPtr, intStride, width, extHeight, 3, false);        
  
  // Generate @ 1,1
  intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][1].getLumaAddr();
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
  
  // Generate @ 3,1
  intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][1].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
  
  if (halfPelRef.getVer() != 0)
  {
    // Generate @ 2,1
    intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][1].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true);
    
    // Generate @ 2,3
    intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][3].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true);
  }
  else
  {
    // Generate @ 0,1
    intPtr = m_filteredBlockTmp[1].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][1].getLumaAddr();
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true);
    
    // Generate @ 0,3
    intPtr = m_filteredBlockTmp[3].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][3].getLumaAddr();
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true);
  }
  
  if (halfPelRef.getHor() != 0)
  {
    // Generate @ 1,2
    intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[1][2].getLumaAddr();
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
    
    // Generate @ 3,2
    intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[3][2].getLumaAddr();
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);  
  }
  else
  {
    // Generate @ 1,0
    intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[1][0].getLumaAddr();
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
    
    // Generate @ 3,0
    intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[3][0].getLumaAddr();
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
  }
  
  // Generate @ 1,3
  intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][3].getLumaAddr();
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
  
  // Generate @ 3,3
  intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][3].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
}

/** set wp tables
 * \param TComDataCU* pcCU
 * \param iRefIdx
 * \param eRefPicListCur
 * \returns Void
 */
Void  TEncSearch::setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur )
{
  if ( iRefIdx<0 )
  {
    m_cDistParam.bApplyWeight = false;
    return;
  }

  TComSlice       *pcSlice  = pcCU->getSlice();
  TComPPS         *pps      = pcCU->getSlice()->getPPS();
  wpScalingParam  *wp0 , *wp1;
  m_cDistParam.bApplyWeight = ( pcSlice->getSliceType()==P_SLICE && pps->getUseWP() ) || ( pcSlice->getSliceType()==B_SLICE && pps->getWPBiPred() ) ;
  if ( !m_cDistParam.bApplyWeight ) return;

  Int iRefIdx0 = ( eRefPicListCur == REF_PIC_LIST_0 ) ? iRefIdx : (-1);
  Int iRefIdx1 = ( eRefPicListCur == REF_PIC_LIST_1 ) ? iRefIdx : (-1);

  getWpScaling( pcCU, iRefIdx0, iRefIdx1, wp0 , wp1 );

  if ( iRefIdx0 < 0 ) wp0 = NULL;
  if ( iRefIdx1 < 0 ) wp1 = NULL;

  m_cDistParam.wpCur  = NULL;

  if ( eRefPicListCur == REF_PIC_LIST_0 )
  {
    m_cDistParam.wpCur = wp0;
  }
  else
  {
    m_cDistParam.wpCur = wp1;
  }
}

//! \}