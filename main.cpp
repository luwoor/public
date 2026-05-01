// ========== 绝杀两行，放在最顶部！强制编译器读UTF-8源码！ ==========
#pragma execution_character_set("utf-8")
#pragma source_character_set("utf-8")

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// ===================== 控件ID =====================
enum ControlID
{
    ID_BTN_SAVE_CHAP = 101,
    ID_BTN_SAVE_PROJ = 102,
    ID_BTN_EXPORT_TXT = 103,
    ID_BTN_RESET = 104,
    ID_BTN_THEME = 105,
    ID_LIST_CHAPTER = 201
};

// ===================== 全局句柄 =====================
HWND hMainWindow;
HWND hEditNovelName, hEditWorldView, hEditCharacter, hEditOutline, hEditForeshadow, hEditInspire;
HWND hListChapterBox;
HWND hEditChapterTitle, hEditChapterContent;
HWND hLabelWordCountCurr, hLabelTotalWordCount;

// ===================== 数据结构 =====================
struct Chapter
{
    std::wstring Title;
    std::wstring Content;
};
std::vector<Chapter> g_ChapterList;
int g_SelectChapterIndex = -1;
bool g_IsDarkTheme = false;

// ===================== 尺寸常量 =====================
const int LEFT_PANEL_WIDTH = 300;
const int MID_PANEL_WIDTH = 200;
const int CONTROL_PADDING = 8;

// ===================== 宽字符转UTF8 =====================
std::string WideToUtf8(const std::wstring& wStr)
{
    if (wStr.empty()) return "";
    int nBytes = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), NULL, 0, NULL, NULL);
    std::string result(nBytes, 0);
    WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), &result[0], nBytes, NULL, NULL);
    return result;
}

// ===================== 字数统计 =====================
int CountValidWord(const std::wstring& text)
{
    int count = 0;
    for (wchar_t ch : text)
    {
        if (ch != L' ' && ch != L'\n' && ch != L'\r') count++;
    }
    return count;
}

void RefreshWordCount()
{
    wchar_t contentBuffer[65535] = { 0 };
    GetWindowTextW(hEditChapterContent, contentBuffer, 65535);
    int currCount = CountValidWord(contentBuffer);
    wchar_t currText[64] = { 0 };
    swprintf(currText, L"当前章节：%d 字", currCount);
    SetWindowTextW(hLabelWordCountCurr, currText);

    int totalCount = 0;
    for (const auto& chap : g_ChapterList)
    {
        totalCount += CountValidWord(chap.Content);
    }
    wchar_t totalText[64] = { 0 };
    swprintf(totalText, L"全书总字数：%d 字", totalCount);
    SetWindowTextW(hLabelTotalWordCount, totalText);
}

// ===================== 章节操作 =====================
void RefreshChapterList()
{
    SendMessageW(hListChapterBox, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_ChapterList.size(); i++)
    {
        wchar_t itemText[256] = { 0 };
        swprintf(itemText, L"第%d章 %s", i + 1, g_ChapterList[i].Title.c_str());
        SendMessageW(hListChapterBox, LB_ADDSTRING, 0, (LPARAM)itemText);
    }
    RefreshWordCount();
}

void LoadSelectChapter(int index)
{
    g_SelectChapterIndex = index;
    SetWindowTextW(hEditChapterTitle, g_ChapterList[index].Title.c_str());
    SetWindowTextW(hEditChapterContent, g_ChapterList[index].Content.c_str());
}

void SaveCurrentChapter()
{
    wchar_t titleBuffer[256] = { 0 };
    wchar_t contentBuffer[65535] = { 0 };
    GetWindowTextW(hEditChapterTitle, titleBuffer, 256);
    GetWindowTextW(hEditChapterContent, contentBuffer, 65535);

    std::wstring title = titleBuffer;
    std::wstring content = contentBuffer;

    if (title.empty())
    {
        MessageBoxW(hMainWindow, L"请输入章节标题！", L"提示", MB_ICONWARNING);
        return;
    }

    if (g_SelectChapterIndex < 0)
    {
        g_ChapterList.push_back({ title, content });
    }
    else
    {
        g_ChapterList[g_SelectChapterIndex] = { title, content };
    }

    RefreshChapterList();
    SetWindowTextW(hEditChapterTitle, L"");
    SetWindowTextW(hEditChapterContent, L"");
    g_SelectChapterIndex = -1;
    MessageBoxW(hMainWindow, L"✅ 章节保存成功", L"成功", MB_OK);
}

// ===================== 文件操作 =====================
void ExportNovelToTxt()
{
    wchar_t novelNameBuffer[256] = { 0 };
    GetWindowTextW(hEditNovelName, novelNameBuffer, 256);
    std::wstring novelName = novelNameBuffer;
    if (novelName.empty()) novelName = L"未命名小说";

    wchar_t savePath[MAX_PATH] = { 0 };
    OPENFILENAMEW openFileInfo = { 0 };
    openFileInfo.lStructSize = sizeof(OPENFILENAMEW);
    openFileInfo.hwndOwner = hMainWindow;
    openFileInfo.lpstrFilter = L"文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0";
    openFileInfo.lpstrFile = savePath;
    openFileInfo.nMaxFile = MAX_PATH;
    openFileInfo.lpstrTitle = L"导出小说为TXT";
    openFileInfo.Flags = OFN_OVERWRITEPROMPT;
    openFileInfo.lpstrDefExt = L"txt";

    if (!GetSaveFileNameW(&openFileInfo)) return;

    std::wstring fullContent = L"《" + novelName + L"》\r\n\r\n";
    for (int i = 0; i < g_ChapterList.size(); i++)
    {
        const auto& chap = g_ChapterList[i];
        fullContent += L"第" + std::to_wstring(i + 1) + L"章 " + chap.Title + L"\r\n\r\n";
        fullContent += chap.Content + L"\r\n\r\n\r\n";
    }

    HANDLE hFile = CreateFileW(savePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        std::string utf8Content = WideToUtf8(fullContent);
        DWORD writeSize = 0;
        WriteFile(hFile, utf8Content.c_str(), (DWORD)utf8Content.size(), &writeSize, NULL);
        CloseHandle(hFile);
        MessageBoxW(hMainWindow, L"✅ TXT导出成功", L"成功", MB_OK);
    }
}

void SaveNovelProject()
{
    wchar_t savePath[MAX_PATH] = { 0 };
    OPENFILENAMEW openFileInfo = { 0 };
    openFileInfo.lStructSize = sizeof(OPENFILENAMEW);
    openFileInfo.hwndOwner = hMainWindow;
    openFileInfo.lpstrFilter = L"小说项目(*.nov)\0*.nov\0所有文件(*.*)\0*.*\0";
    openFileInfo.lpstrFile = savePath;
    openFileInfo.nMaxFile = MAX_PATH;
    openFileInfo.lpstrTitle = L"保存小说项目";
    openFileInfo.Flags = OFN_OVERWRITEPROMPT;
    openFileInfo.lpstrDefExt = L"nov";

    if (!GetSaveFileNameW(&openFileInfo)) return;

    wchar_t buffer[65535] = { 0 };
    GetWindowTextW(hEditNovelName, buffer, 256); std::wstring novelName = buffer;
    GetWindowTextW(hEditWorldView, buffer, 65535); std::wstring worldView = buffer;
    GetWindowTextW(hEditCharacter, buffer, 65535); std::wstring character = buffer;
    GetWindowTextW(hEditOutline, buffer, 65535); std::wstring outline = buffer;
    GetWindowTextW(hEditForeshadow, buffer, 65535); std::wstring foreshadow = buffer;
    GetWindowTextW(hEditInspire, buffer, 65535); std::wstring inspire = buffer;

    std::wstring projectData = novelName + L"\n###SPLIT###\n" +
        worldView + L"\n###SPLIT###\n" +
        character + L"\n###SPLIT###\n" +
        outline + L"\n###SPLIT###\n" +
        foreshadow + L"\n###SPLIT###\n" +
        inspire;

    for (const auto& chap : g_ChapterList)
    {
        projectData += L"\n###CHAPTER###\n" + chap.Title + L"\n###CONTENT###\n" + chap.Content;
    }

    HANDLE hFile = CreateFileW(savePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        std::string utf8Data = WideToUtf8(projectData);
        DWORD writeSize = 0;
        WriteFile(hFile, utf8Data.c_str(), (DWORD)utf8Data.size(), &writeSize, NULL);
        CloseHandle(hFile);
        MessageBoxW(hMainWindow, L"✅ 项目保存成功", L"成功", MB_OK);
    }
}

void ResetAllContent()
{
    if (MessageBoxW(hMainWindow, L"⚠️ 确定要清空所有内容吗？", L"确认", MB_YESNO | MB_ICONWARNING) != IDYES)
        return;

    SetWindowTextW(hEditNovelName, L"");
    SetWindowTextW(hEditWorldView, L"");
    SetWindowTextW(hEditCharacter, L"");
    SetWindowTextW(hEditOutline, L"");
    SetWindowTextW(hEditForeshadow, L"");
    SetWindowTextW(hEditInspire, L"");

    g_ChapterList.clear();
    RefreshChapterList();
    SetWindowTextW(hEditChapterTitle, L"");
    SetWindowTextW(hEditChapterContent, L"");
}

void ToggleDarkTheme()
{
    g_IsDarkTheme = !g_IsDarkTheme;
    InvalidateRect(hMainWindow, NULL, TRUE);
}

// ===================== 窗口回调 =====================
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        hMainWindow = hWnd;
        const int WIN_WIDTH = 1200;
        const int WIN_HEIGHT = 700;

        RECT screenRect;
        GetWindowRect(GetDesktopWindow(), &screenRect);
        int winX = (screenRect.right - WIN_WIDTH) / 2;
        int winY = (screenRect.bottom - WIN_HEIGHT) / 2;
        SetWindowPos(hWnd, NULL, winX, winY, WIN_WIDTH, WIN_HEIGHT, 0);

        // ========== 左侧 ==========
        int leftY = CONTROL_PADDING;
        CreateWindowW(L"STATIC", L"📖 小说名", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditNovelName = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 70, leftY, LEFT_PANEL_WIDTH - 80, 24, hWnd, NULL, NULL, NULL);
        leftY += 36;

        CreateWindowW(L"STATIC", L"🌍 世界观", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditWorldView = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowW(L"STATIC", L"👤 人物", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditCharacter = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowW(L"STATIC", L"📝 大纲", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditOutline = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowW(L"STATIC", L"🔍 伏笔", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditForeshadow = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowW(L"STATIC", L"💡 灵感", WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditInspire = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);

        // ========== 中间 ==========
        int midX = LEFT_PANEL_WIDTH + CONTROL_PADDING * 2;
        CreateWindowW(L"STATIC", L"📖 章节列表", WS_CHILD | WS_VISIBLE, midX, CONTROL_PADDING, MID_PANEL_WIDTH, 20, hWnd, NULL, NULL, NULL);
        hListChapterBox = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY, midX, 30, MID_PANEL_WIDTH, WIN_HEIGHT - 80, hWnd, (HMENU)ID_LIST_CHAPTER, NULL, NULL);

        // ========== 右侧 ==========
        int rightX = LEFT_PANEL_WIDTH + MID_PANEL_WIDTH + CONTROL_PADDING * 3;
        int rightCtrlWidth = WIN_WIDTH - rightX - CONTROL_PADDING;

        CreateWindowW(L"BUTTON", L"💾 保存章节", WS_CHILD | WS_VISIBLE, rightX, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_SAVE_CHAP, NULL, NULL);
        CreateWindowW(L"BUTTON", L"📦 保存项目", WS_CHILD | WS_VISIBLE, rightX + 115, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_SAVE_PROJ, NULL, NULL);
        CreateWindowW(L"BUTTON", L"📄 导出TXT", WS_CHILD | WS_VISIBLE, rightX + 230, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_EXPORT_TXT, NULL, NULL);
        CreateWindowW(L"BUTTON", L"🔄 一键初始化", WS_CHILD | WS_VISIBLE, rightX + 345, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_RESET, NULL, NULL);
        CreateWindowW(L"BUTTON", L"🌓 明暗切换", WS_CHILD | WS_VISIBLE, rightX + 460, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_THEME, NULL, NULL);

        hEditChapterTitle = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, rightX, 40, rightCtrlWidth, 24, hWnd, NULL, NULL, NULL);
        hEditChapterContent = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_WANTRETURN, rightX, 72, rightCtrlWidth, WIN_HEIGHT - 180, hWnd, NULL, NULL, NULL);

        hLabelWordCountCurr = CreateWindowW(L"STATIC", L"当前章节：0 字", WS_CHILD | WS_VISIBLE, rightX, WIN_HEIGHT - 90, 200, 20, hWnd, NULL, NULL, NULL);
        hLabelTotalWordCount = CreateWindowW(L"STATIC", L"全书总字数：0 字", WS_CHILD | WS_VISIBLE, rightX + 220, WIN_HEIGHT - 90, 200, 20, hWnd, NULL, NULL, NULL);
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_BTN_SAVE_CHAP: SaveCurrentChapter(); break;
        case ID_BTN_SAVE_PROJ: SaveNovelProject(); break;
        case ID_BTN_EXPORT_TXT: ExportNovelToTxt(); break;
        case ID_BTN_RESET: ResetAllContent(); break;
        case ID_BTN_THEME: ToggleDarkTheme(); break;
        }

        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == ID_LIST_CHAPTER)
        {
            int selectIndex = SendMessageW(hListChapterBox, LB_GETCURSEL, 0, 0);
            if (selectIndex != LB_ERR) LoadSelectChapter(selectIndex);
        }

        if ((HWND)lParam == hEditChapterContent && HIWORD(wParam) == EN_CHANGE)
        {
            RefreshWordCount();
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paintInfo;
        HDC hDC = BeginPaint(hWnd, &paintInfo);
        if (g_IsDarkTheme)
            FillRect(hDC, &paintInfo.rcPaint, CreateSolidBrush(RGB(26, 29, 36)));
        else
            FillRect(hDC, &paintInfo.rcPaint, CreateSolidBrush(RGB(245, 247, 250)));
        EndPaint(hWnd, &paintInfo);
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

// ===================== 程序入口 =====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    INITCOMMONCONTROLSEX commonCtrlInfo = { 0 };
    commonCtrlInfo.dwSize = sizeof(INITCOMMONCONTROLSEX);
    commonCtrlInfo.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&commonCtrlInfo);

    const wchar_t WINDOW_CLASS_NAME[] = L"NovelWriterWin32Class";
    WNDCLASSW windowClass = { 0 };
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&windowClass);

    HWND hWnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"小说创作器 · 纯C++原生完整版",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 700,
        NULL, NULL,
        hInstance, NULL
    );
    ShowWindow(hWnd, nCmdShow);

    MSG message = { 0 };
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}
