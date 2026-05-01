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
HWND hLabelWordCountCurr, hLabelWordCountTotal;

// ===================== 数据结构 =====================
struct Chapter
{
    std::string Title;
    std::string Content;
};
std::vector<Chapter> g_ChapterList;
int g_SelectChapterIndex = -1;
bool g_IsDarkTheme = false;

// ===================== 尺寸常量 =====================
const int LEFT_PANEL_WIDTH = 300;
const int MID_PANEL_WIDTH = 200;
const int CONTROL_PADDING = 8;

// ===================== 中文常量（16进制硬编码，绝对不乱码） =====================
#define TXT_NOVEL_NAME      "\xE5\xB0\x8F\xE8\xAF\x9D\xE5\x90\x8D"
#define TXT_WORLD_VIEW      "\xE4\xB8\x96\xE7\x95\x8C\xE8\xA7\x82"
#define TXT_CHARACTER       "\xE4\xBA\xBA\xE7\x89\xA9"
#define TXT_OUTLINE         "\xE5\xA4\xA7\xE7\xBA\xB2"
#define TXT_FORESHAWDOW     "\xE4\xBC\x87\xE7\xAC\x86"
#define TXT_INSPIRE         "\xE7\x81\xB5\xE6\x84\x9F"
#define TXT_CHAPTER_LIST    "\xE7\xAB\xA0\xE8\x8A\x82\xE5\x88\x97\xE8\xA1\xA8"
#define TXT_CHAPTER_TITLE    "\xE6\xA0\x87\xE8\x8A\x82\xE6\xA0\x87\xE9\xA2\x98"
#define TXT_CHAPTER_CONTENT  "\xE6\xAD\xA3\xE6\x96\x87"
#define TXT_SAVE_CHAPTER     "\xE4\xBF\x9D\xE5\xAD\x98\xE7\xAB\xA0\xE8\x8A\x82"
#define TXT_SAVE_PROJECT     "\xE4\xBF\x9D\xE5\xAD\x98\xE9\xA1\xB9\xE7\x9B\xAE"
#define TXT_EXPORT_TXT       "\xE5\xAF\xBC\xE5\x87\xBA\xTXT"
#define TXT_RESET_ALL        "\xE4\xB8\x80\xE9\x94\xAE\xE5\x88\x9D\xE5\xA7\x8B\xE5\x8C\x96"
#define TXT_THEME_TOGGLE     "\xE6\x98\x8E\xE6\x9A\x97\xE5\x88\x87\xE6\x8D\xA2"
#define TXT_CURR_WORD_COUNT  "\xE5\xBD\x93\xE5\x89\x8D\xE7\xAB\xA0\xE8\x8A\x82\xEF\xBC\x9A0\xE5\xAD\x97"
#define TXT_TOTAL_WORD_COUNT "\xE5\x85\xA8\xE4\xB9\xA6\xE6\x80\xBB\xE5\xAD\x97\xEF\xBC\x9A0\xE5\xAD\x97"
#define TXT_APP_TITLE        "\xE5\xB0\x8F\xE8\xAF\x9D\xE5\x88\x9B\xE4\xBD\x9C\xE5\x99\xA8 \xC2\xB7 \xE7\xB9\xAF\xE7\xBA\xAF\xC2\x2B\x2B\xE5\x8E\x9F\xE7\x94\x9F\xE5\x8E\x9F\xE7\x89\x88\xE7\x89\x88\xE7\x89\x88\xE7\x89\x88"

// ===================== 字数统计 =====================
int CountValidWord(const std::string& text)
{
    int count = 0;
    for (char ch : text)
    {
        if (ch != ' ' && ch != '\n' && ch != '\r') count++;
    }
    return count;
}

void RefreshWordCount()
{
    char contentBuffer[65535] = { 0 };
    GetWindowTextA(hEditChapterContent, contentBuffer, 65535);
    int currCount = CountValidWord(contentBuffer);
    char currText[64] = { 0 };
    sprintf(currText, "当前章节：%d 字", currCount);
    SetWindowTextA(hLabelWordCountCurr, currText);

    int totalCount = 0;
    for (const auto& chap : g_ChapterList)
    {
        totalCount += CountValidWord(chap.Content);
    }
    char totalText[64] = { 0 };
    sprintf(totalText, "全书总字数：%d 字", totalCount);
    SetWindowTextA(hLabelWordCountTotal, totalText);
}

// ===================== 章节操作 =====================
void RefreshChapterList()
{
    SendMessageA(hListChapterBox, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_ChapterList.size(); i++)
    {
        char itemText[256] = { 0 };
        sprintf(itemText, "第%d章 %s", i + 1, g_ChapterList[i].Title.c_str());
        SendMessageA(hListChapterBox, LB_ADDSTRING, 0, (LPARAM)itemText);
    }
    RefreshWordCount();
}

void LoadSelectChapter(int index)
{
    g_SelectChapterIndex = index;
    SetWindowTextA(hEditChapterTitle, g_ChapterList[index].Title.c_str());
    SetWindowTextA(hEditChapterContent, g_ChapterList[index].Content.c_str());
}

void SaveCurrentChapter()
{
    char titleBuffer[256] = { 0 };
    char contentBuffer[65535] = { 0 };
    GetWindowTextA(hEditChapterTitle, titleBuffer, 256);
    GetWindowTextA(hEditChapterContent, contentBuffer, 65535);

    std::string title = titleBuffer;
    std::string content = contentBuffer;

    if (title.empty())
    {
        MessageBoxA(hMainWindow, "请输入章节标题！", "提示", MB_ICONWARNING);
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
    SetWindowTextA(hEditChapterTitle, "");
    SetWindowTextA(hEditChapterContent, "");
    g_SelectChapterIndex = -1;
    MessageBoxA(hMainWindow, "✅ 章节保存成功", "成功", MB_OK);
}

// ===================== 文件操作 =====================
void ExportNovelToTxt()
{
    char novelNameBuffer[256] = { 0 };
    GetWindowTextA(hEditNovelName, novelNameBuffer, 256);
    std::string novelName = novelNameBuffer;
    if (novelName.empty()) novelName = "未命名小说";

    char savePath[MAX_PATH] = { 0 };
    OPENFILENAMEA openFileInfo = { 0 };
    openFileInfo.lStructSize = sizeof(OPENFILENAMEA);
    openFileInfo.hwndOwner = hMainWindow;
    openFileInfo.lpstrFilter = "文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0";
    openFileInfo.lpstrFile = savePath;
    openFileInfo.nMaxFile = MAX_PATH;
    openFileInfo.lpstrTitle = "导出小说为TXT";
    openFileInfo.Flags = OFN_OVERWRITEPROMPT;
    openFileInfo.lpstrDefExt = "txt";

    if (!GetSaveFileNameA(&openFileInfo)) return;

    std::string fullContent = "《" + novelName + "》\r\n\r\n";
    for (int i = 0; i < g_ChapterList.size(); i++)
    {
        const auto& chap = g_ChapterList[i];
        fullContent += "第" + std::to_string(i + 1) + "章 " + chap.Title + "\r\n\r\n";
        fullContent += chap.Content + "\r\n\r\n\r\n";
    }

    HANDLE hFile = CreateFileA(savePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD writeSize = 0;
        WriteFile(hFile, fullContent.c_str(), (DWORD)fullContent.size(), &writeSize, NULL);
        CloseHandle(hFile);
        MessageBoxA(hMainWindow, "✅ TXT导出成功", "成功", MB_OK);
    }
}

void SaveNovelProject()
{
    char savePath[MAX_PATH] = { 0 };
    OPENFILENAMEA openFileInfo = { 0 };
    openFileInfo.lStructSize = sizeof(OPENFILENAMEA);
    openFileInfo.hwndOwner = hMainWindow;
    openFileInfo.lpstrFilter = "小说项目(*.nov)\0*.nov\0所有文件(*.*)\0*.*\0";
    openFileInfo.lpstrFile = savePath;
    openFileInfo.nMaxFile = MAX_PATH;
    openFileInfo.lpstrTitle = "保存小说项目";
    openFileInfo.Flags = OFN_OVERWRITEPROMPT;
    openFileInfo.lpstrDefExt = "nov";

    if (!GetSaveFileNameA(&openFileInfo)) return;

    char buffer[65535] = { 0 };
    GetWindowTextA(hEditNovelName, buffer, 256); std::string novelName = buffer;
    GetWindowTextA(hEditWorldView, buffer, 65535); std::string worldView = buffer;
    GetWindowTextA(hEditCharacter, buffer, 65535); std::string character = buffer;
    GetWindowTextA(hEditOutline, buffer, 65535); std::string outline = buffer;
    GetWindowTextA(hEditForeshadow, buffer, 65535); std::string foreshadow = buffer;
    GetWindowTextA(hEditInspire, buffer, 65535); std::string inspire = buffer;

    std::string projectData = novelName + "\n###SPLIT###\n" +
        worldView + "\n###SPLIT###\n" +
        character + "\n###SPLIT###\n" +
        outline + "\n###SPLIT###\n" +
        foreshadow + "\n###SPLIT###\n" +
        inspire;

    for (const auto& chap : g_ChapterList)
    {
        projectData += "\n###CHAPTER###\n" + chap.Title + "\n###CONTENT###\n" + chap.Content;
    }

    HANDLE hFile = CreateFileA(savePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD writeSize = 0;
        WriteFile(hFile, projectData.c_str(), (DWORD)projectData.size(), &writeSize, NULL);
        CloseHandle(hFile);
        MessageBoxA(hMainWindow, "✅ 项目保存成功", "成功", MB_OK);
    }
}

void ResetAllContent()
{
    if (MessageBoxA(hMainWindow, "⚠️ 确定要清空所有内容吗？", "确认", MB_YESNO | MB_ICONWARNING) != IDYES)
        return;

    SetWindowTextA(hEditNovelName, "");
    SetWindowTextA(hEditWorldView, "");
    SetWindowTextA(hEditCharacter, "");
    SetWindowTextA(hEditOutline, "");
    SetWindowTextA(hEditForeshadow, "");
    SetWindowTextA(hEditInspire, "");

    g_ChapterList.clear();
    RefreshChapterList();
    SetWindowTextA(hEditChapterTitle, "");
    SetWindowTextA(hEditChapterContent, "");
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
        CreateWindowA("STATIC", TXT_NOVEL_NAME, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditNovelName = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 70, leftY, LEFT_PANEL_WIDTH - 80, 24, hWnd, NULL, NULL, NULL);
        leftY += 36;

        CreateWindowA("STATIC", TXT_WORLD_VIEW, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditWorldView = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowA("STATIC", TXT_CHARACTER, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditCharacter = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowA("STATIC", TXT_OUTLINE, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditOutline = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowA("STATIC", TXT_FORESHAWDOW, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditForeshadow = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);
        leftY += 132;

        CreateWindowA("STATIC", TXT_INSPIRE, WS_CHILD | WS_VISIBLE, CONTROL_PADDING, leftY, 60, 20, hWnd, NULL, NULL, NULL);
        hEditInspire = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, CONTROL_PADDING, leftY + 24, LEFT_PANEL_WIDTH - CONTROL_PADDING * 2, 100, hWnd, NULL, NULL, NULL);

        // ========== 中间 ==========
        int midX = LEFT_PANEL_WIDTH + CONTROL_PADDING * 2;
        CreateWindowA("STATIC", TXT_CHAPTER_LIST, WS_CHILD | WS_VISIBLE, midX, CONTROL_PADDING, MID_PANEL_WIDTH, 20, hWnd, NULL, NULL, NULL);
        hListChapterBox = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | LBS_NOTIFY, midX, 30, MID_PANEL_WIDTH, WIN_HEIGHT - 80, hWnd, (HMENU)ID_LIST_CHAPTER, NULL, NULL);

        // ========== 右侧 ==========
        int rightX = LEFT_PANEL_WIDTH + MID_PANEL_WIDTH + CONTROL_PADDING * 3;
        int rightCtrlWidth = WIN_WIDTH - rightX - CONTROL_PADDING;

        CreateWindowA("BUTTON", TXT_SAVE_CHAPTER, WS_CHILD | WS_VISIBLE, rightX, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_SAVE_CHAP, NULL, NULL);
        CreateWindowA("BUTTON", TXT_SAVE_PROJECT, WS_CHILD | WS_VISIBLE, rightX + 115, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_SAVE_PROJ, NULL, NULL);
        CreateWindowA("BUTTON", TXT_EXPORT_TXT, WS_CHILD | WS_VISIBLE, rightX + 230, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_EXPORT_TXT, NULL, NULL);
        CreateWindowA("BUTTON", TXT_RESET_ALL, WS_CHILD | WS_VISIBLE, rightX + 345, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_RESET, NULL, NULL);
        CreateWindowA("BUTTON", TXT_THEME_TOGGLE, WS_CHILD | WS_VISIBLE, rightX + 460, CONTROL_PADDING, 110, 28, hWnd, (HMENU)ID_BTN_THEME, NULL, NULL);

        hEditChapterTitle = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, rightX, 40, rightCtrlWidth, 24, hWnd, NULL, NULL, NULL);
        hEditChapterContent = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_WANTRETURN, rightX, 72, rightCtrlWidth, WIN_HEIGHT - 180, hWnd, NULL, NULL, NULL);

        hLabelWordCountCurr = CreateWindowA("STATIC", TXT_CURR_WORD_COUNT, WS_CHILD | WS_VISIBLE, rightX, WIN_HEIGHT - 90, 200, 20, hWnd, NULL, NULL, NULL);
        hLabelWordCountTotal = CreateWindowA("STATIC", TXT_TOTAL_WORD_COUNT, WS_CHILD | WS_VISIBLE, rightX + 220, WIN_HEIGHT - 90, 200, 20, hWnd, NULL, NULL, NULL);
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
            int selectIndex = SendMessageA(hListChapterBox, LB_GETCURSEL, 0, 0);
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

    const char WINDOW_CLASS_NAME[] = "NovelWriterWin32Class";
    WNDCLASSA windowClass = { 0 };
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&windowClass);

    HWND hWnd = CreateWindowExA(
        0,
        WINDOW_CLASS_NAME,
        TXT_APP_TITLE,
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
