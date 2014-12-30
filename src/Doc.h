/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// Doc is to EbookController what BaseEngine is to DisplayModel:
// It simply abstracts all document objects, allows querying the type, casting
// to the wrapped object and present as much of the unified interface as
// possible.
// It's small enough to be passed by value.

class EpubDoc;
class Fb2Doc;
class MobiDoc;
class PalmDoc;

struct ImageData;
class EbookTocVisitor;
class HtmlFormatter;
class HtmlFormatterArgs;

enum DocType { Doc_None, Doc_Epub, Doc_Fb2, Doc_Mobi, Doc_Pdb };
enum DocError { Error_None, Error_Unknown };

class Doc
{
protected:
    DocType type;

    // If there was an error loading a file in CreateFromFile(),
    // this is an error message to be shown to the user. Most
    // of the time it's a generic error message but can be
    // more specific e.g. mobi loading might specify that
    // loading failed due to DRM
    DocError error;

    // A copy of the file path which is needed in case of an error (else
    // the file path is supposed to be stored inside the wrapped *Doc)
    ScopedMem<WCHAR> filePath;

    union {
        void *      generic;
        EpubDoc *   epubDoc;
        Fb2Doc *    fb2Doc;
        MobiDoc *   mobiDoc;
        PalmDoc *   palmDoc;
    };

    const WCHAR *GetFilePathFromDoc() const;

public:
    Doc(const Doc& other);
    Doc& operator=(const Doc& other);
    ~Doc();

    void Clear();
    Doc() { Clear(); }
    explicit Doc(EpubDoc *doc);
    explicit Doc(Fb2Doc *doc);
    explicit Doc(MobiDoc *doc);
    explicit Doc(PalmDoc *doc);

    void Delete();

    // note: find a better name, if possible
    bool IsNone() const { return Doc_None == type; }
    bool IsDocLoaded() const { return !IsNone(); }
    DocType Type() const { return type; }

    bool LoadingFailed() const {
        CrashIf(error && !IsNone());
        return error != Error_None;
    }

    // instead of adding these to Doc, they could also be part
    // of a virtual EbookDoc interface that *Doc implement
    const WCHAR *GetFilePath() const;
    const WCHAR *GetDefaultFileExt() const;
    WCHAR *GetProperty(DocumentProperty prop) const;
    const char *GetHtmlData(size_t &len) const;
    size_t GetHtmlDataSize() const;
    ImageData *GetCoverImage() const;
    bool HasToc() const;
    bool ParseToc(EbookTocVisitor *visitor) const;
    HtmlFormatter *CreateFormatter(HtmlFormatterArgs *args) const;

    static Doc CreateFromFile(const WCHAR *filePath);
    static bool IsSupportedFile(const WCHAR *filePath, bool sniff=false);
};
