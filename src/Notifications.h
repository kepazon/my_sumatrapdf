/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

class NotificationWnd;

class NotificationWndCallback {
public:
    // called after a message has timed out or has been canceled
    virtual void RemoveNotification(NotificationWnd *wnd) = 0;
    virtual ~NotificationWndCallback() { }
};

class NotificationWnd : public ProgressUpdateUI {
public:
    static const int TIMEOUT_TIMER_ID = 1;

    HWND self;
    bool hasProgress;
    bool hasCancel;

    HFONT font;
    bool  highlight;
    NotificationWndCallback *notificationCb;

    // only used for progress notifications
    bool isCanceled;
    int  progress;
    int  progressWidth;
    WCHAR *progressMsg; // must contain two %d (for current and total)

    void CreatePopup(HWND parent, const WCHAR *message);
    void UpdateWindowPosition(const WCHAR *message, bool init=false);

    static const int TL_MARGIN = 8;
    int groupId; // for use by Notifications

    // to reduce flicker, we might ask the window to shrink the size less often
    // (notifcation windows are only shrunken if by less than factor shrinkLimit)
    float shrinkLimit;

    // Note: in most cases use WindowInfo::ShowNotification()
    NotificationWnd(HWND parent, const WCHAR *message, int timeoutInMS=0, bool highlight=false, NotificationWndCallback *cb=nullptr) :
        hasProgress(false), hasCancel(!timeoutInMS), notificationCb(cb), highlight(highlight), progressMsg(nullptr), shrinkLimit(1.0f) {
        CreatePopup(parent, message);
        if (timeoutInMS)
            SetTimer(self, TIMEOUT_TIMER_ID, timeoutInMS, nullptr);
    }

    NotificationWnd(HWND parent, const WCHAR *message, const WCHAR *progressMsg, NotificationWndCallback *cb=nullptr) :
        hasProgress(true), hasCancel(true), notificationCb(cb), highlight(false), isCanceled(false), progress(0), shrinkLimit(1.0f) {
        this->progressMsg = str::Dup(progressMsg);
        CreatePopup(parent, message);
    }

    ~NotificationWnd() {
        DestroyWindow(self);
        DeleteObject(font);
        free(progressMsg);
    }

    HWND hwnd() { return self; }

    void UpdateMessage(const WCHAR *message, int timeoutInMS=0, bool highlight=false);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // ProgressUpdateUI methods
    virtual void UpdateProgress(int current, int total);
    virtual bool WasCanceled();
};

class Notifications : public NotificationWndCallback {
    Vec<NotificationWnd *> wnds;

    int  GetWndX(NotificationWnd *wnd);
    void MoveBelow(NotificationWnd *fix, NotificationWnd *move);
    void Remove(NotificationWnd *wnd);

public:
    ~Notifications() { DeleteVecMembers(wnds); }

    bool Contains(NotificationWnd *wnd) { return wnds.Contains(wnd); }

    // groupId is used to classify notifications and causes a notification
    // to replace any other notification of the same group
    void         Add(NotificationWnd *wnd, int groupId=0);
    NotificationWnd * GetForGroup(int groupId);
    void         RemoveForGroup(int groupId);
    void         Relayout();

    // NotificationWndCallback methods
    virtual void RemoveNotification(NotificationWnd *wnd);
};

void RegisterNotificationsWndClass();
