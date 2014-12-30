/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// utils
#include "BaseUtil.h"
#include "Dpi.h"
#include "FileUtil.h"
#include "WinUtil.h"
// layout controllers
#include "BaseEngine.h"
#include "SettingsStructs.h"
#include "FileHistory.h"
#include "GlobalPrefs.h"
// ui
#include "SumatraPDF.h"
#include "WindowInfo.h"
#include "resource.h"
#include "FileThumbnails.h"
#include "SumatraAbout.h"
#include "Translations.h"
#include "Version.h"

#ifndef ABOUT_USE_LESS_COLORS
#define ABOUT_LINE_OUTER_SIZE       2
#else
#define ABOUT_LINE_OUTER_SIZE       1
#endif
#define ABOUT_LINE_SEP_SIZE         1
#define ABOUT_LEFT_RIGHT_SPACE_DX   8
#define ABOUT_MARGIN_DX            10
#define ABOUT_BOX_MARGIN_DY         6
#define ABOUT_BORDER_COL            RGB(0, 0, 0)
#define ABOUT_TXT_DY                6
#define ABOUT_RECT_PADDING          8
#define ABOUT_INNER_PADDING         6

#define ABOUT_CLASS_NAME        L"SUMATRA_PDF_ABOUT"

#define ABOUT_WIN_TITLE         _TR("About SumatraPDF")

#define COL_BLUE_LINK           RGB(0x00, 0x20, 0xa0)

#define SUMATRA_TXT_FONT        L"Arial Black"
#define SUMATRA_TXT_FONT_SIZE   24

#define VERSION_TXT_FONT        L"Arial Black"
#define VERSION_TXT_FONT_SIZE   12

#define VERSION_TXT             L"v" CURR_VERSION_STR
#ifdef SVN_PRE_RELEASE_VER
 #define VERSION_SUB_TXT        L"Pre-release"
#else
 #define VERSION_SUB_TXT        L""
#endif

#ifdef GIT_COMMIT_ID
#define GIT_COMMIT_ID_STR       TEXT(QM(GIT_COMMIT_ID))
#endif

// TODO: replace this link with a better one where license information is nicely collected/linked
#if defined(SVN_PRE_RELEASE_VER) || defined(DEBUG)
#define URL_LICENSE L"https://github.com/sumatrapdfreader/sumatrapdf/blob/master/AUTHORS"
#define URL_AUTHORS L"https://github.com/sumatrapdfreader/sumatrapdf/blob/master/AUTHORS"
#define URL_TRANSLATORS L"https://github.com/sumatrapdfreader/sumatrapdf/blob/master/TRANSLATORS"
#else
#define URL_LICENSE L"https://github.com/sumatrapdfreader/sumatrapdf/blob/" UPDATE_CHECK_VER L"rel/AUTHORS"
#define URL_AUTHORS L"https://github.com/sumatrapdfreader/sumatrapdf/blob/" UPDATE_CHECK_VER L"rel/AUTHORS"
#define URL_TRANSLATORS L"https://github.com/sumatrapdfreader/sumatrapdf/blob/" UPDATE_CHECK_VER L"rel/TRANSLATORS"
#endif

#define LAYOUT_LTR              0

static ATOM gAtomAbout;
static HWND gHwndAbout;
static HWND gHwndAboutTooltip = nullptr;
static const WCHAR *gClickedURL = nullptr;

struct AboutLayoutInfoEl {
    /* static data, must be provided */
    const WCHAR *   leftTxt;
    const WCHAR *   rightTxt;
    const WCHAR *   url;

    /* data calculated by the layout */
    RectI           leftPos;
    RectI           rightPos;
};

static AboutLayoutInfoEl gAboutLayoutInfo[] = {
    { L"website",        L"SumatraPDF website",   WEBSITE_MAIN_URL},
    { L"manual",         L"SumatraPDF manual",    WEBSITE_MANUAL_URL },
    { L"forums",         L"SumatraPDF forums",    L"http://blog.kowalczyk.info/forum_sumatra" },
    { L"programming",    L"The Programmers",      URL_AUTHORS },
    { L"translations",   L"The Translators",      URL_TRANSLATORS },
    { L"licenses",       L"Various Open Source",  URL_LICENSE },
#ifdef GIT_COMMIT_ID
    // TODO: use short ID for rightTxt (only first 7 digits) with less hackery
    { L"last change",    L"git commit " GIT_COMMIT_ID_STR, L"https://github.com/sumatrapdfreader/sumatrapdf/commit/" GIT_COMMIT_ID_STR },
#endif
#ifdef SVN_PRE_RELEASE_VER
    { L"a note",         L"Pre-release version, for testing only!", nullptr },
#endif
#ifdef DEBUG
    { L"a note",         L"Debug version, for testing only!", nullptr },
#endif
    { nullptr, nullptr, nullptr }
};

static Vec<StaticLinkInfo> gLinkInfo;

#define COL1 RGB(196, 64, 50)
#define COL2 RGB(227, 107, 35)
#define COL3 RGB(93,  160, 40)
#define COL4 RGB(69, 132, 190)
#define COL5 RGB(112, 115, 207)

static void DrawSumatraPDF(HDC hdc, PointI pt)
{
    const WCHAR *txt = APP_NAME_STR;
#ifdef ABOUT_USE_LESS_COLORS
    // simple black version
    SetTextColor(hdc, ABOUT_BORDER_COL);
    TextOut(hdc, pt.x, pt.y, txt, (int)str::Len(txt));
#else
    // colorful version
    COLORREF cols[] = { COL1, COL2, COL3, COL4, COL5, COL5, COL4, COL3, COL2, COL1 };
    for (size_t i = 0; i < str::Len(txt); i++) {
        SetTextColor(hdc, cols[i % dimof(cols)]);
        TextOut(hdc, pt.x, pt.y, txt + i, 1);

        SIZE txtSize;
        GetTextExtentPoint32(hdc, txt + i, 1, &txtSize);
        pt.x += txtSize.cx;
    }
#endif
}

static SizeI CalcSumatraVersionSize(HDC hdc)
{
    SizeI result;

    ScopedFont fontSumatraTxt(CreateSimpleFont(hdc, SUMATRA_TXT_FONT, SUMATRA_TXT_FONT_SIZE));
    ScopedFont fontVersionTxt(CreateSimpleFont(hdc, VERSION_TXT_FONT, VERSION_TXT_FONT_SIZE));
    ScopedHdcSelect selFont(hdc, fontSumatraTxt);

    SIZE txtSize;
    /* calculate minimal top box size */
    const WCHAR *txt = APP_NAME_STR;
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    result.dy = txtSize.cy + ABOUT_BOX_MARGIN_DY * 2;
    result.dx = txtSize.cx;

    /* consider version and version-sub strings */
    SelectObject(hdc, fontVersionTxt);
    txt = VERSION_TXT;
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    LONG minWidth = txtSize.cx;
    txt = VERSION_SUB_TXT;
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    txtSize.cx = std::max(txtSize.cx, minWidth);
    result.dx += 2 * (txtSize.cx + ABOUT_INNER_PADDING);

    return result;
}

static void DrawSumatraVersion(HDC hdc, RectI rect)
{
    ScopedFont fontSumatraTxt(CreateSimpleFont(hdc, SUMATRA_TXT_FONT, SUMATRA_TXT_FONT_SIZE));
    ScopedFont fontVersionTxt(CreateSimpleFont(hdc, VERSION_TXT_FONT, VERSION_TXT_FONT_SIZE));
    HGDIOBJ oldFont = SelectObject(hdc, fontSumatraTxt);

    SetBkMode(hdc, TRANSPARENT);

    SIZE txtSize;
    const WCHAR *txt = APP_NAME_STR;
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    RectI mainRect(rect.x + (rect.dx - txtSize.cx) / 2,
                   rect.y + (rect.dy - txtSize.cy) / 2, txtSize.cx, txtSize.cy);
    DrawSumatraPDF(hdc, mainRect.TL());

    SetTextColor(hdc, WIN_COL_BLACK);
    SelectObject(hdc, fontVersionTxt);
    PointI pt(mainRect.x + mainRect.dx + ABOUT_INNER_PADDING, mainRect.y);
    txt = VERSION_TXT;
    TextOut(hdc, pt.x, pt.y, txt, (int)str::Len(txt));
    txt = VERSION_SUB_TXT;
    TextOut(hdc, pt.x, pt.y + 16, txt, (int)str::Len(txt));

    SelectObject(hdc, oldFont);
}

static RectI DrawBottomRightLink(HWND hwnd, HDC hdc, const WCHAR *txt)
{
    ScopedFont fontLeftTxt(CreateSimpleFont(hdc, L"MS Shell Dlg", 14));
    ScopedGdiObj<HPEN> penLinkLine(CreatePen(PS_SOLID, 1, COL_BLUE_LINK));
    ScopedHdcSelect font(hdc, fontLeftTxt);

    SetTextColor(hdc, COL_BLUE_LINK);
    SetBkMode(hdc, TRANSPARENT);
    ClientRect rc(hwnd);

    SIZE txtSize;
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    RectI rect(rc.dx - txtSize.cx - ABOUT_INNER_PADDING,
               rc.y + rc.dy - txtSize.cy - ABOUT_INNER_PADDING, txtSize.cx, txtSize.cy);
    if (IsUIRightToLeft())
        rect.x = ABOUT_INNER_PADDING;
    RECT rTmp = rect.ToRECT();
    DrawText(hdc, txt, -1, &rTmp, IsUIRightToLeft() ? DT_RTLREADING : DT_LEFT);

    SelectObject(hdc, penLinkLine);
    PaintLine(hdc, RectI(rect.x, rect.y + rect.dy, rect.dx, 0));

    // make the click target larger
    rect.Inflate(ABOUT_INNER_PADDING, ABOUT_INNER_PADDING);
    return rect;
}

/* Draws the about screen and remembers some state for hyperlinking.
   It transcribes the design I did in graphics software - hopeless
   to understand without seeing the design. */
static void DrawAbout(HWND hwnd, HDC hdc, RectI rect, Vec<StaticLinkInfo>& linkInfo)
{
    HPEN penBorder = CreatePen(PS_SOLID, ABOUT_LINE_OUTER_SIZE, WIN_COL_BLACK);
    HPEN penDivideLine = CreatePen(PS_SOLID, ABOUT_LINE_SEP_SIZE, WIN_COL_BLACK);
    HPEN penLinkLine = CreatePen(PS_SOLID, ABOUT_LINE_SEP_SIZE, COL_BLUE_LINK);

    ScopedFont fontLeftTxt(CreateSimpleFont(hdc, LEFT_TXT_FONT, LEFT_TXT_FONT_SIZE));
    ScopedFont fontRightTxt(CreateSimpleFont(hdc, RIGHT_TXT_FONT, RIGHT_TXT_FONT_SIZE));

    HGDIOBJ origFont = SelectObject(hdc, fontLeftTxt); /* Just to remember the orig font */

    ClientRect rc(hwnd);
    RECT rTmp = rc.ToRECT();
    ScopedGdiObj<HBRUSH> brushAboutBg(CreateSolidBrush(GetAboutBgColor()));
    FillRect(hdc, &rTmp, brushAboutBg);

    /* render title */
    RectI titleRect(rect.TL(), CalcSumatraVersionSize(hdc));

    ScopedGdiObj<HBRUSH> bgBrush(CreateSolidBrush(GetLogoBgColor()));
    SelectObject(hdc, bgBrush);
    SelectObject(hdc, penBorder);
#ifndef ABOUT_USE_LESS_COLORS
    Rectangle(hdc, rect.x, rect.y + ABOUT_LINE_OUTER_SIZE, rect.x + rect.dx, rect.y + titleRect.dy + ABOUT_LINE_OUTER_SIZE);
#else
    RectI titleBgBand(0, rect.y, rc.dx, titleRect.dy);
    RECT rcLogoBg = titleBgBand.ToRECT();
    FillRect(hdc, &rcLogoBg, bgBrush);
    PaintLine(hdc, RectI(0, rect.y, rc.dx, 0));
    PaintLine(hdc, RectI(0, rect.y + titleRect.dy, rc.dx, 0));
#endif

    titleRect.Offset((rect.dx - titleRect.dx) / 2, 0);
    DrawSumatraVersion(hdc, titleRect);

    /* render attribution box */
    SetTextColor(hdc, ABOUT_BORDER_COL);
    SetBkMode(hdc, TRANSPARENT);

#ifndef ABOUT_USE_LESS_COLORS
    Rectangle(hdc, rect.x, rect.y + titleRect.dy, rect.x + rect.dx, rect.y + rect.dy);
#endif

    /* render text on the left*/
    SelectObject(hdc, fontLeftTxt);
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        TextOut(hdc, el->leftPos.x, el->leftPos.y, el->leftTxt, (int)str::Len(el->leftTxt));
    }

    /* render text on the right */
    SelectObject(hdc, fontRightTxt);
    SelectObject(hdc, penLinkLine);
    linkInfo.Reset();
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        bool hasUrl = HasPermission(Perm_DiskAccess) && el->url;
        SetTextColor(hdc, hasUrl ? COL_BLUE_LINK : ABOUT_BORDER_COL);
        int txtLen = (int)str::Len(el->rightTxt);
#ifdef GIT_COMMIT_ID
        if (str::EndsWith(el->rightTxt, GIT_COMMIT_ID_STR))
            txtLen -= str::Len(GIT_COMMIT_ID_STR) - 7;
#endif
        TextOut(hdc, el->rightPos.x, el->rightPos.y, el->rightTxt, txtLen);

        if (hasUrl) {
            int underlineY = el->rightPos.y + el->rightPos.dy - 3;
            PaintLine(hdc, RectI(el->rightPos.x, underlineY, el->rightPos.dx, 0));
            linkInfo.Append(StaticLinkInfo(el->rightPos, el->url, el->url));
        }
    }

    SelectObject(hdc, penDivideLine);
    RectI divideLine(gAboutLayoutInfo[0].rightPos.x - ABOUT_LEFT_RIGHT_SPACE_DX,
                     rect.y + titleRect.dy + 4, 0, rect.y + rect.dy - 4 - gAboutLayoutInfo[0].rightPos.y);
    PaintLine(hdc, divideLine);

    SelectObject(hdc, origFont);

    DeleteObject(penBorder);
    DeleteObject(penDivideLine);
    DeleteObject(penLinkLine);
}

static void UpdateAboutLayoutInfo(HWND hwnd, HDC hdc, RectI *rect)
{
    ScopedFont fontLeftTxt(CreateSimpleFont(hdc, LEFT_TXT_FONT, LEFT_TXT_FONT_SIZE));
    ScopedFont fontRightTxt(CreateSimpleFont(hdc, RIGHT_TXT_FONT, RIGHT_TXT_FONT_SIZE));

    HGDIOBJ origFont = SelectObject(hdc, fontLeftTxt);

    /* calculate minimal top box size */
    SizeI headerSize = CalcSumatraVersionSize(hdc);

    /* calculate left text dimensions */
    SelectObject(hdc, fontLeftTxt);
    int leftLargestDx = 0;
    int leftDy = 0;
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        SIZE txtSize;
        GetTextExtentPoint32(hdc, el->leftTxt, (int)str::Len(el->leftTxt), &txtSize);
        el->leftPos.dx = txtSize.cx;
        el->leftPos.dy = txtSize.cy;

        if (el == &gAboutLayoutInfo[0])
            leftDy = el->leftPos.dy;
        else
            assert(leftDy == el->leftPos.dy);
        if (leftLargestDx < el->leftPos.dx)
            leftLargestDx = el->leftPos.dx;
    }

    /* calculate right text dimensions */
    SelectObject(hdc, fontRightTxt);
    int rightLargestDx = 0;
    int rightDy = 0;
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        SIZE txtSize;
        int txtLen = (int)str::Len(el->rightTxt);
#ifdef GIT_COMMIT_ID
        if (str::EndsWith(el->rightTxt, GIT_COMMIT_ID_STR))
            txtLen -= str::Len(GIT_COMMIT_ID_STR) - 7;
#endif
        GetTextExtentPoint32(hdc, el->rightTxt, txtLen, &txtSize);
        el->rightPos.dx = txtSize.cx;
        el->rightPos.dy = txtSize.cy;

        if (el == &gAboutLayoutInfo[0])
            rightDy = el->rightPos.dy;
        else
            assert(rightDy == el->rightPos.dy);
        if (rightLargestDx < el->rightPos.dx)
            rightLargestDx = el->rightPos.dx;
    }

    /* calculate total dimension and position */
    RectI minRect;
    minRect.dx = ABOUT_LEFT_RIGHT_SPACE_DX + leftLargestDx + ABOUT_LINE_SEP_SIZE + rightLargestDx + ABOUT_LEFT_RIGHT_SPACE_DX;
    if (minRect.dx < headerSize.dx)
        minRect.dx = headerSize.dx;
    minRect.dx += 2 * ABOUT_LINE_OUTER_SIZE + 2 * ABOUT_MARGIN_DX;

    minRect.dy = headerSize.dy;
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++)
        minRect.dy += rightDy + ABOUT_TXT_DY;
    minRect.dy += 2 * ABOUT_LINE_OUTER_SIZE + 4;

    ClientRect rc(hwnd);
    minRect.x = (rc.dx - minRect.dx) / 2;
    minRect.y = (rc.dy - minRect.dy) / 2;

    if (rect)
        *rect = minRect;

    /* calculate text positions */
    int linePosX = ABOUT_LINE_OUTER_SIZE + ABOUT_MARGIN_DX + leftLargestDx + ABOUT_LEFT_RIGHT_SPACE_DX;
    int currY = minRect.y + headerSize.dy + 4;
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        el->leftPos.x = minRect.x + linePosX - ABOUT_LEFT_RIGHT_SPACE_DX - el->leftPos.dx;
        el->leftPos.y = currY + (rightDy - leftDy) / 2;
        el->rightPos.x = minRect.x + linePosX + ABOUT_LEFT_RIGHT_SPACE_DX;
        el->rightPos.y = currY;
        currY += rightDy + ABOUT_TXT_DY;
    }

    SelectObject(hdc, origFont);
}

static void OnPaintAbout(HWND hwnd)
{
    PAINTSTRUCT ps;
    RectI rc;
    HDC hdc = BeginPaint(hwnd, &ps);
    SetLayout(hdc, LAYOUT_LTR);
    UpdateAboutLayoutInfo(hwnd, hdc, &rc);
    DrawAbout(hwnd, hdc, rc, gLinkInfo);
    EndPaint(hwnd, &ps);
}

static void CopyAboutInfoToClipboard(HWND hwnd)
{
    str::Str<WCHAR> info(512);
    info.AppendFmt(L"%s %s\r\n", APP_NAME_STR, VERSION_TXT);
    for (size_t i = info.Size() - 2; i > 0; i--) {
        info.Append('-');
    }
    info.Append(L"\r\n");
    // concatenate all the information into a single string
    // (cf. CopyPropertiesToClipboard in SumatraProperties.cpp)
    size_t maxLen = 0;
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        maxLen = std::max(maxLen, str::Len(el->leftTxt));
    }
    for (AboutLayoutInfoEl *el = gAboutLayoutInfo; el->leftTxt; el++) {
        for (size_t i = maxLen - str::Len(el->leftTxt); i > 0; i--)
            info.Append(' ');
        info.AppendFmt(L"%s: %s\r\n", el->leftTxt, el->url ? el->url : el->rightTxt);
    }
    CopyTextToClipboard(info.LendData());
}

const WCHAR *GetStaticLink(Vec<StaticLinkInfo>& linkInfo, int x, int y, StaticLinkInfo *info)
{
    if (!HasPermission(Perm_DiskAccess))
        return nullptr;

    PointI pt(x, y);
    for (size_t i = 0; i < linkInfo.Count(); i++) {
        if (linkInfo.At(i).rect.Contains(pt)) {
            if (info)
                *info = linkInfo.At(i);
            return linkInfo.At(i).target;
        }
    }

    return nullptr;
}

static void CreateInfotipForLink(StaticLinkInfo& linkInfo)
{
    if (gHwndAboutTooltip)
        return;

    gHwndAboutTooltip = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        gHwndAbout, nullptr, GetModuleHandle(nullptr), nullptr);

    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = gHwndAbout;
    ti.uFlags = TTF_SUBCLASS;
    ti.lpszText = (WCHAR *)linkInfo.infotip;
    ti.rect = linkInfo.rect.ToRECT();

    SendMessage(gHwndAboutTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

static void ClearInfotip()
{
    if (!gHwndAboutTooltip)
        return;

    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = gHwndAbout;

    SendMessage(gHwndAboutTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);
    DestroyWindow(gHwndAboutTooltip);
    gHwndAboutTooltip = nullptr;
}

LRESULT CALLBACK WndProcAbout(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const WCHAR * url;
    PointI pt;

    switch (message)
    {
        case WM_CREATE:
            assert(!gHwndAbout);
            break;

        case WM_ERASEBKGND:
            // do nothing, helps to avoid flicker
            return TRUE;

        case WM_PAINT:
            OnPaintAbout(hwnd);
            break;

        case WM_SETCURSOR:
            if (GetCursorPosInHwnd(hwnd, pt)) {
                StaticLinkInfo linkInfo;
                if (GetStaticLink(gLinkInfo, pt.x, pt.y, &linkInfo)) {
                    CreateInfotipForLink(linkInfo);
                    SetCursor(IDC_HAND);
                    return TRUE;
                }
            }
            ClearInfotip();
            return DefWindowProc(hwnd, message, wParam, lParam);

        case WM_LBUTTONDOWN:
            gClickedURL = GetStaticLink(gLinkInfo, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_LBUTTONUP:
            url = GetStaticLink(gLinkInfo, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (url && url == gClickedURL)
                LaunchBrowser(url);
            break;

        case WM_CHAR:
            if (VK_ESCAPE == wParam)
                DestroyWindow(hwnd);
            break;

        case WM_COMMAND:
            if (IDM_COPY_SELECTION == LOWORD(wParam))
                CopyAboutInfoToClipboard(hwnd);
            break;

        case WM_DESTROY:
            ClearInfotip();
            assert(gHwndAbout);
            gHwndAbout = nullptr;
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void OnMenuAbout()
{
    if (gHwndAbout) {
        SetActiveWindow(gHwndAbout);
        return;
    }

    if (!gAtomAbout) {
        WNDCLASSEX  wcex;
        FillWndClassEx(wcex, ABOUT_CLASS_NAME, WndProcAbout);
        wcex.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SUMATRAPDF));
        gAtomAbout = RegisterClassEx(&wcex);
        CrashIf(!gAtomAbout);
    }

    gHwndAbout = CreateWindow(
            ABOUT_CLASS_NAME, ABOUT_WIN_TITLE,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            nullptr, nullptr,
            GetModuleHandle(nullptr), nullptr);
    if (!gHwndAbout)
        return;

    ToggleWindowStyle(gHwndAbout, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, IsUIRightToLeft(), GWL_EXSTYLE);

    // get the dimensions required for the about box's content
    RectI rc;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(gHwndAbout, &ps);
    SetLayout(hdc, LAYOUT_LTR);
    UpdateAboutLayoutInfo(gHwndAbout, hdc, &rc);
    EndPaint(gHwndAbout, &ps);
    rc.Inflate(ABOUT_RECT_PADDING, ABOUT_RECT_PADDING);

    // resize the new window to just match these dimensions
    WindowRect wRc(gHwndAbout);
    ClientRect cRc(gHwndAbout);
    wRc.dx += rc.dx - cRc.dx;
    wRc.dy += rc.dy - cRc.dy;
    MoveWindow(gHwndAbout, wRc.x, wRc.y, wRc.dx, wRc.dy, FALSE);

    ShowWindow(gHwndAbout, SW_SHOW);
}

void DrawAboutPage(WindowInfo& win, HDC hdc)
{
    ClientRect rc(win.hwndCanvas);
    UpdateAboutLayoutInfo(win.hwndCanvas, hdc, &rc);
    DrawAbout(win.hwndCanvas, hdc, rc, win.staticLinks);
    if (HasPermission(Perm_SavePreferences | Perm_DiskAccess) && gGlobalPrefs->rememberOpenedFiles) {
        RectI rect = DrawBottomRightLink(win.hwndCanvas, hdc, _TR("Show frequently read"));
        win.staticLinks.Append(StaticLinkInfo(rect, SLINK_LIST_SHOW));
    }
}

/* alternate static page to display when no document is loaded */

#define DOCLIST_SEPARATOR_DY        2
#define DOCLIST_THUMBNAIL_BORDER_W  1
#define DOCLIST_MARGIN_LEFT         DpiScaleX(win.hwndFrame, 40)
#define DOCLIST_MARGIN_BETWEEN_X    DpiScaleX(win.hwndFrame, 30)
#define DOCLIST_MARGIN_RIGHT        DpiScaleX(win.hwndFrame, 40)
#define DOCLIST_MARGIN_TOP          DpiScaleY(win.hwndFrame, 60)
#define DOCLIST_MARGIN_BETWEEN_Y    DpiScaleY(win.hwndFrame, 50)
#define DOCLIST_MARGIN_BOTTOM       DpiScaleY(win.hwndFrame, 40)
#define DOCLIST_MAX_THUMBNAILS_X    5
#define DOCLIST_BOTTOM_BOX_DY       DpiScaleY(win.hwndFrame, 50)

void DrawStartPage(WindowInfo& win, HDC hdc, FileHistory& fileHistory, COLORREF textColor, COLORREF backgroundColor)
{
    HPEN penBorder = CreatePen(PS_SOLID, DOCLIST_SEPARATOR_DY, WIN_COL_BLACK);
    HPEN penThumbBorder = CreatePen(PS_SOLID, DOCLIST_THUMBNAIL_BORDER_W, WIN_COL_BLACK);
    HPEN penLinkLine = CreatePen(PS_SOLID, 1, COL_BLUE_LINK);

    ScopedFont fontSumatraTxt(CreateSimpleFont(hdc, L"MS Shell Dlg", 24));
    ScopedFont fontLeftTxt(CreateSimpleFont(hdc, L"MS Shell Dlg", 14));

    HGDIOBJ origFont = SelectObject(hdc, fontSumatraTxt); /* Just to remember the orig font */

    ClientRect rc(win.hwndCanvas);
    RECT rTmp = rc.ToRECT();
    ScopedGdiObj<HBRUSH> brushLogoBg(CreateSolidBrush(GetLogoBgColor()));
    FillRect(hdc, &rTmp, brushLogoBg);

    SelectObject(hdc, brushLogoBg);
    SelectObject(hdc, penBorder);

    bool isRtl = IsUIRightToLeft();

    /* render title */
    RectI titleBox = RectI(PointI(0, 0), CalcSumatraVersionSize(hdc));
    titleBox.x = rc.dx - titleBox.dx - 3;
    DrawSumatraVersion(hdc, titleBox);
    PaintLine(hdc, RectI(0, titleBox.dy, rc.dx, 0));

    /* render recent files list */
    SelectObject(hdc, penThumbBorder);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, WIN_COL_BLACK);

    rc.y += titleBox.dy;
    rc.dy -= titleBox.dy;
    rTmp = rc.ToRECT();
    ScopedGdiObj<HBRUSH> brushAboutBg(CreateSolidBrush(GetAboutBgColor()));
    FillRect(hdc, &rTmp, brushAboutBg);
    rc.dy -= DOCLIST_BOTTOM_BOX_DY;

    Vec<DisplayState *> list;
    fileHistory.GetFrequencyOrder(list);

    int width = limitValue((rc.dx - DOCLIST_MARGIN_LEFT - DOCLIST_MARGIN_RIGHT + DOCLIST_MARGIN_BETWEEN_X) / (THUMBNAIL_DX + DOCLIST_MARGIN_BETWEEN_X), 1, DOCLIST_MAX_THUMBNAILS_X);
    int height = std::min((rc.dy - DOCLIST_MARGIN_TOP - DOCLIST_MARGIN_BOTTOM + DOCLIST_MARGIN_BETWEEN_Y) / (THUMBNAIL_DY + DOCLIST_MARGIN_BETWEEN_Y), FILE_HISTORY_MAX_FREQUENT / width);
    PointI offset(rc.x + DOCLIST_MARGIN_LEFT + (rc.dx - width * THUMBNAIL_DX - (width - 1) * DOCLIST_MARGIN_BETWEEN_X - DOCLIST_MARGIN_LEFT - DOCLIST_MARGIN_RIGHT) / 2, rc.y + DOCLIST_MARGIN_TOP);
    if (offset.x < ABOUT_INNER_PADDING)
        offset.x = ABOUT_INNER_PADDING;
    else if (list.Count() == 0)
        offset.x = DOCLIST_MARGIN_LEFT;

    SelectObject(hdc, fontSumatraTxt);
    SIZE txtSize;
    const WCHAR *txt = _TR("Frequently Read");
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    RectI headerRect(offset.x, rc.y + (DOCLIST_MARGIN_TOP - txtSize.cy) / 2, txtSize.cx, txtSize.cy);
    if (isRtl)
        headerRect.x = rc.dx - offset.x - headerRect.dx;
    rTmp = headerRect.ToRECT();
    DrawText(hdc, txt, -1, &rTmp, (isRtl ? DT_RTLREADING : DT_LEFT) | DT_NOPREFIX);

    SelectObject(hdc, fontLeftTxt);
    SelectObject(hdc, GetStockBrush(NULL_BRUSH));

    win.staticLinks.Reset();
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            if (h * width + w >= (int)list.Count()) {
                // display the "Open a document" link right below the last row
                height = w > 0 ? h + 1 : h;
                break;
            }
            DisplayState *state = list.At(h * width + w);

            RectI page(offset.x + w * (THUMBNAIL_DX + DOCLIST_MARGIN_BETWEEN_X),
                       offset.y + h * (THUMBNAIL_DY + DOCLIST_MARGIN_BETWEEN_Y),
                       THUMBNAIL_DX, THUMBNAIL_DY);
            if (isRtl)
                page.x = rc.dx - page.x - page.dx;
            bool loadOk = true;
            if (!state->thumbnail)
                loadOk = LoadThumbnail(*state);
            if (loadOk && state->thumbnail) {
                SizeI thumbSize = state->thumbnail->Size();
                if (thumbSize.dx != THUMBNAIL_DX || thumbSize.dy != THUMBNAIL_DY) {
                    page.dy = thumbSize.dy * THUMBNAIL_DX / thumbSize.dx;
                    page.y += THUMBNAIL_DY - page.dy;
                }
                HRGN clip = CreateRoundRectRgn(page.x, page.y, page.x + page.dx, page.y + page.dy, 10, 10);
                SelectClipRgn(hdc, clip);
                RenderedBitmap *clone = state->thumbnail->Clone();
                if (clone) {
                    UpdateBitmapColors(clone->GetBitmap(), textColor, backgroundColor);
                    clone->StretchDIBits(hdc, page);
                    delete clone;
                }
                else {
                    state->thumbnail->StretchDIBits(hdc, page);
                }
                SelectClipRgn(hdc, nullptr);
                DeleteObject(clip);
            }
            RoundRect(hdc, page.x, page.y, page.x + page.dx, page.y + page.dy, 10, 10);

            int iconSpace = DpiScaleX(win.hwndFrame, 20);
            RectI rect(page.x + iconSpace, page.y + page.dy + 3, page.dx - iconSpace, iconSpace);
            if (isRtl)
                rect.x -= iconSpace;
            rTmp = rect.ToRECT();
            DrawText(hdc, path::GetBaseName(state->filePath), -1, &rTmp, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | (isRtl ? DT_RIGHT : DT_LEFT));

            SHFILEINFO sfi = { 0 };
            HIMAGELIST himl = (HIMAGELIST)SHGetFileInfo(state->filePath, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            ImageList_Draw(himl, sfi.iIcon, hdc,
                           isRtl ? page.x + page.dx - DpiScaleX(win.hwndFrame, 16) : page.x,
                           rect.y, ILD_TRANSPARENT);

            win.staticLinks.Append(StaticLinkInfo(rect.Union(page), state->filePath, state->filePath));
        }
    }

    /* render bottom links */
    rc.y += DOCLIST_MARGIN_TOP + height * THUMBNAIL_DY + (height - 1) * DOCLIST_MARGIN_BETWEEN_Y + DOCLIST_MARGIN_BOTTOM;
    rc.dy = DOCLIST_BOTTOM_BOX_DY;

    SetTextColor(hdc, COL_BLUE_LINK);
    SelectObject(hdc, penLinkLine);

    HIMAGELIST himl = (HIMAGELIST)SendMessage(win.hwndToolbar, TB_GETIMAGELIST, 0, 0);
    RectI rectIcon(offset.x, rc.y, 0, 0);
    ImageList_GetIconSize(himl, &rectIcon.dx, &rectIcon.dy);
    rectIcon.y += (rc.dy - rectIcon.dy) / 2;
    if (isRtl)
        rectIcon.x = rc.dx - offset.x - rectIcon.dx;
    ImageList_Draw(himl, 0 /* index of Open icon */, hdc, rectIcon.x, rectIcon.y, ILD_NORMAL);

    txt = _TR("Open a document...");
    GetTextExtentPoint32(hdc, txt, (int)str::Len(txt), &txtSize);
    RectI rect(offset.x + rectIcon.dx + 3, rc.y + (rc.dy - txtSize.cy) / 2, txtSize.cx, txtSize.cy);
    if (isRtl)
        rect.x = rectIcon.x - rect.dx - 3;
    rTmp = rect.ToRECT();
    DrawText(hdc, txt, -1, &rTmp, isRtl ? DT_RTLREADING : DT_LEFT);
    PaintLine(hdc, RectI(rect.x, rect.y + rect.dy, rect.dx, 0));
    // make the click target larger
    rect = rect.Union(rectIcon);
    rect.Inflate(10, 10);
    win.staticLinks.Append(StaticLinkInfo(rect, SLINK_OPEN_FILE));

    rect = DrawBottomRightLink(win.hwndCanvas, hdc, _TR("Hide frequently read"));
    win.staticLinks.Append(StaticLinkInfo(rect, SLINK_LIST_HIDE));

    SelectObject(hdc, origFont);

    DeleteObject(penBorder);
    DeleteObject(penThumbBorder);
    DeleteObject(penLinkLine);
}
