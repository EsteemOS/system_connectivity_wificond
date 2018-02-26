/*
 * Copyright (c) 2013, 2017, The Linux Foundation. All rights reserved.
 *
 * wpa_supplicant/hostapd / common helper functions, etc.
 * Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <unicode/ucnv.h>
#include <android-base/logging.h>

#include "wifi_gbk2utf.h"

#define CONVERT_LINE_LEN 512
#define CHARSET_CN "gbk"
#define MAX_TICTOK 1   // Aging in next <tictok> scan

namespace android {

pthread_mutex_t *g_pItemListMutex = NULL;
struct accessPointObjectItem *g_pItemList = NULL;


static bool __isSsidEual(const vector<uint8_t>& ssid1,
                         const vector<uint8_t>& ssid2)
{
    if (ssid1.size() != ssid2.size()) {
        return false;
    }

    for (size_t i = 0; i < ssid1.size(); i ++) {
       if(ssid1[i] != ssid2[i]) {
            return false;
       }
    }

    return true;
}


/* check if the SSID string is UTF coded */
static bool __isUTF8String(const unsigned char* str, int length)
{
    unsigned int nBytes = 0;
    unsigned char chr;
    bool bAllAscii = true;
    for (int i = 0; i < length; i++) {
        chr = *(str+i);
        if ((chr & 0x80) != 0) {
            bAllAscii = false;
        }
        if (0 == nBytes) {
            if (chr >= 0x80) {
                if (chr >= 0xFC && chr <= 0xFD) {
                    nBytes = 6;
                } else if (chr >= 0xF8) {
                    nBytes = 5;
                } else if (chr >= 0xF0) {
                    nBytes = 4;
                } else if (chr >= 0xE0) {
                    nBytes = 3;
                } else if (chr >= 0xC0) {
                    nBytes = 2;
                } else {
                    return false;
                }
                nBytes--;
            }
        } else {
            if ((chr & 0xC0) != 0x80) {
            return false;
            }
            nBytes--;
        }
    }

    if (nBytes > 0 || bAllAscii) {
        return false;
    }
    return true;
}


/*
 * https://en.wikipedia.org/wiki/GBK
 *
 * GBK character is encoded as 1 or 2 bytes.
 * - A single byte with range 0x00-0x7f is ASCII.
 * - A byte with the high bit set indicates that it is
 *   the first of 2 bytes.
 *   byte1: (0x81-0xFE)
 *   byte2: (0x40-0xFE) except 0x7F
 *
 * This function only returns true only it is GBK string
 * but not all character is ASCII.
 */
static bool __isGBKString(const unsigned char *str, int length)
{
    unsigned char byte1;
    unsigned char byte2;
    bool isAllASCII = true;

    for (int i = 0; i < length; i ++) {
        byte1 = *(str+i);

        if (byte1 >= 0x81 && byte1 < 0xFF && (i+1) < length) {
            byte2 = *(str+i+1);
            if (byte2 >= 0x40 && byte2 < 0xFF && byte2 != 0x7F) {
                // GBK
                isAllASCII = false;
                i ++;
                continue;
            } else {
                return false;
            }
        } else if (byte1 < 0x80){
            // ASCII
            continue;
        } else {
            return false;
        }
    }

    if (isAllASCII)
        return false;

    return true;
}


void wifigbk_dumpSsid(const char* tag, const vector<uint8_t>& ssid)
{
    char strbuf[200] = {0};
    size_t i;
    size_t len_show;

    const uint8_t *bytes = ssid.data();
    const size_t len = ssid.size();

    if (len) {
        len_show = (len > 32) ? 32 : len;

        for (i = 0; i < len_show; i++)
           snprintf(&strbuf[i * 3], 4, " %02x", bytes[i]);
    }

    LOG(INFO) << tag << "[len=" << len << "] " << strbuf;
}

void wifigbk_dumpHistory() {
#if WIFIGBK_DEBUG
    struct accessPointObjectItem *pTmpItemNode = NULL;
    pTmpItemNode = g_pItemList;
    LOG(INFO) << "*****";
    while (pTmpItemNode) {
        wifigbk_dumpSsid("dumpAPObjectItem ssid", pTmpItemNode->ssid);
        wifigbk_dumpSsid("dumpAPObjectItem utf_ssid", pTmpItemNode->utf_ssid);
        pTmpItemNode = pTmpItemNode->pNext;
    }
#endif
}


void wifigbk_addToHistory(const vector<uint8_t>& ssid,
                          const vector<uint8_t>& utf_ssid)
{
    struct accessPointObjectItem *pTmpItemNode = NULL;
    struct accessPointObjectItem *pItemNode = NULL;
    struct accessPointObjectItem *pLastNode = NULL;
    bool entryExist = false;

    pthread_mutex_lock(g_pItemListMutex);

    pTmpItemNode = g_pItemList;
    while (pTmpItemNode) {
        if (__isSsidEual(ssid, pTmpItemNode->ssid)) {
            entryExist = true;
            pTmpItemNode->tictok = MAX_TICTOK;
        }
        pLastNode = pTmpItemNode;
        pTmpItemNode = pTmpItemNode->pNext;
    }
    if (!entryExist) {
        pItemNode = new struct accessPointObjectItem();
        if (NULL == pItemNode) {
            goto EXIT;
        }
        memset(pItemNode, 0, sizeof(accessPointObjectItem));
        pItemNode->utf_ssid = utf_ssid;
        pItemNode->ssid = ssid;
        pItemNode->pNext = NULL;
        pItemNode->tictok = MAX_TICTOK;

        wifigbk_dumpSsid("wifigbk_addToHistory: GBK ", ssid);
        wifigbk_dumpSsid("wifigbk_addToHistory: UTF ", utf_ssid);

        if (NULL == g_pItemList) {
            g_pItemList = pItemNode;
        } else {
            pLastNode->pNext = pItemNode;
        }
    }

EXIT:
    pthread_mutex_unlock(g_pItemListMutex);
}

void wifigbk_ageOutHistory()
{
    struct accessPointObjectItem *agingNode = NULL;
    struct accessPointObjectItem *pItemNode = NULL;
    struct accessPointObjectItem *sItemNode = NULL;
    vector<uint8_t> v;

    LOG(INFO) << "Enters wifigbk_ageOutHistory";

    pthread_mutex_lock(g_pItemListMutex);

    pItemNode = g_pItemList;
    sItemNode = g_pItemList;
    while (pItemNode) {
        if (pItemNode->tictok > 0) {
            pItemNode->tictok --;
            sItemNode = pItemNode;
            pItemNode = pItemNode->pNext;
        } else {
            agingNode = pItemNode;

            if (pItemNode == g_pItemList) {
                g_pItemList = g_pItemList->pNext;
                pItemNode = g_pItemList;
                sItemNode = g_pItemList;
            } else {
                sItemNode->pNext = pItemNode->pNext;
                pItemNode = pItemNode->pNext;
            }

            if (agingNode != NULL) {
                wifigbk_dumpSsid("Aging GBK remove", agingNode->ssid);
                agingNode->ssid.swap(v);
                agingNode->utf_ssid.swap(v);
                delete agingNode;
                agingNode = NULL;
            }
        }
    }

    pthread_mutex_unlock(g_pItemListMutex);
    return;
}


int wifigbk_init() {
    if (!g_pItemListMutex) {
        g_pItemListMutex = new pthread_mutex_t;
        if (NULL == g_pItemListMutex) {
            return -1;
        }
        pthread_mutex_init(g_pItemListMutex, NULL);
    }

    return 0;
}


int wifigbk_deinit() {
    if (g_pItemListMutex != NULL) {
        pthread_mutex_lock(g_pItemListMutex);

        struct accessPointObjectItem *pCurrentNode = g_pItemList;
        struct accessPointObjectItem *pNextNode = NULL;
        vector<uint8_t> v;
        while (pCurrentNode) {
            pNextNode = pCurrentNode->pNext;
            pCurrentNode->ssid.swap(v);
            pCurrentNode->utf_ssid.swap(v);
            delete pCurrentNode;
            pCurrentNode = pNextNode;
        }
        g_pItemList = NULL;

        pthread_mutex_unlock(g_pItemListMutex);
        pthread_mutex_destroy(g_pItemListMutex);
        delete g_pItemListMutex;
        g_pItemListMutex = NULL;
    }
    return 0;
}


bool wifigbk_isGbk(const vector<uint8_t>& ssid)
{
    bool isUTF8;
    bool isGbk;
    const uint8_t *ssid_bytes = ssid.data();
    const size_t ssid_len = ssid.size();

    isUTF8 = __isUTF8String(ssid_bytes, ssid_len);
    isGbk = __isGBKString(ssid_bytes, ssid_len);

    if (!isUTF8 && isGbk) {
        return true;
    }

    return false;
}


int wifigbk_toUtf(const vector<uint8_t>& ssid, vector<uint8_t>* utf_ssid)
{
    UErrorCode err = U_ZERO_ERROR;
    const uint8_t *ssid_bytes = ssid.data();
    const size_t ssid_len = ssid.size();
    char buf[CONVERT_LINE_LEN] = {0};

    UConverter*  pConverter = ucnv_open(CHARSET_CN, &err);
    if (U_FAILURE(err)) {
        return -1;
    }

    ucnv_toAlgorithmic(UCNV_UTF8, pConverter, buf, CONVERT_LINE_LEN,
                        (const char*)ssid_bytes, ssid_len, &err);

    if (U_FAILURE(err)) {
        ucnv_close(pConverter);
        return -2;
    }

    *utf_ssid = vector<uint8_t>(buf, buf + strlen(buf));
    wifigbk_addToHistory(ssid, *utf_ssid);

    ucnv_close(pConverter);
    return 0;
}


int wifigbk_getFromHistory(const vector<uint8_t>& in_ssid,
                           vector<uint8_t>& out_ssid)
{
    int ret = -1;
    bool isGbk;
    struct accessPointObjectItem *pTmpItemNode = NULL;

    if (g_pItemListMutex == NULL) {
        LOG(INFO) << "wifigbk_toGbk fail as wifigbk is not inited.";
        return -2;
    }

    isGbk = wifigbk_isGbk(in_ssid);
    pthread_mutex_lock(g_pItemListMutex);

    pTmpItemNode = g_pItemList;
    while (pTmpItemNode) {
        if (__isSsidEual(in_ssid,
                         isGbk? pTmpItemNode->ssid : pTmpItemNode->utf_ssid)) {
            ret = 0;
            out_ssid = isGbk ? pTmpItemNode->utf_ssid : pTmpItemNode->ssid;
            goto EXIT;
        }
        pTmpItemNode = pTmpItemNode->pNext;
    }

EXIT:
    pthread_mutex_unlock(g_pItemListMutex);
    return ret;
}


} //namespace android
