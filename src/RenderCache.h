/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#define RENDER_DELAY_UNDEFINED ((UINT)-1)
#define RENDER_DELAY_FAILED    ((UINT)-2)
#define INVALID_TILE_RES       ((USHORT)-1)

#define MAX_PAGE_REQUESTS 8
// keep this value reasonably low, else we'll run out of
// GDI resources/memory when caching many larger bitmaps
#define MAX_BITMAPS_CACHED 64

class RenderingCallback {
public:
    virtual void Callback(RenderedBitmap *bmp=nullptr) = 0;
    virtual ~RenderingCallback() { }
};

/* A page is split into tiles of at most TILE_MAX_W x TILE_MAX_H pixels.
   A given tile starts at (col / 2^res * page_width, row / 2^res * page_height). */
struct TilePosition {
    USHORT res, row, col;

    explicit TilePosition(USHORT res=INVALID_TILE_RES, USHORT row=-1, USHORT col=-1) :
        res(res), row(row), col(col) { }
    bool operator==(const TilePosition& other) const {
        return res == other.res && row == other.row && col == other.col;
    }
};

/* We keep a cache of rendered bitmaps. BitmapCacheEntry keeps data
   that uniquely identifies rendered page (dm, pageNo, rotation, zoom)
   and the corresponding rendered bitmap. */
struct BitmapCacheEntry {
    DisplayModel *   dm;
    int              pageNo;
    int              rotation;
    float            zoom;
    TilePosition     tile;

    // owned by the BitmapCacheEntry
    RenderedBitmap * bitmap;
    bool             outOfDate;
    int              refs;

    BitmapCacheEntry(DisplayModel *dm, int pageNo, int rotation, float zoom, TilePosition tile, RenderedBitmap *bitmap) :
        dm(dm), pageNo(pageNo), rotation(rotation), zoom(zoom), tile(tile), bitmap(bitmap), outOfDate(false), refs(1) { }
    ~BitmapCacheEntry() { delete bitmap; }
};

/* Even though this looks a lot like a BitmapCacheEntry, we keep it
   separate for clarity in the code (PageRenderRequests are reused,
   while BitmapCacheEntries are ref-counted) */
struct PageRenderRequest {
    DisplayModel *      dm;
    int                 pageNo;
    int                 rotation;
    float               zoom;
    TilePosition        tile;

    RectD               pageRect; // calculated from TilePosition
    bool                abort;
    AbortCookie *       abortCookie;
    DWORD               timestamp;
    // owned by the PageRenderRequest (use it before reusing the request)
    // on rendering success, the callback gets handed the RenderedBitmap
    RenderingCallback * renderCb;
};

class RenderCache
{
private:
    BitmapCacheEntry *  cache[MAX_BITMAPS_CACHED];
    int                 cacheCount;
    // make sure to never ask for requestAccess in a cacheAccess
    // protected critical section in order to avoid deadlocks
    CRITICAL_SECTION    cacheAccess;

    PageRenderRequest   requests[MAX_PAGE_REQUESTS];
    int                 requestCount;
    PageRenderRequest * curReq;
    CRITICAL_SECTION    requestAccess;
    HANDLE              renderThread;

    SizeI               maxTileSize;
    bool                isRemoteSession;

public:
    COLORREF            textColor;
    COLORREF            backgroundColor;

    RenderCache();
    ~RenderCache();

    void    RequestRendering(DisplayModel *dm, int pageNo);
    void    Render(DisplayModel *dm, int pageNo, int rotation, float zoom,
                   RectD pageRect, RenderingCallback& callback);
    void    CancelRendering(DisplayModel *dm);
    bool    Exists(DisplayModel *dm, int pageNo, int rotation,
                   float zoom=INVALID_ZOOM, TilePosition *tile=nullptr);
    void    FreeForDisplayModel(DisplayModel *dm) { FreePage(dm); }
    void    KeepForDisplayModel(DisplayModel *oldDm, DisplayModel *newDm);
    void    Invalidate(DisplayModel *dm, int pageNo, RectD rect);
    // returns how much time in ms has past since the most recent rendering
    // request for the visible part of the page if nothing at all could be
    // painted, 0 if something has been painted and RENDER_DELAY_FAILED on failure
    UINT    Paint(HDC hdc, RectI bounds, DisplayModel *dm, int pageNo,
                  PageInfo *pageInfo, bool *renderOutOfDateCue);

protected:
    /* Interface for page rendering thread */
    HANDLE  startRendering;

    bool    ClearCurrentRequest();
    bool    GetNextRequest(PageRenderRequest *req);
    void    Add(PageRenderRequest &req, RenderedBitmap *bitmap);

private:
    USHORT  GetTileRes(DisplayModel *dm, int pageNo);
    USHORT  GetMaxTileRes(DisplayModel *dm, int pageNo, int rotation);
    bool    ReduceTileSize();

    bool    IsRenderQueueFull() const { return requestCount == MAX_PAGE_REQUESTS; }
    UINT    GetRenderDelay(DisplayModel *dm, int pageNo, TilePosition tile);
    void    RequestRendering(DisplayModel *dm, int pageNo, TilePosition tile, bool clearQueueForPage=true);
    bool    Render(DisplayModel *dm, int pageNo, int rotation, float zoom,
                   TilePosition *tile=nullptr, RectD *pageRect=nullptr,
                   RenderingCallback *callback=nullptr);
    void    ClearQueueForDisplayModel(DisplayModel *dm, int pageNo=INVALID_PAGE_NO,
                                      TilePosition *tile=nullptr);
    void    AbortCurrentRequest();

    static DWORD WINAPI RenderCacheThread(LPVOID data);

    BitmapCacheEntry *  Find(DisplayModel *dm, int pageNo, int rotation,
                             float zoom=INVALID_ZOOM, TilePosition *tile=nullptr);
    void    DropCacheEntry(BitmapCacheEntry *entry);
    void    FreePage(DisplayModel *dm=nullptr, int pageNo=-1, TilePosition *tile=nullptr);
    void    FreeNotVisible() { FreePage(); }

    UINT    PaintTile(HDC hdc, RectI bounds, DisplayModel *dm, int pageNo,
                      TilePosition tile, RectI tileOnScreen, bool renderMissing,
                      bool *renderOutOfDateCue, bool *renderedReplacement);
};
