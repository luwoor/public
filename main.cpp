#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using json = nlohmann::json;

// 全局窗口控件句柄
HWND hMainWnd;
HWND hLeftPanel, hMidPanel, hRightPanel;
HWND hEditNovelName, hEditWorld, hEditChar, hEditOutline, hEditForeshadow, hEditInspire;
HWND hListChapter, hEditChapTitle, hEditChapContent;
HWND hBtnSaveChap, hBtnSaveProj, hBtnExportTxt, hBtnReset, hBtnTheme;
HWND hLabelCurrWord, hLabelTotalWord;

// 章节结构体
struct Chapter {
    std::wstring title;
    std::wstring content;
};
std::vector<Chapter> g_Chapters;
int g_CurrChapIndex = -1;
bool g_IsDarkMode = false;

// 窗口尺寸常量
const int WIDTH_LEFT = 300;
const int WIDTH_MID = 200;
const int PADDING = 8;

// 工具函数：宽字符串转多字节
std::string W2A(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 工具函数：多字节转宽字符串
std::wstring A2W(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// 字数统计
int CountWord(const std::wstring& text) {
    int count = 0;
    for (wchar_t c : text) {
        if (c != L' ' && c != L'\n' && c != L'\r') count++;
    }
    return count;
}

// 更新字数显示
void UpdateWordCount() {
    wchar_t currBuf[64] = {0};
    std::wstring currText;
    GetWindowText(hEditChapContent, currBuf, 64000);
    currText = currBuf;
    int currCount = CountWord(currText);
    swprintf(currBuf, L"当前章节：%d 字", currCount);
    SetWindowText(hLabelCurrWord, currBuf);

    int totalCount = 0;
    for (auto& chap : g_Chapters) totalCount += CountWord(chap.content);
    wchar_t totalBuf[64] = {0};
    swprintf(totalBuf, L"全书总字数：%d 字", totalCount);
    SetWindowText(hLabelTotalWord, totalBuf);
}

// 刷新章节列表
void RefreshChapterList() {
    SendMessage(hListChapter, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_Chapters.size(); i++) {
        wchar_t item[256] = {0};
        swprintf(item, L"第%d章 %s", i + 1, g_Chapters[i].title.c_str());
        SendMessage(hListChapter, LB_ADDSTRING, 0, (LPARAM)item);
    }
    UpdateWordCount();
}

// 加载选中章节
void LoadChapter(int index) {
    g_CurrChapIndex = index;
    Chapter& chap = g_Chapters[index];
    SetWindowText(hEditChapTitle, chap.title.c_str());
    SetWindowText(hEditChapContent, chap.content.c_str());
}

// 保存当前章节
void SaveChapter() {
    wchar_t titleBuf[256] = {0}, contentBuf[64000] = {0};
    GetWindowText(hEditChapTitle, titleBuf, 256);
    GetWindowText(hEditChapContent, contentBuf, 64000);
    std::wstring title = titleBuf, content = contentBuf;

    if (title.empty()) {
        MessageBox(hMainWnd, L"请输入章节标题！", L"提示", MB_ICONWARNING);
        return;
    }

    if (g_CurrChapIndex < 0) g_Chapters.push_back({title, content});
    else g_Chapters[g_CurrChapIndex] = {title, content};

    RefreshChapterList();
    SetWindowText(hEditChapTitle, L"");
    SetWindowText(hEditChapContent, L"");
    g_CurrChapIndex = -1;
    MessageBox(hMainWnd, L"✅ 章节保存成功！", L"成功", MB_OK);
}

// 导出TXT
void ExportTXT() {
    wchar_t novelNameBuf[256] = {0};
    GetWindowText(hEditNovelName, novelNameBuf, 256);
    std::wstring novelName = novelNameBuf;
    if (novelName.empty()) novelName = L"未命名小说";

    // 打开保存对话框
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"导出小说TXT";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"txt";
    if (!GetSaveFileName(&ofn)) return;

    // 拼接文本
    std::wstring txt = L"《" + novelName + L"》\r\n\r\n";
    for (int i = 0; i < g_Chapters.size(); i++) {
        auto& chap = g_Chapters[i];
        txt += L"第" + std::to_wstring(i + 1) + L"章 " + chap.title + L"\r\n\r\n";
        txt += chap.content + L"\r\n\r\n\r\n";
    }

    // 写入文件
    std::ofstream out(W2A(filePath), std::ios::out | std::ios::binary);
    std::string utf8Txt = W2A(txt);
    out << utf8Txt;
    out.close();
    MessageBox(hMainWnd, L"✅ TXT导出成功！", L"成功", MB_OK);
}

// 保存项目JSON
void SaveProject() {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"项目文件(*.json)\0*.json\0所有文件(*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"保存小说项目";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"json";
    if (!GetSaveFileName(&ofn)) return;

    // 读取所有内容
    wchar_t buf[64000] = {0};
    GetWindowText(hEditNovelName, buf, 256); std::wstring novelName = buf;
    GetWindowText(hEditWorld, buf, 64000); std::wstring world = buf;
    GetWindowText(hEditChar, buf, 64000); std::wstring charText = buf;
    GetWindowText(hEditOutline, buf, 64000); std::wstring outline = buf;
    GetWindowText(hEditForeshadow, buf, 64000); std::wstring foreshadow = buf;
    GetWindowText(hEditInspire, buf, 64000); std::wstring inspire = buf;

    // 构造JSON
    json j;
    j["novelName"] = W2A(novelName);
    j["worldview"] = W2A(world);
    j["characters"] = W2A(charText);
    j["outline"] = W2A(outline);
    j["foreshadow"] = W2A(foreshadow);
    j["inspire"] = W2A(inspire);

    json jChaps = json::array();
    for (auto& chap : g_Chapters) {
        json jChap;
        jChap["title"] = W2A(chap.title);
        jChap["content"] = W2A(chap.content);
        jChaps.push_back(jChap);
    }
    j["chapters"] = jChaps;

    // 写入文件
    std::ofstream out(W2A(filePath));
    out << j.dump(4);
    out.close();
    MessageBox(hMainWnd, L"✅ 项目保存成功！", L"成功", MB_OK);
}

// 重置所有内容
void ResetAll() {
    if (MessageBox(hMainWnd, L"⚠️ 确定清空所有内容？", L"确认", MB_YESNO | MB_ICONWARNING) != IDYES) return;
    SetWindowText(hEditNovelName, L"");
    SetWindowText(hEditWorld, L"");
    SetWindowText(hEditChar, L"");
    SetWindowText(hEditOutline, L"");
    SetWindowText(hEditForeshadow, L"");
    SetWindowText(hEditInspire, L"");
    g_Chapters.clear();
    RefreshChapterList();
    SetWindowText(hEditChapTitle, L"");
    SetWindowText(hEditChapContent, L"");
}

// 切换明暗主题
void ToggleTheme() {
    g_IsDarkMode = !g_IsDarkMode;
    InvalidateRect(hMainWnd, NULL, TRUE);
}

// 窗口回调函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hMainWnd = hWnd;
        int winW = 1200, winH = 700;
        // 主窗口居中
        RECT screen; GetWindowRect(GetDesktopWindow(), &screen);
        SetWindowPos(hWnd, NULL, (screen.right - winW) / 2, (screen.bottom - winH) / 2, winW, winH, 0);

        // 创建三栏面板
        hLeftPanel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, PADDING, PADDING, WIDTH_LEFT, winH - PADDING * 2, hWnd, NULL, NULL, NULL);
        hMidPanel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, WIDTH_LEFT + PADDING * 2, PADDING, WIDTH_MID, winH - PADDING * 2, hWnd, NULL, NULL, NULL);
        hRightPanel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, WIDTH_LEFT + WIDTH_MID + PADDING * 3, PADDING, winW - WIDTH_LEFT - WIDTH_MID - PADDING * 4, winH - PADDING * 2, hWnd, NULL, NULL, NULL);

        // ========== 左侧控件 ==========
        int y = PADDING;
        hEditNovelName = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, PADDING, y, WIDTH_LEFT - PADDING * 2, 24, hLeftPanel, NULL, NULL, NULL); y += 32;
        hEditWorld = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, PADDING, y, WIDTH_LEFT - PADDING * 2, 100, hLeftPanel, NULL, NULL, NULL); y += 108;
        hEditChar = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, PADDING, y, WIDTH_LEFT - PADDING * 2, 100, hLeftPanel, NULL, NULL, NULL); y += 108;
        hEditOutline = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, PADDING, y, WIDTH_LEFT - PADDING * 2, 100, hLeftPanel, NULL, NULL, NULL); y += 108;
        hEditForeshadow = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, PADDING, y, WIDTH_LEFT - PADDING * 2, 100, hLeftPanel, NULL, NULL, NULL); y += 108;
        hEditInspire = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, PADDING, y, WIDTH_LEFT - PADDING * 2, 100, hLeftPanel, NULL, NULL, NULL);

        // ========== 中间控件 ==========
        CreateWindowW(L"STATIC", L"📖 章节列表", WS_CHILD | WS_VISIBLE, PADDING, PADDING, WIDTH_MID - PADDING * 2, 20, hMidPanel, NULL, NULL, NULL);
        hListChapter = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY, PADDING, 28, WIDTH_MID - PADDING * 2, winH - 80, hMidPanel, NULL, NULL, NULL);

        // ========== 右侧控件 ==========
        int rY = PADDING;
        // 按钮行
        hBtnSaveChap = CreateWindowW(L"BUTTON", L"💾 保存章节", WS_CHILD | WS_VISIBLE, PADDING, rY, 110, 28, hRightPanel, (HMENU)101, NULL, NULL);
        hBtnSaveProj = CreateWindowW(L"BUTTON", L"📦 保存项目", WS_CHILD | WS_VISIBLE, 120, rY, 110, 28, hRightPanel, (HMENU)102, NULL, NULL);
        hBtnExportTxt = CreateWindowW(L"BUTTON", L"📄 导出TXT", WS_CHILD | WS_VISIBLE, 230, rY, 110, 28, hRightPanel, (HMENU)103, NULL, NULL);
        hBtnReset = CreateWindowW(L"BUTTON", L"🔄 一键初始化", WS_CHILD | WS_VISIBLE, 340, rY, 110, 28, hRightPanel, (HMENU)104, NULL, NULL);
        hBtnTheme = CreateWindowW(L"BUTTON", L"🌓 明暗切换", WS_CHILD | WS_VISIBLE, 450, rY, 110, 28, hRightPanel, (HMENU)105, NULL, NULL);
        rY += 36;

        hEditChapTitle = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, PADDING, rY, winW - WIDTH_LEFT - WIDTH_MID - PADDING * 6, 24, hRightPanel, NULL, NULL, NULL);
        rY += 32;
        hEditChapContent = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_WANTRETURN, PADDING, rY, winW - WIDTH_LEFT - WIDTH_MID - PADDING * 6, winH - 180, hRightPanel, NULL, NULL, NULL);
        rY += winH - 212;

        hLabelCurrWord = CreateWindowW(L"STATIC", L"当前章节：0 字", WS_CHILD | WS_VISIBLE, PADDING, rY, 200, 20, hRightPanel, NULL, NULL, NULL);
        hLabelTotalWord = CreateWindowW(L"STATIC", L"全书总字数：0 字", WS_CHILD | WS_VISIBLE, 220, rY, 200, 20, hRightPanel, NULL, NULL, NULL);
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
        if (HIWORD(wParam) == LBN_SELCHANGE && (HWND)lParam == hListChapter) {
            int sel = SendMessage(hListChapter, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) LoadChapter(sel);
        }
        // 正文编辑更新字数
        if ((HWND)lParam == hEditChapContent && HIWORD(wParam) == EN_CHANGE) UpdateWordCount();
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // 明暗主题背景
        if (g_IsDarkMode) FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(26, 29, 36)));
        else FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(245, 247, 250)));
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

// 主函数
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine;

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"NovelWriterWin32";
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // 创建窗口
    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"小说创作器（纯C++原生版）",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 700,
        NULL, NULL, hInstance, NULL
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
