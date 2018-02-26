/*
 * Copyright (c) 2013, 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of The Linux Foundation nor
 *      the names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WIFI_GBK2UTF_H_
#define WIFI_GBK2UTF_H_

#include <vector>

#include <utils/misc.h>
#include <utils/Log.h>

using std::vector;

namespace android {

struct accessPointObjectItem {
    vector<uint8_t> ssid;
    vector<uint8_t> utf_ssid;
    int tictok;
    struct  accessPointObjectItem *pNext;
};

extern int wifigbk_init();

extern int wifigbk_deinit();

extern bool wifigbk_isGbk(const vector<uint8_t>& ssid);

extern int wifigbk_toUtf(const vector<uint8_t>& ssid, vector<uint8_t>* utf_ssid);

extern int wifigbk_getFromHistory(const vector<uint8_t>& in_ssid, vector<uint8_t>& out_ssid);

extern void wifigbk_addToHistory(const vector<uint8_t>& ssid, const vector<uint8_t>& utf_ssid);

extern void wifigbk_ageOutHistory();

extern void wifigbk_dumpSsid(const char* tag, const vector<uint8_t>& ssid);

extern void wifigbk_dumpHistory();

} //namespace android

#endif  // WIFI_GBK2UTF_H_
