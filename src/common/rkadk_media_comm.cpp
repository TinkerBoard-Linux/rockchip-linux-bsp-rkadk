/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "rkadk_media_comm.h"
#include "rkadk_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  bool bUsed;
  RKADK_S32 s32BindCnt;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;
} RKADK_BIND_INFO_S;

typedef struct {
  bool bUsed;
  RKADK_S32 s32InitCnt;
  RKADK_S32 s32ChnId;
  CODEC_TYPE_E enCodecType;
} RKADK_MEDIA_INFO_S;

typedef struct {
  pthread_mutex_t CntMutex;
  RKADK_MEDIA_INFO_S stAiInfo[RKADK_MEDIA_AI_MAX_CNT];
  RKADK_MEDIA_INFO_S stAencInfo[RKADK_MEDIA_AENC_MAX_CNT];
  RKADK_MEDIA_INFO_S stViInfo[RKADK_MEDIA_VI_MAX_CNT];
  RKADK_BIND_INFO_S stAiAencInfo[RKADK_AI_AENC_MAX_BIND_CNT];
} RKADK_MEDIA_CONTEXT_S;

static bool g_bMediaCtxInit = false;
static RKADK_MEDIA_CONTEXT_S g_stMediaCtx;

static void RKADK_MEDIA_CtxInit() {
  if (g_bMediaCtxInit)
    return;

  memset(&g_stMediaCtx, 0, sizeof(RKADK_MEDIA_CONTEXT_S));
  g_bMediaCtxInit = true;
}

static RKADK_U32 RKADK_MEDIA_FindUsableIdx(RKADK_MEDIA_INFO_S *pstInfo,
                                           int count, const char *mode) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed) {
      RKADK_LOGD("%s: find usable index[%d]", mode, i);
      return i;
    }
  }

  return -1;
}

static RKADK_S32 RKADK_MEDIA_GetIdx(RKADK_MEDIA_INFO_S *pstInfo, int count,
                                    RKADK_S32 s32ChnId, const char *mode) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo[i].bUsed)
      continue;

    if (pstInfo[i].s32ChnId == s32ChnId) {
      RKADK_LOGD("%s: find matched index[%d] ChnId[%d]", mode, i, s32ChnId);
      return i;
    }
  }

  return -1;
}

RKADK_S32 RKADK_MPI_AI_Init(RKADK_S32 s32AiChnId, AI_CHN_ATTR_S *pstAiChnAttr) {
  int ret;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstAiChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                         s32AiChnId, "AI_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                                  "AI_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      return -1;
    }
  }

  if (0 == g_stMediaCtx.stAiInfo[i].s32InitCnt) {
    ret = RK_MPI_AI_SetChnAttr(s32AiChnId, pstAiChnAttr);
    if (ret) {
      RKADK_LOGE("Set AI[%d] attribute failed(%d)", s32AiChnId, ret);
      return ret;
    }

    ret = RK_MPI_AI_EnableChn(s32AiChnId);
    if (ret) {
      RKADK_LOGE("Create AI[%d] failed(%d)", s32AiChnId, ret);
      return ret;
    }

    g_stMediaCtx.stAiInfo[i].bUsed = true;
    g_stMediaCtx.stAiInfo[i].s32ChnId = s32AiChnId;
  }

  g_stMediaCtx.stAiInfo[i].s32InitCnt++;
  RKADK_LOGD("aiChnId[%d], InitCnt[%d]", s32AiChnId,
             g_stMediaCtx.stAiInfo[i].s32InitCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_AI_DeInit(RKADK_S32 s32AiChnId) {
  int ret;
  RKADK_S32 i;

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAiInfo, RKADK_MEDIA_AI_MAX_CNT,
                         s32AiChnId, "AI_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32AiChnId[%d]", i, s32AiChnId);
    return -1;
  }

  RKADK_S32 s32InitCnt = g_stMediaCtx.stAiInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("aiChnId[%d] has already deinit", s32AiChnId);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_AI_DisableChn(s32AiChnId);
    if (ret) {
      RKADK_LOGE("Disable AI[%d] error %d", s32AiChnId, ret);
      return ret;
    }

    g_stMediaCtx.stAiInfo[i].bUsed = false;
    g_stMediaCtx.stAiInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stAiInfo[i].s32InitCnt--;
  RKADK_LOGD("aiChnId[%d], InitCnt[%d]", s32AiChnId,
             g_stMediaCtx.stAiInfo[i].s32InitCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_AENC_Init(RKADK_S32 s32AencChnId,
                              AENC_CHN_ATTR_S *pstAencChnAttr) {
  int ret;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstAencChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAencInfo, RKADK_MEDIA_AENC_MAX_CNT,
                         s32AencChnId, "AENC_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stAencInfo,
                                  RKADK_MEDIA_AENC_MAX_CNT, "AENC_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      return -1;
    }
  } else {
    if (g_stMediaCtx.stAencInfo[i].enCodecType != pstAencChnAttr->enCodecType) {
      RKADK_LOGE("find matched index[%d], but CodecType inequality[%d, %d]", i,
                 g_stMediaCtx.stAencInfo[i].enCodecType,
                 pstAencChnAttr->enCodecType);
      return -1;
    }
  }

  if (0 == g_stMediaCtx.stAencInfo[i].s32InitCnt) {
    ret = RK_MPI_AENC_CreateChn(s32AencChnId, pstAencChnAttr);
    if (ret) {
      RKADK_LOGE("Create AENC[%d] failed(%d)", s32AencChnId, ret);
      return ret;
    }

    g_stMediaCtx.stAencInfo[i].bUsed = true;
    g_stMediaCtx.stAencInfo[i].s32ChnId = s32AencChnId;
    g_stMediaCtx.stAencInfo[i].enCodecType = pstAencChnAttr->enCodecType;
  }

  g_stMediaCtx.stAencInfo[i].s32InitCnt++;
  RKADK_LOGD("aencChnId[%d], InitCnt[%d]", s32AencChnId,
             g_stMediaCtx.stAencInfo[i].s32InitCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_AENC_DeInit(RKADK_S32 s32AencChnId) {
  int ret;
  RKADK_S32 i;

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stAencInfo, RKADK_MEDIA_AENC_MAX_CNT,
                         s32AencChnId, "AENC_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32AencChnId[%d]", i, s32AencChnId);
    return -1;
  }

  RKADK_S32 s32InitCnt = g_stMediaCtx.stAencInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("aencChnId[%d] has already deinit", s32AencChnId);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_AENC_DestroyChn(s32AencChnId);
    if (ret) {
      RKADK_LOGE("Destroy AENC[%d] error %d", s32AencChnId, ret);
      return ret;
    }

    g_stMediaCtx.stAencInfo[i].bUsed = false;
    g_stMediaCtx.stAencInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stAencInfo[i].s32InitCnt--;
  RKADK_LOGD("aencChnId[%d], InitCnt[%d]", s32AencChnId,
             g_stMediaCtx.stAencInfo[i].s32InitCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_VI_Init(RKADK_U32 u32CamId, RKADK_S32 s32ViChnId,
                            VI_CHN_ATTR_S *pstViChnAttr) {
  int ret;
  RKADK_S32 i;

  RKADK_CHECK_POINTER(pstViChnAttr, RKADK_FAILURE);
  RKADK_MEDIA_CtxInit();

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                         s32ViChnId, "VI_INIT");
  if (i < 0) {
    i = RKADK_MEDIA_FindUsableIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                                  "VI_INIT");
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      return -1;
    }
  }

  if (0 == g_stMediaCtx.stViInfo[i].s32InitCnt) {
    ret = RK_MPI_VI_SetChnAttr(u32CamId, s32ViChnId, pstViChnAttr);
    if (ret) {
      RKADK_LOGE("Set VI[%d] attribute error %d", s32ViChnId, ret);
      return ret;
    }

    ret = RK_MPI_VI_EnableChn(u32CamId, s32ViChnId);
    if (ret) {
      RKADK_LOGE("Create VI[%d] error %d", s32ViChnId, ret);
      return ret;
    }

    g_stMediaCtx.stViInfo[i].bUsed = true;
    g_stMediaCtx.stViInfo[i].s32ChnId = s32ViChnId;
  }

  g_stMediaCtx.stViInfo[i].s32InitCnt++;
  RKADK_LOGD("viChnId[%d], InitCnt[%d]", s32ViChnId,
             g_stMediaCtx.stViInfo[i].s32InitCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_VI_DeInit(RKADK_U32 u32CamId, RKADK_S32 s32ViChnId) {
  int ret;
  RKADK_S32 i;

  i = RKADK_MEDIA_GetIdx(g_stMediaCtx.stViInfo, RKADK_MEDIA_VI_MAX_CNT,
                         s32ViChnId, "VI_DEINIT");
  if (i < 0) {
    RKADK_LOGE("not find matched index[%d] s32ChnId[%d]", i, s32ViChnId);
    return -1;
  }

  RKADK_S32 s32InitCnt = g_stMediaCtx.stViInfo[i].s32InitCnt;
  if (0 == s32InitCnt) {
    RKADK_LOGD("viChnId[%d] has already deinit", s32ViChnId);
    return 0;
  } else if (1 == s32InitCnt) {
    ret = RK_MPI_VI_DisableChn(u32CamId, s32ViChnId);
    if (ret) {
      RKADK_LOGE("Destory VI[%d] failed, ret=%d", s32ViChnId, ret);
      return ret;
    }

    g_stMediaCtx.stViInfo[i].bUsed = false;
    g_stMediaCtx.stViInfo[i].s32ChnId = 0;
  }

  g_stMediaCtx.stViInfo[i].s32InitCnt--;
  RKADK_LOGD("viChnId[%d], InitCnt[%d]", s32ViChnId,
             g_stMediaCtx.stViInfo[i].s32InitCnt);

  return 0;
}

static RKADK_U32 RKADK_BIND_FindUsableIdx(RKADK_BIND_INFO_S *pstInfo,
                                          int count) {
  for (int i = 0; i < count; i++) {
    if (!pstInfo->bUsed) {
      RKADK_LOGD("find usable index[%d]", i);
      return i;
    }
  }

  return -1;
}

static RKADK_S32 RKADK_BIND_GetIdx(RKADK_BIND_INFO_S *pstInfo, int count,
                                   const MPP_CHN_S *pstSrcChn,
                                   const MPP_CHN_S *pstDestChn) {
  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  for (int i = 0; i < count; i++) {
    if (!pstInfo->bUsed)
      continue;

    if (pstInfo->stSrcChn.s32ChnId == pstSrcChn->s32ChnId &&
        pstInfo->stSrcChn.enModId == pstSrcChn->enModId &&
        pstInfo->stDestChn.s32ChnId == pstDestChn->s32ChnId &&
        pstInfo->stDestChn.enModId == pstDestChn->enModId) {
      RKADK_LOGD("find matched index[%d]: src ChnId[%d] dest ChnId[%d]", i,
                 pstSrcChn->s32ChnId, pstDestChn->s32ChnId);
      return i;
    }
  }

  return -1;
}

RKADK_S32 RKADK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn,
                             const MPP_CHN_S *pstDestChn) {
  int ret, count;
  RKADK_S32 i;
  RKADK_BIND_INFO_S *pstInfo = NULL;

  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  if (pstSrcChn->enModId == RK_ID_AI && pstDestChn->enModId == RK_ID_AENC) {
    count = RKADK_AI_AENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stAiAencInfo;
  } else {
    RKADK_LOGE("Nonsupport: src enModId: %d, dest enModId: %d",
               pstSrcChn->enModId, pstDestChn->enModId);
    return -1;
  }

  i = RKADK_BIND_GetIdx(pstInfo, count, pstSrcChn, pstDestChn);
  if (i < 0) {
    i = RKADK_BIND_FindUsableIdx(pstInfo, count);
    if (i < 0) {
      RKADK_LOGE("not find usable index");
      return -1;
    }
  }

  if (0 == pstInfo[i].s32BindCnt) {
    ret = RK_MPI_SYS_Bind(pstSrcChn, pstDestChn);
    if (ret) {
      RKADK_LOGE("Bind src[%d %d] and dest[%d %d] failed(%d)",
                 pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
                 pstDestChn->s32ChnId, ret);
      return ret;
    }

    pstInfo[i].bUsed = true;
    memcpy(&pstInfo[i].stSrcChn, pstSrcChn, sizeof(MPP_CHN_S));
    memcpy(&pstInfo[i].stDestChn, pstDestChn, sizeof(MPP_CHN_S));
  }

  pstInfo[i].s32BindCnt++;
  RKADK_LOGD("src[%d, %d], dest[%d, %d], BindCnt[%d]", pstSrcChn->enModId,
             pstSrcChn->s32ChnId, pstDestChn->enModId, pstDestChn->s32ChnId,
             pstInfo[i].s32BindCnt);

  return 0;
}

RKADK_S32 RKADK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn,
                               const MPP_CHN_S *pstDestChn) {
  int ret, count;
  RKADK_S32 i;
  RKADK_BIND_INFO_S *pstInfo = NULL;

  RKADK_CHECK_POINTER(pstSrcChn, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstDestChn, RKADK_FAILURE);

  if (pstSrcChn->enModId == RK_ID_AI && pstDestChn->enModId == RK_ID_AENC) {
    count = RKADK_AI_AENC_MAX_BIND_CNT;
    pstInfo = g_stMediaCtx.stAiAencInfo;
  } else {
    RKADK_LOGE("Nonsupport");
    return -1;
  }

  i = RKADK_BIND_GetIdx(pstInfo, count, pstSrcChn, pstDestChn);
  if (i < 0) {
    RKADK_LOGE("not find usable index");
    return -1;
  }

  if (0 == pstInfo[i].s32BindCnt) {
    RKADK_LOGD("src[%d, %d], dest[%d, %d] has already UnBind",
               pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
               pstDestChn->s32ChnId);
    return 0;
  } else if (1 == pstInfo[i].s32BindCnt) {
    ret = RK_MPI_SYS_UnBind(pstSrcChn, pstDestChn);
    if (ret) {
      RKADK_LOGE("UnBind src[%d, %d] and dest[%d, %d] failed(%d)",
                 pstSrcChn->enModId, pstSrcChn->s32ChnId, pstDestChn->enModId,
                 pstDestChn->s32ChnId, ret);
      return ret;
    }

    pstInfo[i].bUsed = false;
    memset(&pstInfo[i].stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&pstInfo[i].stDestChn, 0, sizeof(MPP_CHN_S));
  }

  pstInfo[i].s32BindCnt--;
  RKADK_LOGD("src[%d, %d], dest[%d, %d], BindCnt[%d]", pstSrcChn->enModId,
             pstSrcChn->s32ChnId, pstDestChn->enModId, pstDestChn->s32ChnId,
             pstInfo[i].s32BindCnt);

  return 0;
}

CODEC_TYPE_E RKADK_MEDIA_GetRkCodecType(RKADK_CODEC_TYPE_E enType) {
  CODEC_TYPE_E enCodecType;

  switch (enType) {
  case RKADK_CODEC_TYPE_H264:
    enCodecType = RK_CODEC_TYPE_H264;
    break;

  case RKADK_CODEC_TYPE_H265:
    enCodecType = RK_CODEC_TYPE_H265;
    break;

  case RKADK_CODEC_TYPE_MJPEG:
    enCodecType = RK_CODEC_TYPE_MJPEG;
    break;

  case RKADK_CODEC_TYPE_JPEG:
    enCodecType = RK_CODEC_TYPE_JPEG;
    break;

  case RKADK_CODEC_TYPE_MP3:
    enCodecType = RK_CODEC_TYPE_MP3;
    break;

  case RKADK_CODEC_TYPE_G711A:
    enCodecType = RK_CODEC_TYPE_G711A;
    break;

  case RKADK_CODEC_TYPE_G711U:
    enCodecType = RK_CODEC_TYPE_G711U;
    break;

  case RKADK_CODEC_TYPE_G726:
    enCodecType = RK_CODEC_TYPE_G726;
    break;

  case RKADK_CODEC_TYPE_MP2:
    enCodecType = RK_CODEC_TYPE_MP2;
    break;

  default:
    enCodecType = RK_CODEC_TYPE_NONE;
    break;
  }

  return enCodecType;
}

RKADK_CODEC_TYPE_E RKADK_MEDIA_GetCodecType(CODEC_TYPE_E enType) {
  RKADK_CODEC_TYPE_E enCodecType;

  switch (enType) {
  case RK_CODEC_TYPE_H264:
    enCodecType = RKADK_CODEC_TYPE_H264;
    break;

  case RK_CODEC_TYPE_H265:
    enCodecType = RKADK_CODEC_TYPE_H265;
    break;

  case RK_CODEC_TYPE_MJPEG:
    enCodecType = RKADK_CODEC_TYPE_MJPEG;
    break;

  case RK_CODEC_TYPE_JPEG:
    enCodecType = RKADK_CODEC_TYPE_JPEG;
    break;

  case RK_CODEC_TYPE_MP3:
    enCodecType = RKADK_CODEC_TYPE_MP3;
    break;
  case RK_CODEC_TYPE_G711A:
    enCodecType = RKADK_CODEC_TYPE_G711A;
    break;
  case RK_CODEC_TYPE_G711U:
    enCodecType = RKADK_CODEC_TYPE_G711U;
    break;

  case RK_CODEC_TYPE_G726:
    enCodecType = RKADK_CODEC_TYPE_G726;
    break;

  case RK_CODEC_TYPE_MP2:
    enCodecType = RKADK_CODEC_TYPE_MP2;
    break;

  default:
    enCodecType = RKADK_CODEC_TYPE_BUTT;
    break;
  }

  return enCodecType;
}