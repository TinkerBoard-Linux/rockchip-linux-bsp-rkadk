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

#ifndef __RKADK_THUMB_COMM_H__
#define __RKADK_THUMB_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_media_comm.h"

RKADK_S32 ThumbnailInit(RKADK_U32 u32CamId, RKADK_U32 thumb_width,
                        RKADK_U32 thumb_height, RKADK_U32 venc_chn,
                        RKADK_U32 vi_chn);

RKADK_S32 ThumbnailDeInit(RKADK_U32 u32CamId, RKADK_U32 venc_chn,
                          RKADK_U32 vi_chn);

RKADK_S32 ThumbnailPhotoData(RKADK_U8 *pJpegdata, RKADK_U32 JpegLen,
                             VENC_STREAM_S stThuFrame,
                             RKADK_U8 *pNewPhoto);

RKADK_S32 ThumbnailChnBind(RKADK_U32 u32VencChn, RKADK_U32 u32VencChnTb);

RKADK_S32 ThumbnailBuildIn(RKADK_CHAR *pszFileName,
                           RKADK_THUMB_ATTR_S *pstThumbAttr);

RKADK_S32 ThumbnailSaveFile(RKADK_CHAR *pszFileName, unsigned char *buf, RKADK_U32 size);

RKADK_S32 ThumbnailRenameFile(RKADK_CHAR *pszFileName);

RKADK_S32 ThumbnailFileInit();

#ifdef __cplusplus
}
#endif
#endif