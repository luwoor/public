#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include <vector>

// 链接系统原生库
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// ===================== 全局控件句柄 =====================
HWND hMainWnd;
HWND hEditNovelName, hEditWorld, hEditChar, hEditOutline, hEditForeshadow, hEditInspire;
HWND hListChapter, hEditChapTitle, hEditChapContent;
HWND hBtnSaveChap, hBtnSaveProj, hBtnExportTxt, hBtnReset, hBtnTheme;
HWND hLabelCurrWord, hLabelTotalWord;

// ===================== 数据结构 =====================
struct Chapter {
    std::wstring title;
    std::wstring content;
};
std::vector<Chapter> g_Chapters;
int g_CurrChapIndex = -1;
bool g_IsDarkMode = false;

// ===================== 尺寸常量 =====================
const int WIDTH_LEFT = 300;
const int WIDTH_MID = 200;
const int PADDING = 8;

// ===================== 工具函数：宽/多字节互转 =====================
std::string W2A(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

std::wstring A2W(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size);
    return wstr;
}

// ===================== 字数统计 =====================
int CountWord(const std::wstring& text) {
    int cnt = 0;
    for (wchar_t c : text) {
        if (c != L' ' && c != L'\n' && c != L'\r') cnt++;
    }
    return cnt;
}

void UpdateWordCount() {
    wchar_t buf[64000] = {0};
    GetWindowTextW(hEditChapContent, buf, 64000);
    int curr = CountWord(buf);
    
    wchar_t currTxt[64];
    swprintf(currTxt, L"当前章节：%d 字", curr);
    SetWindowTextW(hLabelCurrWord, currTxt);

    int total = 0;
    for (auto& ch : g_Chapters) total += CountWord(ch.content);
    
    wchar_t totalTxt[64];
    swprintf(totalTxt, L"全书总字数：%d 字", total);
    SetWindowTextW(hLabelTotalWord, totalTxt);
}

// ===================== 章节操作 =====================
void RefreshChapterList() {
    SendMessage(hListChapter, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_Chapters.size(); i++) {
        wchar_t item[256];
        swprintf(item, L"第%d章 %s", i + 1, g_Chapters[i].title.c_str());
        SendMessage(hListChapter, LB_ADDSTRING, 0, (LPARAM)item);
    }
    UpdateWordCount();
}

void LoadChapter(int idx) {
    g_CurrChapIndex = idx;
    SetWindowTextW(hEditChapTitle, g_Chapters[idx].title.c_str());
    SetWindowTextW(hEditChapContent, g_Chapters[idx].content.c_str());
}

void SaveChapter() {
    wchar_t t[256], c[64000];
    GetWindowTextW(hEditChapTitle, t, 256);
    GetWindowTextW(hEditChapContent, c, 64000);
    std::wstring title = t, content = c;

    if (title.empty()) {
        MessageBoxW(hMainWnd, L"请输入章节标题！", L"提示", MB_ICONWARNING);
        return;
    }
    if (g_CurrChapIndex < 0) g_Chapters.push_back({title, content});
    else g_Chapters[g_CurrChapIndex] = {title, content};

    RefreshChapterList();
    SetWindowTextW(hEditChapTitle, L"");
    SetWindowTextW(hEditChapContent, L"");
    g_CurrChapIndex = -1;
    MessageBoxW(hMainWnd, L"✅ 章节保存成功", L"成功", MB_OK);
}

// ===================== 文件操作（纯原生API） =====================
void ExportTXT() {
    wchar_t novelName[256];
    GetWindowTextW(hEditNovelName, novelName, 256);
    std::wstring name = novelName;
    if (name.empty()) name = L"未命名小说";

    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {sizeof(ofn), hMainWnd, NULL, L"文本文件(*.txt)\0*.txt\0", NULL, 0, L"导出小说", OFN_OVERWRITEPROMPT, L"txt", path, MAX_PATH};
    if (!GetSaveFileNameW(&ofn)) return;

    std::wstring txt = L"《" + name + L"》\r\n\r\n";
    for (int i = 0; i < g_Chapters.size(); i++) {
        auto& ch = g_Chapters[i];
        txt += L"第" + std::to_wstring(i + 1) + L"章 " + ch.title + L"\r\n\r\n";
        txt += ch.content + L"\r\n\r\n\r\n";
    }

    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        std::string utf8 = W2A(txt);
        DWORD w;
        WriteFile(hFile, utf8.c_str(), (DWORD)utf8.size(), &w, NULL);
        CloseHandle(hFile);
        MessageBoxW(hMainWnd, L"✅ TXT导出成功", L"成功", MB_OK);
    }
}

void SaveProject() {
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {sizeof(ofn), hMainWnd, NULL, L"项目文件(*.nov)\0*.nov\0", NULL, 0, L"保存项目", OFN_OVERWRITEPROMPT, L"nov", path, MAX_PATH};
    if (!GetSaveFileNameW(&ofn)) return;

    // 读取所有内容
    wchar_t buf[64000];
    GetWindowTextW(hEditNovelName, buf, 256); std::wstring novelName = buf;
    GetWindowTextW(hEditWorld, buf, 64000); std::wstring world = buf;
    GetWindowTextW(hEditChar, buf, 64000); std::wstring charTxt = buf;
    GetWindowTextW(hEditOutline, buf, 64000); std::wstring outline = buf;
    GetWindowTextW(hEditForeshadow, buf, 64000); std::wstring foreshadow = buf;
    GetWindowTextW(hEditInspire, buf, 64000); std::wstring inspire = buf;

    // 写入（简单分隔符，零依赖）
    std::wstring data = novelName + L"\n###SPLIT###\n" + world + L"\n###SPLIT###\n" + charTxt + L"\n###SPLIT###\n" + outline + L"\n###SPLIT###\n" + foreshadow + L"\n###SPLIT###\n" + inspire;
    for (auto& ch : g_Chapters) data += L"\n###CHAP###\n" + ch.title + L"\n###CONT###\n" + ch.content;

    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        std::string utf8 = W2A(data);
        DWORD w;
        WriteFile(hFile, utf8.c_str(), (DWORD)utf8.size(), &w, NULL);
        CloseHandle(hFile);
        MessageBoxW(hMainWnd, L"✅ 项目保存成功", L"成功", MB_OK);
    }
}

void ResetAll() {
    if (MessageBoxW(hMainWnd, L"⚠️ 确定清空所有？", L"确认", MB_YESNO | MB_ICONWARNING) != IDYES) return;
    SetWindowTextW(hEditNovelName, L"");
    SetWindowTextW(hEditWorld, L"");
    SetWindowTextW(hEditChar, L"");
    SetWindowTextW(hEditOutline, L"");
    SetWindowTextW(hEditForeshadow, L"");
    SetWindowTextW(hEditInspire, L"");
    g_Chapters.clear();
    RefreshChapterList();
    SetWindowTextW(hEditChapTitle, L"");
    SetWindowTextW(hEditChapContent, L"");
}

void ToggleTheme() {
    g_IsDarkMode = !g_IsDarkMode;
    InvalidateRect(hMainWnd, NULL, TRUE);
}

// ===================== 窗口回调函数（全宽字符版本） =====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hMainWnd = hWnd;
        int W = 1200, H = 700;
        RECT scr; GetWindowRect(GetDesktopWindow(), &scr);
        SetWindowPos(hWnd, NULL, (scr.right-W)/2, (scr.bottom-H)/2, W, H, 0);

        // 左侧编辑区
        hEditNovelName = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER, PADDING, PADDING, WIDTH_LEFT-PADDING*2, 24, hWnd, NULL, NULL, NULL);
        hEditWorld = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE, PADDING, 40, WIDTH_LEFT-PADDING*2, 100, hWnd, NULL, NULL, NULL);
        hEditChar = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE, PADDING, 148, WIDTH_LEFT-PADDING*2, 100, hWnd, NULL, NULL, NULL);
        hEditOutline = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE, PADDING, 256, WIDTH_LEFT-PADDING*2, 100, hWnd, NULL, NULL, NULL);
        hEditForeshadow = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE, PADDING, 364, WIDTH_LEFT-PADDING*2, 100, hWnd, NULL, NULL, NULL);
        hEditInspire = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE, PADDING, 472, WIDTH_LEFT-PADDING*2, 100, hWnd, NULL, NULL, NULL);

        // 中间章节列表
        CreateWindowW(L"STATIC", L"📖 章节列表", WS_CHILD|WS_VISIBLE, WIDTH_LEFT+PADDING*2, PADDING, WIDTH_MID, 20, hWnd, NULL, NULL, NULL);
        hListChapter = CreateWindowW(L"LISTBOX", L"", WS_CHILD|WS_VISIBLE|LBS_NOTIFY, WIDTH_LEFT+PADDING*2, 30, WIDTH_MID, H-80, hWnd, NULL, NULL, NULL);

        // 右侧按钮+编辑区
        int rx = WIDTH_LEFT + WIDTH_MID + PADDING*3;
        hBtnSaveChap = CreateWindowW(L"BUTTON", L"💾 保存章节", WS_CHILD|WS_VISIBLE, rx, PADDING, 110, 28, hWnd, (HMENU)101, NULL, NULL);
        hBtnSaveProj = CreateWindowW(L"BUTTON", L"📦 保存项目", WS_CHILD|WS_VISIBLE, rx+115, PADDING, 110, 28, hWnd, (HMENU)102, NULL, NULL);
        hBtnExportTxt = CreateWindowW(L"BUTTON", L"📄 导出TXT", WS_CHILD|WS_VISIBLE, rx+230, PADDING, 110, 28, hWnd, (HMENU)103, NULL, NULL);
        hBtnReset = CreateWindowW(L"BUTTON", L"🔄 一键初始化", WS_CHILD|WS_VISIBLE, rx+345, PADDING, 110, 28, hWnd, (HMENU)104, NULL, NULL);
        hBtnTheme = CreateWindowW(L"BUTTON", L"🌓 明暗切换", WS_CHILD|WS_VISIBLE, rx+460, PADDING, 110, 28, hWnd, (HMENU)105, NULL, NULL);

        hEditChapTitle = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER, rx, 40, W-rx-PADDING, 24, hWnd, NULL, NULL, NULL);
        hEditChapContent = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_WANTRETURN, rx, 72, W-rx-PADDING, H-180, hWnd, NULL, NULL, NULL);
        hLabelCurrWord = CreateWindowW(L"STATIC", L"当前章节：0 字", WS_CHILD|WS_VISIBLE, rx, H-90, 200, 20, hWnd, NULL, NULL, NULL);
        hLabelTotalWord = CreateWindowW(L"STATIC", L"全书总字数：0 字", WS_CHILD|WS_VISIBLE, rx+220, H-90, 200, 20, hWnd, NULL, NULL, NULL);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 101: SaveChapter(); break;
        case 102: SaveProject(); break;
        case 103: ExportTXT(); break;
        case 104: ResetAll(); break;
        case 105: ToggleTheme(); break;
        }
        // 章节列表点击
        if (HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = SendMessage(hListChapter, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) LoadChapter(sel);
        }
        // 正文编辑更新字数
        if (HIWORD(wParam) == EN_CHANGE) UpdateWordCount();
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (g_IsDarkMode) FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(26,29,36)));
        else FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(245,247,250)));
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY: 
        PostQuitMessage(0); 
        break;
    default: 
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ===================== 程序入口（严格匹配宽字符类型） =====================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, PWSTR lpCmd, int nCmdShow) {
    (void)hPrevInst; (void)lpCmd;

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex = {sizeof(icex), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icex);

    // 注册窗口类（全宽字符）
    const wchar_t CLASS_NAME[] = L"NovelWin32";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // 创建主窗口
    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"小说创作器 · 纯C++原生", 
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 700,
        NULL, NULL, hInst, NULL
    );
    ShowWindow(hWnd, nCmdShow);

    // 消息循环
    MSG m;
    while (GetMessage(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return 0;
}
