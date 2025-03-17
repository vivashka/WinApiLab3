#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include <vector>


// Для метода 3 – потоки ввода-вывода C++
#include <fstream>
#include <sstream>
#include <string>

// Глобальные переменные настроек и состояния поля
int cell_size = 50;       // размер клетки в пикселях
int grid_width = 10;      // число клеток по горизонтали
int grid_height = 10;     // число клеток по вертикали
LPBYTE grid = NULL;       // массив состояния клеток: 0 – пусто, 1 – круг, 2 – крест

int window_width = 480;   // ширина клиентской области окна
int window_height = 320;  // высота клиентской области окна

// Стандартные значения по умолчанию
const int DEFAULT_CELL_SIZE = 50;
const int DEFAULT_WINDOW_WIDTH = 480;
const int DEFAULT_WINDOW_HEIGHT = 320;
const COLORREF DEFAULT_BG_COLOR = 0xFFFFFF; // белый
const COLORREF DEFAULT_GRID_COLOR = 0x000000; // черный

// Структура для настроек
struct Config {
    int cellSize;
    int window_width;
    int window_height;
    COLORREF bgColor;
    COLORREF gridColor;
};

// Глобальный объект настроек
Config g_config;

// Для сохранения состояния поля при загрузке из файла
LPBYTE g_savedGrid = NULL;
int g_savedGridWidth = 0, g_savedGridHeight = 0;

// Глобальная переменная выбора метода
int g_method = 4;

// Имя файла конфигурации
const TCHAR* CONFIG_FILENAME = _T("config.cfg");

// Прототип оконной процедуры
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Функция чтения настроек и сохранённого состояния поля с помощью отображения файла в память
bool ReadConfigMapping(Config& cfg)
{
    HANDLE hFile = CreateFile(_T("config.cfg"), GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return false;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, fileSize, NULL);
    if (!hMapping) {
        CloseHandle(hFile);
        return false;
    }

    LPVOID pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, fileSize);
    if (!pView) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return false;
    }

    char* buffer = static_cast<char*>(pView);
    int offset = 0;
    int n = sscanf_s(buffer, "%d\n%d\n%d\n%X\n%X\n%d\n%d\n%n",
        &cfg.cellSize,
        &cfg.window_width,
        &cfg.window_height,
        reinterpret_cast<unsigned int*>(&cfg.bgColor),
        reinterpret_cast<unsigned int*>(&cfg.gridColor),
        &g_savedGridWidth,
        &g_savedGridHeight,
        &offset);

    if (n < 7) {
        UnmapViewOfFile(pView);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return false;
    }

    int totalCells = g_savedGridWidth * g_savedGridHeight;
    if (totalCells > 0)
    {
        g_savedGrid = (LPBYTE)GlobalAlloc(GPTR, totalCells * sizeof(BYTE));
        char* p = buffer + offset;
        for (int i = 0; i < totalCells; i++) {
            int val = 0, nChars = 0;
            if (sscanf_s(p, "%d%n", &val, &nChars) != 1)
                break;
            g_savedGrid[i] = (BYTE)val;
            p += nChars;
        }
    }

    UnmapViewOfFile(pView);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return true;
}

bool WriteConfigMapping(const Config& cfg)
{
    HANDLE hFile = CreateFile(_T("config.cfg"), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    char configBuffer[256];
    int len1 = sprintf_s(configBuffer, sizeof(configBuffer),
        "%d\n%d\n%d\n%06X\n%06X\n%d\n%d\n",
        cfg.cellSize,
        cfg.window_width,
        cfg.window_height,
        cfg.bgColor,
        cfg.gridColor,
        grid_width,
        grid_height);

    int totalCells = grid_width * grid_height;
    std::vector<char> gridBuffer(totalCells * 4);
    int pos = 0;
    for (int i = 0; i < totalCells; i++) {
        int written = sprintf_s(gridBuffer.data() + pos, gridBuffer.size() - pos, "%d ", grid[i]);
        pos += written;
        if (pos >= gridBuffer.size())
            break;
    }

    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, configBuffer, (DWORD)strlen(configBuffer), &bytesWritten, NULL);
    result = result && WriteFile(hFile, gridBuffer.data(), (DWORD)pos, &bytesWritten, NULL);
    CloseHandle(hFile);
    return (result != FALSE);
}


// Реализация метода 2: стандартные файловые функции (fopen, fread, fwrite, fclose)
bool ReadConfigC(Config& cfg)
{
    FILE* file;
    errno_t err = _wfopen_s(&file, CONFIG_FILENAME, L"r");
    if (err != 0 || file == nullptr) {
        wprintf(L"Failed to open the file.\n");
    }
    // Считываем параметры: cellSize, window_width, window_height, bgColor, gridColor, saved_grid_width, saved_grid_height
    if (fscanf_s(file, "%d\n%d\n%d\n%X\n%X\n%d\n%d\n",
        &cfg.cellSize,
        &cfg.window_width,
        &cfg.window_height,
        &cfg.bgColor,
        &cfg.gridColor,
        &g_savedGridWidth,
        &g_savedGridHeight) != 7)
    {
        fclose(file);
        return false;
    }
    int totalCells = g_savedGridWidth * g_savedGridHeight;
    if (totalCells > 0)
    {
        g_savedGrid = (LPBYTE)GlobalAlloc(GPTR, totalCells * sizeof(BYTE));
        for (int i = 0; i < totalCells; i++) {
            int val = 0;
            fscanf_s(file, "%d ", &val);
            g_savedGrid[i] = (BYTE)val;
        }
    }
    fclose(file);
    return true;
}

bool WriteConfigC(const Config& cfg)
{
    FILE* file;
    errno_t err = _wfopen_s(&file, CONFIG_FILENAME, L"w");
    if (err != 0 || file == nullptr) {
        wprintf(L"Failed to open the file.\n");
    }
    // Записываем параметры
    fprintf(file, "%d\n%d\n%d\n%06X\n%06X\n%d\n%d\n",
        cell_size,
        window_width,
        window_height,
        cfg.bgColor,
        cfg.gridColor,
        grid_width,
        grid_height);
    int totalCells = grid_width * grid_height;
    for (int i = 0; i < totalCells; i++)
    {
        fprintf(file, "%d ", grid[i]);
    }
    fclose(file);
    return true;
}

// Реализация метода 3: потоки ввода-вывода C++ (fstream)
bool ReadConfigFstream(Config& cfg)
{
    std::ifstream ifs("config.cfg");
    if (!ifs.is_open())
        return false;
    std::string line;
    try {
        std::getline(ifs, line);
        cfg.cellSize = std::stoi(line);
        std::getline(ifs, line);
        cfg.window_width = std::stoi(line);
        std::getline(ifs, line);
        cfg.window_height = std::stoi(line);
        std::getline(ifs, line);
        cfg.bgColor = (COLORREF)std::stoul(line, nullptr, 16);
        std::getline(ifs, line);
        cfg.gridColor = (COLORREF)std::stoul(line, nullptr, 16);
        std::getline(ifs, line);
        g_savedGridWidth = std::stoi(line);
        std::getline(ifs, line);
        g_savedGridHeight = std::stoi(line);
    }
    catch (...) {
        ifs.close();
        return false;
    }
    int totalCells = g_savedGridWidth * g_savedGridHeight;
    if (totalCells > 0)
    {
        g_savedGrid = (LPBYTE)GlobalAlloc(GPTR, totalCells * sizeof(BYTE));
        for (int i = 0; i < totalCells; i++) {
            int val;
            ifs >> val;
            g_savedGrid[i] = (BYTE)val;
        }
    }
    ifs.close();
    return true;
}

bool WriteConfigFstream(const Config& cfg)
{
    std::ofstream ofs("config.cfg");
    if (!ofs.is_open())
        return false;
    ofs << cell_size << "\n"
        << window_width << "\n"
        << window_height << "\n";
    ofs << std::hex << cfg.bgColor << "\n"
        << std::hex << cfg.gridColor << "\n";
    ofs << std::dec << grid_width << "\n"
        << grid_height << "\n";
    int totalCells = grid_width * grid_height;
    for (int i = 0; i < totalCells; i++) {
        ofs << static_cast<int>(grid[i]) << " ";
    }
    ofs.close();
    return true;
}

// Реализация метода 4: файловые функции WinAPI (CreateFile, ReadFile, WriteFile)
bool ReadConfigWinAPI(Config& cfg)
{
    HANDLE hFile = CreateFile(CONFIG_FILENAME, GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    char buffer[8192] = { 0 };
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
    {
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);
    buffer[bytesRead] = '\0';
    int offset = 0;
    int n = sscanf_s(buffer, "%d\n%d\n%d\n%X\n%X\n%d\n%d\n%n",
        &cfg.cellSize,
        &cfg.window_width,
        &cfg.window_height,
        reinterpret_cast<unsigned int*>(&cfg.bgColor),
        reinterpret_cast<unsigned int*>(&cfg.gridColor),
        &g_savedGridWidth,
        &g_savedGridHeight,
        &offset);
    if (n < 7)
        return false;
    int totalCells = g_savedGridWidth * g_savedGridHeight;
    if (totalCells > 0)
    {
        g_savedGrid = (LPBYTE)GlobalAlloc(GPTR, totalCells * sizeof(BYTE));
        char* p = buffer + offset;
        for (int i = 0; i < totalCells; i++) {
            int val = 0, nChars = 0;
            if (sscanf_s(p, "%d%n", &val, &nChars) != 1)
                break;
            g_savedGrid[i] = (BYTE)val;
            p += nChars;
        }
    }
    return true;
}

bool WriteConfigWinAPI(const Config& cfg)
{
    HANDLE hFile = CreateFile(CONFIG_FILENAME, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    char configBuffer[256];
    int len1 = sprintf_s(configBuffer, sizeof(configBuffer),
        "%d\n%d\n%d\n%06X\n%06X\n%d\n%d\n",
        cell_size,
        window_width,
        window_height,
        cfg.bgColor,
        cfg.gridColor,
        grid_width,
        grid_height);
    char gridBuffer[4096] = { 0 };
    int pos = 0;
    int totalCells = grid_width * grid_height;
    for (int i = 0; i < totalCells; i++) {
        int written = sprintf_s(gridBuffer + pos, sizeof(gridBuffer) - pos, "%d ", grid[i]);
        pos += written;
        if (pos >= sizeof(gridBuffer))
            break;
    }
    DWORD bytesWritten = 0;
    BOOL result = WriteFile(hFile, configBuffer, (DWORD)strlen(configBuffer), &bytesWritten, NULL);
    result = result && WriteFile(hFile, gridBuffer, (DWORD)strlen(gridBuffer), &bytesWritten, NULL);
    CloseHandle(hFile);
    return (result != FALSE);
}


// Функции-обёртки для выбора метода чтения/записи конфигурации и состояния
bool ReadConfig(Config& cfg)
{
    switch (g_method)
    {
        
    case 1: return ReadConfigMapping(cfg);
    case 2: return ReadConfigC(cfg);
    case 3: return ReadConfigFstream(cfg);
    case 4: return ReadConfigWinAPI(cfg);
    default: return false;
    }
}

bool WriteConfig(const Config& cfg)
{
    switch (g_method)
    {
    case 1: return WriteConfigMapping(cfg);
    case 2: return WriteConfigC(cfg);
    case 3: return WriteConfigFstream(cfg);
    case 4: return WriteConfigWinAPI(cfg);
    default: return false;
    }
}


// Функция изменения размера сетки (при изменении размеров окна или при загрузке состояния)
void ResizeGrid(int new_width, int new_height) {
    LPBYTE new_grid = (LPBYTE)GlobalAlloc(GPTR, new_width * new_height * sizeof(BYTE));
    if (!new_grid)
        return;
    int minH = (grid_height < new_height) ? grid_height : new_height;
    int minW = (grid_width < new_width) ? grid_width : new_width;
    for (int y = 0; y < minH; y++) {
        for (int x = 0; x < minW; x++) {
            int old_index = y * grid_width + x;
            int new_index = y * new_width + x;
            new_grid[new_index] = grid[old_index];
        }
    }
    GlobalFree(grid);
    grid = new_grid;
    grid_width = new_width;
    grid_height = new_height;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // Парсинг аргументов командной строки: -size, -width, -height, -m (метод: 2,3 или 4)
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; i++)
    {
        if (lstrcmpW(argv[i], L"-size") == 0 && i + 1 < argc)
        {
            g_config.cellSize = _wtoi(argv[++i]);
        }
        else if (lstrcmpW(argv[i], L"-width") == 0 && i + 1 < argc)
        {
            g_config.window_width = _wtoi(argv[++i]);
        }
        else if (lstrcmpW(argv[i], L"-height") == 0 && i + 1 < argc)
        {
            g_config.window_height = _wtoi(argv[++i]);
        }
        else if (lstrcmpW(argv[i], L"-m") == 0 && i + 1 < argc)
        {
            g_method = _wtoi(argv[++i]);
            if (g_method < 2 || g_method > 4)
                g_method = 2;
        }
    }
    LocalFree(argv);

    // Пробуем загрузить настройки и сохранённое поле выбранным методом
    if (!ReadConfig(g_config))
    {
        // Если файла нет или произошла ошибка – используем значения по умолчанию
        g_config.cellSize = DEFAULT_CELL_SIZE;
        g_config.window_width = DEFAULT_WINDOW_WIDTH;
        g_config.window_height = DEFAULT_WINDOW_HEIGHT;
        g_config.bgColor = DEFAULT_BG_COLOR;
        g_config.gridColor = DEFAULT_GRID_COLOR;
    }

    // Обновляем глобальные переменные из настроек
    cell_size = g_config.cellSize;
    window_width = g_config.window_width;
    window_height = g_config.window_height;

    // Вычисляем размеры сетки по клиентской области
    grid_width = window_width / cell_size;
    grid_height = window_height / cell_size;

    // Выделяем память для grid
    grid = (LPBYTE)GlobalAlloc(GPTR, grid_width * grid_height * sizeof(BYTE));
    if (grid)
        ZeroMemory(grid, grid_width * grid_height);

    // Если в файле было сохранено состояние поля, копируем его (по минимальному пересечению)
    if (g_savedGrid)
    {
        int minW = (g_savedGridWidth < grid_width) ? g_savedGridWidth : grid_width;
        int minH = (g_savedGridHeight < grid_height) ? g_savedGridHeight : grid_height;
        for (int y = 0; y < minH; y++) {
            for (int x = 0; x < minW; x++) {
                grid[y * grid_width + x] = g_savedGrid[y * g_savedGridWidth + x];
            }
        }
        GlobalFree(g_savedGrid);
        g_savedGrid = NULL;
    }

    // Регистрация класса окна
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc))
        return -1;

    // Расчёт размеров окна с учетом рамок (клиентская область = window_width x window_height)
    RECT rect = { 0, 0, window_width, window_height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowW(L"MainWindowClass", L"WinApi Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return -1;

    // Основной цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Перед завершением обновляем размеры окна в настройках
    g_config.window_width = window_width;
    g_config.window_height = window_height;
    // Сохраняем настройки и состояние поля выбранным методом
    WriteConfig(g_config);

    if (grid)
        GlobalFree(grid);

    return (int)msg.wParam;
}

// Глобальный RECT для перерисовки
RECT rect;

// Оконная процедура
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Регистрируем горячие клавиши: Ctrl+Q или Esc – выход
    RegisterHotKey(hwnd, 1, MOD_CONTROL, 0x51);
    RegisterHotKey(hwnd, 2, 0, VK_ESCAPE);
    switch (uMsg)
    {
    case WM_LBUTTONDOWN: // Левый клик – рисуем круг (значение 1)
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int col = x / cell_size;
        int row = y / cell_size;
        if (col >= 0 && col < grid_width && row >= 0 && row < grid_height)
        {
            int index = row * grid_width + col;
            grid[index] = 1;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: // Правый клик – рисуем крест (значение 2)
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int col = x / cell_size;
        int row = y / cell_size;
        if (col >= 0 && col < grid_width && row >= 0 && row < grid_height)
        {
            int index = row * grid_width + col;
            grid[index] = 2;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_SIZE:
    {
        int new_window_width = LOWORD(lParam);
        int new_window_height = HIWORD(lParam);
        window_width = new_window_width;
        window_height = new_window_height;
        int new_grid_width = new_window_width / cell_size;
        int new_grid_height = new_window_height / cell_size;
        ResizeGrid(new_grid_width, new_grid_height);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        // Заливаем фон
        HBRUSH hBrushBG = CreateSolidBrush(g_config.bgColor);
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, hBrushBG);
        DeleteObject(hBrushBG);
        // Рисуем сетку
        HPEN hPen = CreatePen(PS_SOLID, 1, g_config.gridColor);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        for (int i = 0; i <= grid_width; i++) {
            int x = i * cell_size;
            MoveToEx(hdc, x, 0, NULL);
            LineTo(hdc, x, window_height);
        }
        for (int j = 0; j <= grid_height; j++) {
            int y = j * cell_size;
            MoveToEx(hdc, 0, y, NULL);
            LineTo(hdc, window_width, y);
        }
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        // Рисуем фигуры
        for (int row = 0; row < grid_height; row++) {
            for (int col = 0; col < grid_width; col++) {
                int index = row * grid_width + col;
                int left = col * cell_size;
                int top = row * cell_size;
                int right = left + cell_size;
                int bottom = top + cell_size;
                int offset = cell_size / 8;
                if (grid[index] == 1) {
                    Ellipse(hdc, left + offset, top + offset, right - offset, bottom - offset);
                }
                else if (grid[index] == 2) {
                    MoveToEx(hdc, left + offset, top + offset, NULL);
                    LineTo(hdc, right - offset, bottom - offset);
                    MoveToEx(hdc, right - offset, top + offset, NULL);
                    LineTo(hdc, left + offset, bottom - offset);
                }
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_HOTKEY:
        if (wParam == 1 || wParam == 2)
        {
            UnregisterHotKey(hwnd, 1);
            UnregisterHotKey(hwnd, 2);
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_DESTROY:
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
