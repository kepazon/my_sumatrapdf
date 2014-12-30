/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

enum TextRenderMethod {
    TextRenderMethodGdiplus, // uses MeasureTextAccurate, which is slower than MeasureTextQuick
    TextRenderMethodGdiplusQuick, // uses MeasureTextQuick
    TextRenderMethodGdi,
    TextRenderMethodHdc,
    //TODO: implement TextRenderDirectDraw
    //TextRenderDirectDraw
};

class ITextRender {
public:
    virtual void            SetFont(CachedFont *font) = 0;
    virtual void            SetTextColor(Gdiplus::Color col) = 0;

    // this is only for the benefit of TextRenderGdi. In GDI+, Draw() uses
    // transparent background color (i.e. whatever is under).
    // GDI doesn't support such transparency so the best we can do is simulate
    // that if the background is solid color. It won't work in other cases
    virtual void            SetTextBgColor(Gdiplus::Color col) = 0;

    virtual float           GetCurrFontLineSpacing() = 0;

    virtual Gdiplus::RectF  Measure(const char *s, size_t sLen) = 0;
    virtual Gdiplus::RectF  Measure(const WCHAR *s, size_t sLen) = 0;

    // GDI+ calls cannot be done if we called Graphics::GetHDC(). However, getting/releasing
    // hdc is very expensive and kills performance if we do it for every Draw(). So we add
    // explicit Lock()/Unlock() calls (only important for TextDrawGdi) so that a caller
    // can batch Draw() calls to minimize GetHDC()/ReleaseHDC() calls
    virtual void Lock() = 0;
    virtual void Unlock() = 0;

    virtual void Draw(const char *s, size_t sLen, RectF& bb, bool isRtl=false) = 0;
    virtual void Draw(const WCHAR *s, size_t sLen, RectF& bb, bool isRtl=false) = 0;

    virtual ~ITextRender() {};

    TextRenderMethod method;
};

class TextRenderGdi : public ITextRender {
private:
    HDC                     hdcGfxLocked;
    HDC                     hdcForTextMeasure;
    CachedFont *            currFont;
    Gdiplus::Graphics *     gfx;
    Gdiplus::Color          textColor;
    Gdiplus::Color          textBgColor;
    WCHAR                   txtConvBuf[512];

    HDC                     memHdc;
    HBITMAP                 memBmp;
    void *                  memBmpData;
    int                     memBmpDx, memBmpDy;


    TextRenderGdi() : hdcGfxLocked(nullptr), hdcForTextMeasure(nullptr),
        currFont(nullptr), gfx(nullptr), memHdc(nullptr), memBmp(nullptr), memBmpData(nullptr),
        memBmpDx(0), memBmpDy(0) { }

    void        FreeMemBmp();
    void        CreateClearBmpOfSize(int dx, int dy);

public:
    void CreateHdcForTextMeasure();
    // note: Draw() ignores any transformation set on gfx
    static TextRenderGdi *  Create(Gdiplus::Graphics *gfx);

    virtual void            SetFont(CachedFont *font);
    virtual void            SetTextColor(Gdiplus::Color col);
    virtual void            SetTextBgColor(Gdiplus::Color col);

    virtual float           GetCurrFontLineSpacing();

    virtual Gdiplus::RectF  Measure(const char *s, size_t sLen);
    virtual Gdiplus::RectF  Measure(const WCHAR *s, size_t sLen);

    virtual void            Lock();
    virtual void            Unlock();

    virtual void            Draw(const char *s, size_t sLen, RectF& bb, bool isRtl=false);
    virtual void            Draw(const WCHAR *s, size_t sLen, RectF& bb, bool isRtl=false);

    void                    DrawTransparent(const char *s, size_t sLen, RectF& bb, bool isRtl=false);
    void                    DrawTransparent(const WCHAR *s, size_t sLen, RectF& bb, bool isRtl=false);

    virtual ~TextRenderGdi();
};

class TextRenderGdiplus : public ITextRender {
private:
    Gdiplus::RectF        (*measureAlgo)(Gdiplus::Graphics *g, Gdiplus::Font *f, const WCHAR *s, int len);

    // We don't own gfx and currFont
    Gdiplus::Graphics *  gfx;
    CachedFont *         currFont;
    Gdiplus::Color       textColor;
    Gdiplus::Brush *     textColorBrush;
    WCHAR                txtConvBuf[512];

    TextRenderGdiplus() : gfx(nullptr), currFont(nullptr), textColorBrush(nullptr), textColor(0,0,0,0) {}

public:
    static TextRenderGdiplus*  Create(Gdiplus::Graphics *gfx, Gdiplus::RectF (*measureAlgo)(Gdiplus::Graphics *g, Gdiplus::Font *f, const WCHAR *s, int len)=nullptr);

    virtual void                SetFont(CachedFont *font);
    virtual void                SetTextColor(Gdiplus::Color col);
    virtual void                SetTextBgColor(Gdiplus::Color col) {}

    virtual float               GetCurrFontLineSpacing();

    virtual Gdiplus::RectF      Measure(const char *s, size_t sLen);
    virtual Gdiplus::RectF      Measure(const WCHAR *s, size_t sLen);

    virtual void Lock() {}
    virtual void Unlock() {}

    virtual void                Draw(const char *s, size_t sLen, RectF& bb, bool isRtl=false);
    virtual void                Draw(const WCHAR *s, size_t sLen, RectF& bb, bool isRtl=false);

    virtual ~TextRenderGdiplus();
};

// Note: this is not meant to be used, just exists so that I can see
// perf compared to other TextRender* implementations
class TextRenderHdc : public ITextRender {

    BITMAPINFO              bmi;

    HDC                     hdc;
    HBITMAP                 bmp;
    void *                  bmpData;

    // We don't own gfx and currFont
    Gdiplus::Graphics *     gfx;
    CachedFont *            currFont;
    Gdiplus::Color          textColor;
    Gdiplus::Color          textBgColor;
    WCHAR                   txtConvBuf[512];

    TextRenderHdc() : hdc(nullptr), bmp(nullptr), bmpData(nullptr), currFont(nullptr),
        textColor(0,0,0,0), textBgColor(0,0,0,0) {
            ZeroMemory(&bmi, sizeof(bmi));
        }

public:
    static TextRenderHdc* Create(Gdiplus::Graphics *gfx, int dx, int dy);

    virtual void                SetFont(CachedFont *font);
    virtual void                SetTextColor(Gdiplus::Color col);
    virtual void                SetTextBgColor(Gdiplus::Color col);

    virtual float               GetCurrFontLineSpacing();

    virtual Gdiplus::RectF      Measure(const char *s, size_t sLen);
    virtual Gdiplus::RectF      Measure(const WCHAR *s, size_t sLen);

    virtual void Lock();
    virtual void Unlock();

    virtual void                Draw(const char *s, size_t sLen, RectF& bb, bool isRtl=false);
    virtual void                Draw(const WCHAR *s, size_t sLen, RectF& bb, bool isRtl=false);

    virtual ~TextRenderHdc();
};

ITextRender *CreateTextRender(TextRenderMethod method, Graphics *gfx, int dx, int dy);

size_t  StringLenForWidth(ITextRender *textRender, const WCHAR *s, size_t len, float dx);
REAL    GetSpaceDx(ITextRender *textRender);
