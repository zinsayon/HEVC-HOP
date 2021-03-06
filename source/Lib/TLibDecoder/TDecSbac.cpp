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

/** \file     TDecSbac.cpp
    \brief    Context-adaptive entropy decoder class
*/

#include "TDecSbac.h"

//! \ingroup TLibDecoder
//! \{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSbac::TDecSbac() 
// new structure here
: m_pcBitstream               ( 0 )
, m_pcTDecBinIf               ( NULL )
, m_numContextModels          ( 0 )
, m_cCUSplitFlagSCModel       ( 1,             1,               NUM_SPLIT_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels )
, m_cCUSkipFlagSCModel        ( 1,             1,               NUM_SKIP_FLAG_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeFlagExtSCModel    ( 1,             1,               NUM_MERGE_FLAG_EXT_CTX        , m_contextModels + m_numContextModels, m_numContextModels)
#if IT_GT
, m_cCUGTFlagExtSCModel    ( 1,             1,               NUM_GT_FLAG_EXT_CTX        , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUMergeIdxExtSCModel     ( 1,             1,               NUM_MERGE_IDX_EXT_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPartSizeSCModel        ( 1,             1,               NUM_PART_SIZE_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPredModeSCModel        ( 1,             1,               NUM_PRED_MODE_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUIntraPredSCModel       ( 1,             1,               NUM_ADI_CTX                   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUChromaPredSCModel      ( 1,             1,               NUM_CHROMA_PRED_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDeltaQpSCModel         ( 1,             1,               NUM_DELTA_QP_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUInterDirSCModel        ( 1,             1,               NUM_INTER_DIR_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCURefPicSCModel          ( 1,             1,               NUM_REF_NO_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMvdSCModel             ( 1,             1,               NUM_MV_RES_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
#if IT_GT
, m_cCUGTSCModel             ( 1,             1,               NUM_GT_RES_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUQtCbfSCModel           ( 1,             2,               NUM_QT_CBF_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUTransSubdivFlagSCModel ( 1,             1,               NUM_TRANS_SUBDIV_FLAG_CTX     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtRootCbfSCModel       ( 1,             1,               NUM_QT_ROOT_CBF_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigCoeffGroupSCModel   ( 1,             2,               NUM_SIG_CG_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigSCModel             ( 1,             1,               NUM_SIG_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastX               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastY               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUOneSCModel             ( 1,             1,               NUM_ONE_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAbsSCModel             ( 1,             1,               NUM_ABS_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cMVPIdxSCModel            ( 1,             1,               NUM_MVP_IDX_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeSCModel      ( 1,             1,               NUM_SAO_MERGE_FLAG_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel        ( 1,             1,               NUM_SAO_TYPE_IDX_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cTransformSkipSCModel     ( 1,             2,               NUM_TRANSFORMSKIP_FLAG_CTX    , m_contextModels + m_numContextModels, m_numContextModels)
, m_CUTransquantBypassFlagSCModel( 1,          1,               NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX, m_contextModels + m_numContextModels, m_numContextModels)
{
  assert( m_numContextModels <= MAX_NUM_CTX_MOD );
}

TDecSbac::~TDecSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecSbac::resetEntropy(TComSlice* pSlice)
{
  SliceType sliceType  = pSlice->getSliceType();
  Int       qp         = pSlice->getSliceQp();

  if (pSlice->getPPS()->getCabacInitPresentFlag() && pSlice->getCabacInitFlag())
  {
    switch (sliceType)
    {
    case P_SLICE:           // change initialization table to B_SLICE initialization
      sliceType = B_SLICE; 
      break;
    case B_SLICE:           // change initialization table to P_SLICE initialization
      sliceType = P_SLICE; 
      break;
    default     :           // should not occur
      assert(0);
    }
  }

  m_cCUSplitFlagSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_SKIP_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer    ( sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT );
#if IT_GT
  m_cCUGTFlagExtSCModel.initBuffer    ( sliceType, qp, (UChar*)INIT_GT_FLAG_EXT );
#endif
  m_cCUMergeIdxExtSCModel.initBuffer     ( sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT );
  m_cCUPartSizeSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_MVD );
#if IT_GT
  m_cCUGTSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_GT );
#endif
  m_cCURefPicSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer         ( sliceType, qp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer   ( sliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer               ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer               ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer            ( sliceType, qp, (UChar*)INIT_MVP_IDX );
  m_cSaoMergeSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );

  m_cCUTransSubdivFlagSCModel.initBuffer ( sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cTransformSkipSCModel.initBuffer     ( sliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
  m_CUTransquantBypassFlagSCModel.initBuffer( sliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
  m_uiLastDQpNonZero  = 0;
  
  // new structure
  m_uiLastQp          = qp;
  
  m_pcTDecBinIf->start();
}

/** The function does the following: Read out terminate bit. Flush CABAC. Byte-align for next tile.
 *  Intialize CABAC states. Start CABAC.
 */
Void TDecSbac::updateContextTables( SliceType eSliceType, Int iQp )
{
  UInt uiBit;
  m_pcTDecBinIf->decodeBinTrm(uiBit);
  assert(uiBit); // end_of_sub_stream_one_bit must be equal to 1
  m_pcTDecBinIf->finish();  
  m_pcBitstream->readOutTrailingBits();
  m_cCUSplitFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
#if IT_GT
  m_cCUGTFlagExtSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_GT_FLAG_EXT );
#endif
  m_cCUMergeFlagExtSCModel.initBuffer    ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT );
  m_cCUMergeIdxExtSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT );
  m_cCUPartSizeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_MVD );
#if IT_GT
  m_cCUGTSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_GT );
#endif
  m_cCURefPicSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer         ( eSliceType, iQp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer   ( eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer               ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer               ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_MVP_IDX );
  m_cSaoMergeSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
  m_cCUTransSubdivFlagSCModel.initBuffer ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cTransformSkipSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
  m_CUTransquantBypassFlagSCModel.initBuffer( eSliceType, iQp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
  m_pcTDecBinIf->start();
}

Void TDecSbac::parseTerminatingBit( UInt& ruiBit )
{
  m_pcTDecBinIf->decodeBinTrm( ruiBit );
  if ( ruiBit )
  {
    m_pcTDecBinIf->finish();
  }
}


Void TDecSbac::xReadUnaryMaxSymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    ruiSymbol = 0;
    return;
  }
  
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );
  
  if( ruiSymbol == 0 || uiMaxSymbol == 1 )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont && ( uiSymbol < uiMaxSymbol - 1 ) );
  
  if( uiCont && ( uiSymbol == uiMaxSymbol - 1 ) )
  {
    uiSymbol++;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;
  
  while( uiBit )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );
    uiSymbol += uiBit << uiCount++;
  }
  
  if ( --uiCount )
  {
    UInt bins;
    m_pcTDecBinIf->decodeBinsEP( bins, uiCount );
    uiSymbol += bins;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadUnarySymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );
  
  if( !ruiSymbol )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont );
  
  ruiSymbol = uiSymbol;
}


/** Parsing of coeff_abs_level_remaing
 * \param ruiSymbol reference to coeff_abs_level_remaing
 * \param ruiParam reference to parameter
 * \returns Void
 */
Void TDecSbac::xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam )
{

  UInt prefix   = 0;
  UInt codeWord = 0;
  do
  {
    prefix++;
    m_pcTDecBinIf->decodeBinEP( codeWord );
  }
  while( codeWord);
  codeWord  = 1 - codeWord;
  prefix -= codeWord;
  codeWord=0;
  if (prefix < COEF_REMAIN_BIN_REDUCTION )
  {
    m_pcTDecBinIf->decodeBinsEP(codeWord,rParam);
    rSymbol = (prefix<<rParam) + codeWord;
  }
  else
  {
    m_pcTDecBinIf->decodeBinsEP(codeWord,prefix-COEF_REMAIN_BIN_REDUCTION+rParam);
    rSymbol = (((1<<(prefix-COEF_REMAIN_BIN_REDUCTION))+COEF_REMAIN_BIN_REDUCTION-1)<<rParam)+codeWord;
  }
}

/** Parse I_PCM information. 
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, parse its PCM alignment bits and codes. 
 */
Void TDecSbac::parseIPCMInfo ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBinTrm(uiSymbol);

#if CU_ENC_DEC_TRAC
  DTRACE_CU("pcm_flag", uiSymbol)
#endif

  if (uiSymbol)
  {
    Bool bIpcmFlag = true;

    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setTrIdxSubParts     ( 0, uiAbsPartIdx, uiDepth );
    pcCU->setIPCMFlagSubParts  ( bIpcmFlag, uiAbsPartIdx, uiDepth );

    UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset>>2;

    Pel* piPCMSample;
    UInt uiWidth;
    UInt uiHeight;
    UInt uiSampleBits;
    UInt uiX, uiY;

    piPCMSample = pcCU->getPCMSampleY() + uiLumaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx);
    uiHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCb() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCr() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    m_pcTDecBinIf->start();
  }
}

Void TDecSbac::parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_CUTransquantBypassFlagSCModel.get( 0, 0, 0 ) );
#if CU_ENC_DEC_TRAC
  DTRACE_CU("cu_transquant_bypass_flag", uiSymbol); 
#endif
  pcCU->setCUTransquantBypassSubParts(uiSymbol ? true : false, uiAbsPartIdx, uiDepth);
}

/** parse skip flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) );
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
#endif  
  if( uiSymbol )
  {
    pcCU->setSkipFlagSubParts( true,        uiAbsPartIdx, uiDepth );
    pcCU->setPredModeSubParts( MODE_INTER,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setMergeFlagSubParts( true , uiAbsPartIdx, 0, uiDepth );
  }
#if CU_ENC_DEC_TRAC
  DTRACE_CU("cu_skip_flag", uiSymbol); 
#endif
}

/** parse merge flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiPUIdx
 * \returns Void
 */
Void TDecSbac::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
#else
  DTRACE_PU("merge_flag", uiSymbol)
#endif
}

#if IT_GT
Void TDecSbac::parseGTFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  UInt uiSymbol;
#if !IT_GT_ENCODE_FLAG_AS_WRONG_MVP
  m_pcTDecBinIf->decodeBin( uiSymbol, *m_cCUGTFlagExtSCModel.get( 0 ) );
  pcCU->setGTFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );
#if CU_ENC_DEC_TRAC
  DTRACE_PU("GT_FLAG", uiSymbol)
#endif
#endif
}
#endif

Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex )
{
  UInt uiUnaryIdx = 0;
  UInt uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
  if ( uiNumCand > 1 )
  {
    for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
    {
      UInt uiSymbol = 0;
      if ( uiUnaryIdx==0 )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) );
      }
      else
      {
        m_pcTDecBinIf->decodeBinEP( uiSymbol );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
#if CU_ENC_DEC_TRAC
    DTRACE_PU("merge_idx", uiUnaryIdx)
#endif
  }
  ruiMergeIndex = uiUnaryIdx;
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseMergeIndex()" )
  DTRACE_CABAC_T( "\tuiMRGIdx= " )
  DTRACE_CABAC_V( ruiMergeIndex )
  DTRACE_CABAC_T( "\n" )
#endif
}

Void TDecSbac::parseMVPIdx      ( Int& riMVPIdx )
{
  UInt uiSymbol;
#if !IT_GT_ENCODE_FLAG_AS_WRONG_MVP
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, AMVP_MAX_NUM_CANDS-1);
#else
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, AMVP_MAX_NUM_CANDS);
#endif
  riMVPIdx = uiSymbol;
}

Void TDecSbac::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return;
  }
  
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSplitFlagSCModel.get( 0, 0, pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth ) ) );
#if CU_ENC_DEC_TRAC
  DTRACE_CU("split_cu_flag", uiSymbol); 
#else
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tSplitFlag\n" )
#endif
  pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );
  
  return;
}

/** parse partition size
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol, uiMode = 0;
  PartSize eMode;
  
  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    uiSymbol = 1;
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
#if CU_ENC_DEC_TRAC          
      DTRACE_CU("part_mode", uiSymbol)
#endif
    }
    eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
    UInt uiTrLevel = 0;    
    UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
    UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
    uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
    if( eMode == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1+uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
  else
  {
    UInt uiMaxNumBits = 2;
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && !( (g_uiMaxCUWidth>>uiDepth) == 8 && (g_uiMaxCUHeight>>uiDepth) == 8 ) )
    {
      uiMaxNumBits ++;
    }
    for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
      if ( uiSymbol )
      {
        break;
      }
      uiMode++;
    }
    eMode = (PartSize) uiMode;
    if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
    {
      if (eMode == SIZE_2NxN)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol);
          eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
        }
      }
      else if (eMode == SIZE_Nx2N)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol);
          eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
        }
      }
    }
#if CU_ENC_DEC_TRAC          
    DTRACE_CU("part_mode", eMode )
#endif
  }
  pcCU->setPartSizeSubParts( eMode, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
}

/** parse prediction mode
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    return;
  }
  
  UInt uiSymbol;
  Int  iPredMode = MODE_INTER;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
  iPredMode += uiSymbol;
#if CU_ENC_DEC_TRAC          
  DTRACE_CU("pred_mode_flag", uiSymbol)
#endif     
  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt absPartIdx, UInt depth )
{
  PartSize mode = pcCU->getPartitionSize( absPartIdx );
  UInt partNum = mode==SIZE_NxN?4:1;
  UInt partOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(absPartIdx) << 1 ) ) >> 2;
  UInt mpmPred[4],symbol;
  Int j,intraPredMode;    
  if (mode==SIZE_NxN)
  {
    depth++;
  }
  for (j=0;j<partNum;j++)
  {
    m_pcTDecBinIf->decodeBin( symbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );
    mpmPred[j] = symbol;
#if CU_ENC_DEC_TRAC          
    DTRACE_CU("prev_intra_luma_pred_flag", symbol)
#endif
  }
  for (j=0;j<partNum;j++)
  {
    Int preds[3] = {-1, -1, -1};
    Int predNum = pcCU->getIntraDirLumaPredictor(absPartIdx+partOffset*j, preds);  
    if (mpmPred[j])
    {
      m_pcTDecBinIf->decodeBinEP( symbol );
      if (symbol)
      {
        m_pcTDecBinIf->decodeBinEP( symbol );
        symbol++;
      }
#if CU_ENC_DEC_TRAC          
      DTRACE_CU("mpm_idx", symbol)
#endif
      intraPredMode = preds[symbol];
    }
    else
    {
      m_pcTDecBinIf->decodeBinsEP( symbol, 5 );
      intraPredMode = symbol;
#if CU_ENC_DEC_TRAC          
      DTRACE_CU("rem_intra_luma_pred_mode", symbol)
#endif          
      //postponed sorting of MPMs (only in remaining branch)
      if (preds[0] > preds[1])
      { 
        std::swap(preds[0], preds[1]); 
      }
      if (preds[0] > preds[2])
      {
        std::swap(preds[0], preds[2]);
      }
      if (preds[1] > preds[2])
      {
        std::swap(preds[1], preds[2]);
      }
      for ( Int i = 0; i < predNum; i++ )
      {
        intraPredMode += ( intraPredMode >= preds[i] );
      }
    }
    pcCU->setLumaIntraDirSubParts( (UChar)intraPredMode, absPartIdx+partOffset*j, depth );
  }
}

Void TDecSbac::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );

  if( uiSymbol == 0 )
  {
#if CU_ENC_DEC_TRAC        
    DTRACE_CU("intra_chroma_pred_mode", uiSymbol )
#endif 
    uiSymbol = DM_CHROMA_IDX;
  } 
  else 
  {
    {
      UInt uiIPredMode;
      m_pcTDecBinIf->decodeBinsEP( uiIPredMode, 2 );
#if CU_ENC_DEC_TRAC          
      DTRACE_CU("intra_chroma_pred_mode", uiIPredMode )
#endif  
      UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
      pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );
      uiSymbol = uiAllowedChromaDir[ uiIPredMode ];
    }
  }
  pcCU->setChromIntraDirSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  return;
}

Void TDecSbac::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx = m_cCUInterDirSCModel.get( 0 );
  uiSymbol = 0;
  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N || pcCU->getHeight(uiAbsPartIdx) != 8 )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, *( pCtx + uiCtx ) );
  }

  if( uiSymbol )
  {
    uiSymbol = 2;
  }
  else
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, *( pCtx + 4 ) );
    assert(uiSymbol == 0 || uiSymbol == 1);
  }

  uiSymbol++;
  ruiInterDir = uiSymbol;
#if CU_ENC_DEC_TRAC
  DTRACE_PU("inter_pred_idc", ruiInterDir - 1 )    
#endif
  return;
}

Void TDecSbac::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList )
{
  UInt uiSymbol;
  {
    ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
    m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );

    if( uiSymbol )
    {
      UInt uiRefNum = pcCU->getSlice()->getNumRefIdx( eRefList ) - 2;
      pCtx++;
      UInt ui;
      for( ui = 0; ui < uiRefNum; ++ui )
      {
        if( ui == 0 )
        {
          m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
        }
        else
        {
          m_pcTDecBinIf->decodeBinEP( uiSymbol );
        }
        if( uiSymbol == 0 )
        {
          break;
        }
      }
      uiSymbol = ui + 1;
    }
    riRefFrmIdx = uiSymbol;
  }
#if CU_ENC_DEC_TRAC
  if ( eRefList == REF_PIC_LIST_0 )
  {
    DTRACE_PU("ref_idx_l0", uiSymbol)
  }
  else
  {
    DTRACE_PU("ref_idx_l1", uiSymbol)
  }
#endif
  return;
}

Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  UInt uiHorAbs;
  UInt uiVerAbs;
  UInt uiHorSign = 0;
  UInt uiVerSign = 0;
  ContextModel *pCtx = m_cCUMvdSCModel.get( 0 );

  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    uiHorAbs=0;
    uiVerAbs=0;
  }
  else
  {
    m_pcTDecBinIf->decodeBin( uiHorAbs, *pCtx );
    m_pcTDecBinIf->decodeBin( uiVerAbs, *pCtx );

    const Bool bHorAbsGr0 = uiHorAbs != 0;
    const Bool bVerAbsGr0 = uiVerAbs != 0;
    pCtx++;

    if( bHorAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
      uiHorAbs += uiSymbol;
    }

    if( bVerAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
      uiVerAbs += uiSymbol;
    }

    if( bHorAbsGr0 )
    {
      if( 2 == uiHorAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 );
        uiHorAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiHorSign );
    }

    if( bVerAbsGr0 )
    {
      if( 2 == uiVerAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 );
        uiVerAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiVerSign );
    }

  }

  const TComMv cMv( uiHorSign ? -Int( uiHorAbs ): uiHorAbs, uiVerSign ? -Int( uiVerAbs ) : uiVerAbs );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  return;
}

#if IT_GT
Void TDecSbac::parseGT( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;

  const Bool bGTFlag = pcCU->getGTFlag(uiAbsPartIdx);

  UInt uiHorGT0Abs = 0;
  UInt uiVerGT0Abs = 0;
  UInt uiHorGT1Abs = 0;
  UInt uiVerGT1Abs = 0;
  UInt uiHorGT2Abs = 0;
  UInt uiVerGT2Abs = 0;
  UInt uiHorGT3Abs = 0;
  UInt uiVerGT3Abs = 0;

  UInt uiHorGT0Sign = 0;
  UInt uiVerGT0Sign = 0;
  UInt uiHorGT1Sign = 0;
  UInt uiVerGT1Sign = 0;
  UInt uiHorGT2Sign = 0;
  UInt uiVerGT2Sign = 0;
  UInt uiHorGT3Sign = 0;
  UInt uiVerGT3Sign = 0;

  Bool C0_NULL = false;
  Bool C1_NULL = false;
  Bool C2_NULL = false;
  Bool C3_NULL = false;

  Bool C0_Hor_NULL = false;
  Bool C1_Hor_NULL = false;
  Bool C2_Hor_NULL = false;
  Bool C3_Hor_NULL = false;

  Bool C0_Ver_NULL = false;
  Bool C1_Ver_NULL = false;
  Bool C2_Ver_NULL = false;
  Bool C3_Ver_NULL = false;

  ContextModel *pCtx = m_cCUGTSCModel.get( 0 );

  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
	  uiHorGT0Abs=0;
	  uiVerGT0Abs=0;
	  uiHorGT1Abs=0;
	  uiVerGT1Abs=0;
	  uiHorGT2Abs=0;
	  uiVerGT2Abs=0;
	  uiHorGT3Abs=0;
	  uiVerGT3Abs=0;
  }
  else
  {
	  if(bGTFlag)
	  {
#if IT_GT_CODING == 0
		  m_pcTDecBinIf->decodeBin( uiHorGT0Abs, *pCtx );
		  m_pcTDecBinIf->decodeBin( uiVerGT0Abs, *pCtx );
		  C0_Hor_NULL = uiHorGT0Abs == 0;
		  C0_Ver_NULL = uiVerGT0Abs == 0;
		  m_pcTDecBinIf->decodeBin( uiHorGT1Abs, *pCtx );
		  m_pcTDecBinIf->decodeBin( uiVerGT1Abs, *pCtx );
		  C1_Hor_NULL = uiHorGT1Abs == 0;
		  C1_Ver_NULL = uiVerGT1Abs == 0;
		  m_pcTDecBinIf->decodeBin( uiHorGT2Abs, *pCtx );
		  m_pcTDecBinIf->decodeBin( uiVerGT2Abs, *pCtx );
		  C2_Hor_NULL = uiHorGT2Abs == 0;
		  C2_Ver_NULL = uiVerGT2Abs == 0;
#if !IT_GT_AFFINE
		  m_pcTDecBinIf->decodeBin( uiHorGT3Abs, *pCtx );
		  m_pcTDecBinIf->decodeBin( uiVerGT3Abs, *pCtx );
		  C3_Hor_NULL = uiHorGT3Abs == 0;
		  C3_Ver_NULL = uiVerGT3Abs == 0;
#endif
		  pCtx++;
		  if( !C0_Hor_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiHorGT0Abs += uiSymbol;
		  }
		  if( !C0_Ver_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiVerGT0Abs += uiSymbol;
		  }
		  if( !C1_Hor_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiHorGT1Abs += uiSymbol;
		  }
		  if( !C1_Ver_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiVerGT1Abs += uiSymbol;
		  }
		  if( !C2_Hor_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiHorGT2Abs += uiSymbol;
		  }
		  if( !C2_Ver_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiVerGT2Abs += uiSymbol;
		  }
#if !IT_GT_AFFINE
		  if( !C3_Hor_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiHorGT3Abs += uiSymbol;
		  }
		  if( !C3_Ver_NULL )
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  uiVerGT3Abs += uiSymbol;
		  }
#endif
		  if( !C0_Hor_NULL )
		  {
			  if( 2 == uiHorGT0Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiHorGT0Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiHorGT0Sign );
		  }
		  if( !C0_Ver_NULL )
		  {
			  if( 2 == uiVerGT0Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiVerGT0Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiVerGT0Sign );
		  }
		  if( !C1_Hor_NULL )
		  {
			  if( 2 == uiHorGT1Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiHorGT1Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiHorGT1Sign );
		  }
		  if( !C1_Ver_NULL )
		  {
			  if( 2 == uiVerGT1Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiVerGT1Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiVerGT1Sign );
		  }
		  if( !C2_Hor_NULL )
		  {
			  if( 2 == uiHorGT2Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiHorGT2Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiHorGT2Sign );
		  }
		  if( !C2_Ver_NULL )
		  {
			  if( 2 == uiVerGT2Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiVerGT2Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiVerGT2Sign );
		  }
#if !IT_GT_AFFINE
		  if( !C3_Hor_NULL )
		  {
			  if( 2 == uiHorGT3Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiHorGT3Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiHorGT3Sign );
		  }
		  if( !C3_Ver_NULL )
		  {
			  if( 2 == uiVerGT3Abs )
			  {
				  xReadEpExGolomb( uiSymbol, 1 );
				  uiVerGT3Abs += uiSymbol;
			  }

			  m_pcTDecBinIf->decodeBinEP( uiVerGT3Sign );
		  }
#endif
#endif
#if IT_GT_CODING == 1
		  // corner == (0,0)
		  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
		  if(!uiSymbol)
		  {
			  C0_NULL = true;
			  uiHorGT0Abs = 0;
			  uiVerGT0Abs = 0;
		  }
		  else
			  C0_NULL = false;
		  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
		  if(!uiSymbol)
		  {
			  C1_NULL = true;
			  uiHorGT1Abs = 0;
			  uiVerGT1Abs = 0;
		  }
		  else
			  C1_NULL = false;
		  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
		  if(!uiSymbol)
		  {
			  C2_NULL = true;
			  uiHorGT2Abs = 0;
			  uiVerGT2Abs = 0;
		  }
		  else
			  C2_NULL = false;
#if !IT_GT_AFFINE
		  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
		  if(!uiSymbol)
		  {
			  C3_NULL = true;
			  uiHorGT3Abs = 0;
			  uiVerGT3Abs = 0;
		  }
		  else
			  C3_NULL = false;
#endif
		  //x or y == 0
		  pCtx++;
		  if(!C0_NULL)
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C0_Hor_NULL = true;
				  uiHorGT0Abs = 0;
			  }
			  else
				  C0_Hor_NULL = false;
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C0_Ver_NULL = true;
				  uiVerGT0Abs = 0;
			  }
			  else
				  C0_Ver_NULL = false;
		  }
		  if(!C1_NULL)
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C1_Hor_NULL = true;
				  uiHorGT1Abs = 0;
			  }
			  else
				  C1_Hor_NULL = false;
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C1_Ver_NULL = true;
				  uiVerGT1Abs = 0;
			  }
			  else
				  C1_Ver_NULL = false;
		  }
		  if(!C2_NULL)
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C2_Hor_NULL = true;
				  uiHorGT2Abs = 0;
			  }
			  else
				  C2_Hor_NULL = false;
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C2_Ver_NULL = true;
				  uiVerGT2Abs = 0;
			  }
			  else
				  C2_Ver_NULL = false;
		  }
#if !IT_GT_AFFINE
		  if(!C3_NULL)
		  {
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C3_Hor_NULL = true;
				  uiHorGT3Abs = 0;
			  }
			  else
				  C3_Hor_NULL = false;
			  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
			  if(!uiSymbol)
			  {
				  C3_Ver_NULL = true;
				  uiVerGT3Abs = 0;
			  }
			  else
				  C3_Ver_NULL = false;
		  }
#endif
		  // signal and abs
		  if(!C0_NULL)
		  {
			  if(!C0_Hor_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiHorGT0Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiHorGT0Abs, 1 );
#else
				  uiHorGT0Abs = 1;
#endif
			  }
			  if(!C0_Ver_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiVerGT0Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiVerGT0Abs, 1 );
#else
				  uiVerGT0Abs = 1;
#endif
			  }
		  }
		  if(!C1_NULL)
		  {
			  if(!C1_Hor_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiHorGT1Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiHorGT1Abs, 1 );
#else
				  uiHorGT1Abs = 1;
#endif
			  }
			  if(!C1_Ver_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiVerGT1Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiVerGT1Abs, 1 );
#else
				  uiVerGT1Abs = 1;
#endif
			  }
		  }
		  if(!C2_NULL)
		  {
			  if(!C2_Hor_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiHorGT2Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiHorGT2Abs, 1 );
#else
				  uiHorGT2Abs = 1;
#endif
			  }
			  if(!C2_Ver_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiVerGT2Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiVerGT2Abs, 1 );
#else
				  uiVerGT2Abs = 1;
#endif
			  }
		  }
#if !IT_GT_AFFINE
		  if(!C3_NULL)
		  {
			  if(!C3_Hor_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiHorGT3Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiHorGT3Abs, 1 );
#else
				  uiHorGT3Abs = 1;
#endif
			  }
			  if(!C3_Ver_NULL)
			  {
				  m_pcTDecBinIf->decodeBinEP( uiVerGT3Sign );
#if IT_MAX_NSS_Iteration > 1
				  xReadEpExGolomb( uiVerGT3Abs, 1 );
#else
				  uiVerGT3Abs = 1;
#endif
			  }
		  }
#endif
#endif
	  }
	  else
	  {
		  uiHorGT0Abs=0;
		  uiVerGT0Abs=0;
		  uiHorGT1Abs=0;
		  uiVerGT1Abs=0;
		  uiHorGT2Abs=0;
		  uiVerGT2Abs=0;
		  uiHorGT3Abs=0;
		  uiVerGT3Abs=0;
	  }
  }

  const TComMv cGT0( uiHorGT0Sign ? -Int( uiHorGT0Abs ): uiHorGT0Abs, uiVerGT0Sign ? -Int( uiVerGT0Abs ) : uiVerGT0Abs );
  const TComMv cGT1( uiHorGT1Sign ? -Int( uiHorGT1Abs ): uiHorGT1Abs, uiVerGT1Sign ? -Int( uiVerGT1Abs ) : uiVerGT1Abs );
  const TComMv cGT2( uiHorGT2Sign ? -Int( uiHorGT2Abs ): uiHorGT2Abs, uiVerGT2Sign ? -Int( uiVerGT2Abs ) : uiVerGT2Abs );
#if IT_GT_AFFINE
  const TComMv cGT3( cGT0.getHor() - cGT1.getHor() + cGT2.getHor(), cGT0.getVer() - cGT1.getVer() + cGT2.getVer() );
#else
  const TComMv cGT3( uiHorGT3Sign ? -Int( uiHorGT3Abs ): uiHorGT3Abs, uiVerGT3Sign ? -Int( uiVerGT3Abs ) : uiVerGT3Abs );
#endif
  pcCU->getCUGT0Field( eRefList )->setAllMv( cGT0, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  pcCU->getCUGT1Field( eRefList )->setAllMv( cGT1, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  pcCU->getCUGT2Field( eRefList )->setAllMv( cGT2, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  pcCU->getCUGT3Field( eRefList )->setAllMv( cGT3, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );

  if(bGTFlag)
  {
	  DTRACE_PU( "GT0L0[0]",  cGT0.getHor());
	  DTRACE_PU( "GT0L0[1]", cGT0.getVer());
	  DTRACE_PU( "GT1L0[0]",  cGT1.getHor());
	  DTRACE_PU( "GT1L0[1]", cGT1.getVer());
	  DTRACE_PU( "GT2L0[0]",  cGT2.getHor());
	  DTRACE_PU( "GT2L0[1]", cGT2.getVer());
	  DTRACE_PU( "GT3L0[0]",  cGT3.getHor());
	  DTRACE_PU( "GT3L0[1]", cGT3.getVer());
  }

  return;
}
#endif


Void TDecSbac::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  m_pcTDecBinIf->decodeBin( ruiSubdivFlag, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiLog2TransformBlockSize ) );
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( ruiSubdivFlag )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiLog2TransformBlockSize )
  DTRACE_CABAC_T( "\n" )
#endif
}

Void TDecSbac::parseQtRootCbf( UInt uiAbsPartIdx, UInt& uiQtRootCbf )
{
  UInt uiSymbol;
  const UInt uiCtx = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
#else
  DTRACE_CU( "rqt_root_cbf", uiSymbol )
#endif
  
  uiQtRootCbf = uiSymbol;
}

Void TDecSbac::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int qp;
  UInt uiDQp;
  Int  iDQp;
  
  UInt uiSymbol;

  xReadUnaryMaxSymbol (uiDQp,  &m_cCUDeltaQpSCModel.get( 0, 0, 0 ), 1, CU_DQP_TU_CMAX);

  if( uiDQp >= CU_DQP_TU_CMAX)
  {
    xReadEpExGolomb( uiSymbol, CU_DQP_EG_k );
    uiDQp+=uiSymbol;
  }

  if ( uiDQp > 0 )
  {
    UInt uiSign;
    Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffsetY();
    m_pcTDecBinIf->decodeBinEP(uiSign);
    iDQp = uiDQp;
    if(uiSign)
    {
      iDQp = -iDQp;
    }
    qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+qpBdOffsetY)) - qpBdOffsetY;
  }
  else 
  {
    qp = pcCU->getRefQP(uiAbsPartIdx);
  }
  pcCU->setQPSubParts(qp, uiAbsPartIdx, uiDepth);  
  pcCU->setCodedQP(qp);
}

Void TDecSbac::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxQtCbf( eType, uiTrDepth );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtCbfSCModel.get( 0, eType ? TEXT_CHROMA: eType, uiCtx ) );
#if !CU_ENC_DEC_TRAC 
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
#endif 
  pcCU->setCbfSubParts( uiSymbol << uiTrDepth, eType, uiAbsPartIdx, uiDepth );
}

void TDecSbac::parseTransformSkipFlags (TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, UInt uiDepth, TextType eTType)
{
  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    return;
  }
  if(width != 4 || height != 4)
  {
    return;
  }
  
  UInt useTransformSkip;
  m_pcTDecBinIf->decodeBin( useTransformSkip , m_cTransformSkipSCModel.get( 0, eTType? TEXT_CHROMA: TEXT_LUMA, 0 ) );
  if(eTType!= TEXT_LUMA)
  {
    const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiDepth;
    if(uiLog2TrafoSize == 2) 
    { 
      uiDepth --;
    }
  }
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T("\tparseTransformSkip()");
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( useTransformSkip )
  DTRACE_CABAC_T( "\tAddr=" )
  DTRACE_CABAC_V( pcCU->getAddr() )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eTType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
#endif

  pcCU->setTransformSkipSubParts( useTransformSkip, eTType, uiAbsPartIdx, uiDepth);
}

/** Parse (X,Y) position of the last significant coefficient
 * \param uiPosLastX reference to X component of last coefficient
 * \param uiPosLastY reference to Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param eTType plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 *
 * This method decodes the X and Y component within a block of the last significant coefficient.
 */
Void TDecSbac::parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, TextType eTType, UInt uiScanIdx )
{
  UInt uiLast;
  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, eTType );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, eTType );

  Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
  blkSizeOffsetX = eTType ? 0: (g_aucConvertToBit[ width ] *3 + ((g_aucConvertToBit[ width ] +1)>>2));
  blkSizeOffsetY = eTType ? 0: (g_aucConvertToBit[ height ]*3 + ((g_aucConvertToBit[ height ]+1)>>2));
  shiftX= eTType ? g_aucConvertToBit[ width  ] :((g_aucConvertToBit[ width  ]+3)>>2);
  shiftY= eTType ? g_aucConvertToBit[ height ] :((g_aucConvertToBit[ height ]+3)>>2);
  // posX
  for( uiPosLastX = 0; uiPosLastX < g_uiGroupIdx[ width - 1 ]; uiPosLastX++ )
  {
    m_pcTDecBinIf->decodeBin( uiLast, *( pCtxX + blkSizeOffsetX + (uiPosLastX >>shiftX) ) );
    if( !uiLast )
    {
      break;
    }
  }

  // posY
  for( uiPosLastY = 0; uiPosLastY < g_uiGroupIdx[ height - 1 ]; uiPosLastY++ )
  {
    m_pcTDecBinIf->decodeBin( uiLast, *( pCtxY + blkSizeOffsetY + (uiPosLastY >>shiftY)) );
    if( !uiLast )
    {
      break;
    }
  }
  if ( uiPosLastX > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastX - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast );
      uiTemp += uiLast << i;
    }
    uiPosLastX = g_uiMinInGroup[ uiPosLastX ] + uiTemp;
  }
  if ( uiPosLastY > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastY - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast );
      uiTemp += uiLast << i;
    }
    uiPosLastY = g_uiMinInGroup[ uiPosLastY ] + uiTemp;
  }
  
  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosLastX, uiPosLastY );
  }
}

Void TDecSbac::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
#if !CU_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( eTType )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
  DTRACE_CABAC_V( uiDepth )
  DTRACE_CABAC_T( "\tabspartidx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )
#endif  
  if( uiWidth > pcCU->getSlice()->getSPS()->getMaxTrSize() )
  {
    uiWidth  = pcCU->getSlice()->getSPS()->getMaxTrSize();
    uiHeight = pcCU->getSlice()->getSPS()->getMaxTrSize();
  }
  if(pcCU->getSlice()->getPPS()->getUseTransformSkip())
  {
    parseTransformSkipFlags( pcCU, uiAbsPartIdx, uiWidth, uiHeight, uiDepth, eTType);
  }

  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );
  
  //----- parse significance map -----
  const UInt  uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  const UInt  uiMaxNumCoeff     = uiWidth * uiHeight;
  const UInt  uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;
  UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
  
  //===== decode last significant =====
  UInt uiPosLastX, uiPosLastY;
  parseLastSignificantXY( uiPosLastX, uiPosLastY, uiWidth, uiHeight, eTType, uiScanIdx );
  UInt uiBlkPosLast      = uiPosLastX + (uiPosLastY<<uiLog2BlockSize);
  pcCoef[ uiBlkPosLast ] = 1;

  //===== decode significance flags =====
  UInt uiScanPosLast;
  const UInt *scan   = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize-1 ];
  for( uiScanPosLast = 0; uiScanPosLast < uiMaxNumCoeffM1; uiScanPosLast++ )
  {
    UInt uiBlkPos = scan[ uiScanPosLast ];
    if( uiBlkPosLast == uiBlkPos )
    {
      break;
    }
  }

  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, eTType );
  ContextModel * const baseCtx = (eTType==TEXT_LUMA) ? m_cCUSigSCModel.get( 0, 0 ) : m_cCUSigSCModel.get( 0, 0 ) + NUM_SIG_FLAG_CTX_LUMA;

  const Int  iLastScanSet      = uiScanPosLast >> LOG2_SCAN_SET_SIZE;
  UInt c1 = 1;
  UInt uiGoRiceParam           = 0;

  Bool beValid; 
  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    beValid = false;
  }
  else 
  {
    beValid = pcCU->getSlice()->getPPS()->getSignHideFlag() > 0;
  }
  UInt absSum = 0;

  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];
  ::memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );
  const UInt uiNumBlkSide = uiWidth >> (MLS_CG_SIZE >> 1);
  const UInt * scanCG;
  {
    scanCG = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize > 3 ? uiLog2BlockSize-2-1 : 0  ];    
    if( uiLog2BlockSize == 3 )
    {
      scanCG = g_sigLastScan8x8[ uiScanIdx ];
    }
    else if( uiLog2BlockSize == 5 )
    {
      scanCG = g_sigLastScanCG32x32;
    }
  }
  Int  iScanPosSig             = (Int) uiScanPosLast;
  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int  iSubPos     = iSubSet << LOG2_SCAN_SET_SIZE;
    uiGoRiceParam    = 0;
    Int numNonZero = 0;
    
    Int lastNZPosInCG = -1, firstNZPosInCG = SCAN_SET_SIZE;

    Int pos[SCAN_SET_SIZE];
    if( iScanPosSig == (Int) uiScanPosLast )
    {
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
      iScanPosSig--;
      pos[ numNonZero ] = uiBlkPosLast;
      numNonZero = 1;
    }

    // decode significant_coeffgroup_flag
    Int iCGBlkPos = scanCG[ iSubSet ];
    Int iCGPosY   = iCGBlkPos / uiNumBlkSide;
    Int iCGPosX   = iCGBlkPos - (iCGPosY * uiNumBlkSide);
    if( iSubSet == iLastScanSet || iSubSet == 0)
    {
      uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
    }
    else
    {
      UInt uiSigCoeffGroup;
      UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiWidth, uiHeight );
      m_pcTDecBinIf->decodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] );
      uiSigCoeffGroupFlag[ iCGBlkPos ] = uiSigCoeffGroup;
    }

    // decode significant_coeff_flag
    Int patternSigCtx = TComTrQuant::calcPatternSigCtx( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiWidth, uiHeight );
    UInt uiBlkPos, uiPosY, uiPosX, uiSig, uiCtxSig;
    for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
    {
      uiBlkPos  = scan[ iScanPosSig ];
      uiPosY    = uiBlkPos >> uiLog2BlockSize;
      uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
      uiSig     = 0;
      
      if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
      {
        if( iScanPosSig > iSubPos || iSubSet == 0  || numNonZero )
        {
          uiCtxSig  = TComTrQuant::getSigCtxInc( patternSigCtx, uiScanIdx, uiPosX, uiPosY, uiLog2BlockSize, eTType );
          m_pcTDecBinIf->decodeBin( uiSig, baseCtx[ uiCtxSig ] );
        }
        else
        {
          uiSig = 1;
        }
      }
      pcCoef[ uiBlkPos ] = uiSig;
      if( uiSig )
      {
        pos[ numNonZero ] = uiBlkPos;
        numNonZero ++;
        if( lastNZPosInCG == -1 )
        {
          lastNZPosInCG = iScanPosSig;
        }
        firstNZPosInCG = iScanPosSig;
      }
    }
    
    if( numNonZero )
    {
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD );
      absSum = 0;
      UInt uiCtxSet    = (iSubSet > 0 && eTType==TEXT_LUMA) ? 2 : 0;
      UInt uiBin;
      if( c1 == 0 )
      {
        uiCtxSet++;
      }
      c1 = 1;
      ContextModel *baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUOneSCModel.get( 0, 0 ) + 4 * uiCtxSet : m_cCUOneSCModel.get( 0, 0 ) + NUM_ONE_FLAG_CTX_LUMA + 4 * uiCtxSet;
      Int absCoeff[SCAN_SET_SIZE];

      for ( Int i = 0; i < numNonZero; i++) absCoeff[i] = 1;   
      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;

      for( Int idx = 0; idx < numC1Flag; idx++ )
      {
        m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[c1] );
        if( uiBin == 1 )
        {
          c1 = 0;
          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
        absCoeff[ idx ] = uiBin + 1;
      }
      
      if (c1 == 0)
      {
        baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUAbsSCModel.get( 0, 0 ) + uiCtxSet : m_cCUAbsSCModel.get( 0, 0 ) + NUM_ABS_FLAG_CTX_LUMA + uiCtxSet;
        if ( firstC2FlagIdx != -1)
        {
          m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[0] ); 
          absCoeff[ firstC2FlagIdx ] = uiBin + 2;
        }
      }

      UInt coeffSigns;
      if ( signHidden && beValid )
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero-1 );
        coeffSigns <<= 32 - (numNonZero-1);
      }
      else
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero );
        coeffSigns <<= 32 - numNonZero;
      }
      
      Int iFirstCoeff2 = 1;    
      if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
      {
        for( Int idx = 0; idx < numNonZero; idx++ )
        {
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2) : 1;

          if( absCoeff[ idx ] == baseLevel)
          {
            UInt uiLevel;
            xReadCoefRemainExGolomb( uiLevel, uiGoRiceParam );
            absCoeff[ idx ] = uiLevel + baseLevel;
            if(absCoeff[idx]>3*(1<<uiGoRiceParam))
            {
              uiGoRiceParam = min<UInt>(uiGoRiceParam+ 1, 4);
            }
          }

          if(absCoeff[ idx ] >= 2)  
          {
            iFirstCoeff2 = 0;
          }
        }
      }

      for( Int idx = 0; idx < numNonZero; idx++ )
      {
        Int blkPos = pos[ idx ];
        // Signs applied later.
        pcCoef[ blkPos ] = absCoeff[ idx ];
        absSum += absCoeff[ idx ];

        if ( idx == numNonZero-1 && signHidden && beValid )
        {
          // Infer sign of 1st element.
          if (absSum&0x1)
          {
            pcCoef[ blkPos ] = -pcCoef[ blkPos ];
          }
        }
        else
        {
          Int sign = static_cast<Int>( coeffSigns ) >> 31;
          pcCoef[ blkPos ] = ( pcCoef[ blkPos ] ^ sign ) - sign;
          coeffSigns <<= 1;
        }
      }
    }
  }
  
  return;
}


Void TDecSbac::parseSaoMaxUvlc ( UInt& val, UInt maxSymbol )
{
  if (maxSymbol == 0)
  {
    val = 0;
    return;
  }

  UInt code;
  Int  i;
  m_pcTDecBinIf->decodeBinEP( code );
  if ( code == 0 )
  {
    val = 0;
    return;
  }

  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBinEP( code );
    if ( code == 0 )
    {
      break;
    }
    i++;
    if (i == maxSymbol) 
    {
      break;
    }
  }

  val = i;
}
Void TDecSbac::parseSaoUflc (UInt uiLength, UInt&  riVal)
{
  m_pcTDecBinIf->decodeBinsEP ( riVal, uiLength );
}
Void TDecSbac::parseSaoMerge (UInt&  ruiVal)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoMergeSCModel.get( 0, 0, 0 ) );
  ruiVal = (Int)uiCode;
}
Void TDecSbac::parseSaoTypeIdx (UInt&  ruiVal)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
  if (uiCode == 0) 
  {
    ruiVal = 0;
  }
  else
  {
    m_pcTDecBinIf->decodeBinEP( uiCode ); 
    if (uiCode == 0)
    {
      ruiVal = 1;
    }
    else
    {
      ruiVal = 2;
    }
  }
}

Void TDecSbac::parseSaoSign(UInt& val)
{
  m_pcTDecBinIf->decodeBinEP ( val ); 
}

Void TDecSbac::parseSAOBlkParam (SAOBlkParam& saoBlkParam
                                , Bool* sliceEnabled
                                , Bool leftMergeAvail
                                , Bool aboveMergeAvail
                                )
{
  UInt uiSymbol;

  Bool isLeftMerge = false;
  Bool isAboveMerge= false;

  if(leftMergeAvail)
  {
    parseSaoMerge(uiSymbol); //sao_merge_left_flag
    isLeftMerge = (uiSymbol?true:false);
  }

  if( aboveMergeAvail && !isLeftMerge)
  {
    parseSaoMerge(uiSymbol); //sao_merge_up_flag
    isAboveMerge = (uiSymbol?true:false);
  }

  if(isLeftMerge || isAboveMerge) //merge mode
  {
    saoBlkParam[SAO_Y].modeIdc = saoBlkParam[SAO_Cb].modeIdc = saoBlkParam[SAO_Cr].modeIdc = SAO_MODE_MERGE;
    saoBlkParam[SAO_Y].typeIdc = saoBlkParam[SAO_Cb].typeIdc = saoBlkParam[SAO_Cr].typeIdc = (isLeftMerge)?SAO_MERGE_LEFT:SAO_MERGE_ABOVE;
  }
  else //new or off mode
  {    
    for(Int compIdx=0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
    {
      SAOOffset& ctbParam = saoBlkParam[compIdx];

      if(!sliceEnabled[compIdx])
      {
        //off
        ctbParam.modeIdc = SAO_MODE_OFF;
        continue;
      }

      //type
      if(compIdx == SAO_Y || compIdx == SAO_Cb)
      {
        parseSaoTypeIdx(uiSymbol); //sao_type_idx_luma or sao_type_idx_chroma

        assert(uiSymbol ==0 || uiSymbol ==1 || uiSymbol ==2);

        if(uiSymbol ==0) //OFF
        {
          ctbParam.modeIdc = SAO_MODE_OFF;
        }
        else if(uiSymbol == 1) //BO
        {
          ctbParam.modeIdc = SAO_MODE_NEW;
          ctbParam.typeIdc = SAO_TYPE_START_BO;
        }
        else //2, EO
        {
          ctbParam.modeIdc = SAO_MODE_NEW;
          ctbParam.typeIdc = SAO_TYPE_START_EO;
        }

      }
      else //Cr, follow Cb SAO type
      {
        ctbParam.modeIdc = saoBlkParam[SAO_Cb].modeIdc;
        ctbParam.typeIdc = saoBlkParam[SAO_Cb].typeIdc;
      }

      if(ctbParam.modeIdc == SAO_MODE_NEW)
      {
        Int offset[4];
        for(Int i=0; i< 4; i++)
        {
          parseSaoMaxUvlc(uiSymbol,  g_saoMaxOffsetQVal[compIdx] ); //sao_offset_abs
          offset[i] = (Int)uiSymbol;
        }

        if(ctbParam.typeIdc == SAO_TYPE_START_BO)
        {
          for(Int i=0; i< 4; i++)
          {
            if(offset[i] != 0)
            {
              parseSaoSign(uiSymbol); //sao_offset_sign
              if(uiSymbol)
              {
                offset[i] = -offset[i];
              }
            }
          }
          parseSaoUflc(NUM_SAO_BO_CLASSES_LOG2, uiSymbol ); //sao_band_position
          ctbParam.typeAuxInfo = uiSymbol;
        
          for(Int i=0; i<4; i++)
          {
            ctbParam.offset[(ctbParam.typeAuxInfo+i)%MAX_NUM_SAO_CLASSES] = offset[i];
          }      
        
        }
        else //EO
        {
          ctbParam.typeAuxInfo = 0;

          if(compIdx == SAO_Y || compIdx == SAO_Cb)
          {
            parseSaoUflc(NUM_SAO_EO_TYPES_LOG2, uiSymbol ); //sao_eo_class_luma or sao_eo_class_chroma
            ctbParam.typeIdc += uiSymbol;
          }
          else
          {
            ctbParam.typeIdc = saoBlkParam[SAO_Cb].typeIdc;
          }
          ctbParam.offset[SAO_CLASS_EO_FULL_VALLEY] = offset[0];
          ctbParam.offset[SAO_CLASS_EO_HALF_VALLEY] = offset[1];
          ctbParam.offset[SAO_CLASS_EO_PLAIN      ] = 0;
          ctbParam.offset[SAO_CLASS_EO_HALF_PEAK  ] = -offset[2];
          ctbParam.offset[SAO_CLASS_EO_FULL_PEAK  ] = -offset[3];
        }
      }
    }
  }
}

/**
 - Initialize our contexts from the nominated source.
 .
 \param pSrc Contexts to be copied.
 */
Void TDecSbac::xCopyContextsFrom( TDecSbac* pSrc )
{
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
}

Void TDecSbac::xCopyFrom( TDecSbac* pSrc )
{
  m_pcTDecBinIf->copyState( pSrc->m_pcTDecBinIf );

  m_uiLastQp           = pSrc->m_uiLastQp;
  xCopyContextsFrom( pSrc );

}

Void TDecSbac::load ( TDecSbac* pScr )
{
  xCopyFrom(pScr);
}

Void TDecSbac::loadContexts ( TDecSbac* pScr )
{
  xCopyContextsFrom(pScr);
}
//! \}
