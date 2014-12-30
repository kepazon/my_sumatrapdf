/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// Re-declare variables for when building libmupdf.dll,
// as exporting/importing them prevents sharing of .obj
// files for all files using them - instead we can just
// link this file along libmupdf.lib and omit it when
// building a static SumatraPDF.exe.

extern "C" {
#include <mupdf/fitz/math.h>
}

// copied from mupdf/source/fitz/geometry.c

const fz_matrix fz_identity = { 1, 0, 0, 1, 0, 0 };

const fz_rect fz_infinite_rect = { 1, 1, -1, -1 };
const fz_rect fz_empty_rect = { 0, 0, 0, 0 };
const fz_rect fz_unit_rect = { 0, 0, 1, 1 };

const fz_irect fz_infinite_irect = { 1, 1, -1, -1 };
const fz_irect fz_empty_irect = { 0, 0, 0, 0 };
const fz_irect fz_unit_bbox = { 0, 0, 1, 1 };

// adapted for mupdf/source/fitz/gdiplus-device.cpp

void RedirectDllIOToConsole() { fz_redirect_io_to_console(); }
