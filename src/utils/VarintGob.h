/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

int VarintGobEncode(int64_t val, uint8_t *d, int dLen);
int VarintGobDecode(const uint8_t *d, int dLen, int64_t *resOut);

int UVarintGobEncode(uint64_t val, uint8_t *d, int dLen);
int UVarintGobDecode(const uint8_t *d, int dLen, uint64_t *resOut);
