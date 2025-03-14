#include <windows.h>

// Глобальные переменные настроек и состояния
unsigned short cell_size = 50; // размер клетки в пикселях
int grid_width = 10; // количество клеток по горизонтали
int grid_height = 10; // количество клеток по вертикали
LPBYTE grid;  // массив состояния клеток: 0 – пусто, 1 – круг, 2 – крест

int window_height = 320;
int window_width = 480;

// Прототип оконной процедуры
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Функция для изменения размера сетки
void ResizeGrid(int new_width, int new_height) {

    LPBYTE new_grid = (LPBYTE)GlobalAlloc(GPTR, new_width * new_height * sizeof(BYTE));

    for (int y = 0; y < min(grid_height, new_height); y++) {
        for (int x = 0; x < min(grid_width, new_width); x++) {
            // Индекс в старом массиве
            int old_index = y * grid_width + x;
            // Индекс в новом массиве
            int new_index = y * new_width + x;
            // Копируем значение
            new_grid[new_index] = grid[old_index];
        }
    }
    GlobalFree(grid);
    grid = new_grid;
    grid_width = new_width;
    grid_height = new_height;
}

// Функция сохранения состояния в бинарный файл "save.dat"
void SaveState()
{
    HANDLE hFile = CreateFileW(L"save.dat", GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        // Сохраняем размеры сетки и размер клетки
        WriteFile(hFile, &grid_width, sizeof(grid_width), &written, NULL);
        WriteFile(hFile, &grid_height, sizeof(grid_height), &written, NULL);
        WriteFile(hFile, &cell_size, sizeof(cell_size), &written, NULL);
        // Сохраняем массив состояния клеток
        WriteFile(hFile, grid, grid_width * grid_height, &written, NULL);
        CloseHandle(hFile);
    }
}

// Функция загрузки состояния из файла "save.dat"
// Возвращает TRUE при успешной загрузке
BOOL LoadState()
{
    HANDLE hFile = CreateFileW(L"save.dat", GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD readBytes;
    int file_width, file_height, file_cell_size;
    if (!ReadFile(hFile, &file_width, sizeof(file_width), &readBytes, NULL) || readBytes != sizeof(file_width))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    if (!ReadFile(hFile, &file_height, sizeof(file_height), &readBytes, NULL) || readBytes != sizeof(file_height))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    if (!ReadFile(hFile, &file_cell_size, sizeof(file_cell_size), &readBytes, NULL) || readBytes != sizeof(
        file_cell_size))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    // Обновляем глобальные переменные из файла
    grid_width = file_width;
    grid_height = file_height;
    cell_size = file_cell_size;

    SIZE_T gridSize = grid_width * grid_height;
    grid = (LPBYTE)GlobalAlloc(GPTR, gridSize);
    if (!grid)
    {
        CloseHandle(hFile);
        return FALSE;
    }
    if (!ReadFile(hFile, grid, gridSize, &readBytes, NULL) || readBytes != gridSize)
    {
        GlobalFree(grid);
        grid = NULL;
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // Парсинг аргументов командной строки
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; i++)
    {
        if (lstrcmpW(argv[i], L"-size") == 0 && i + 1 < argc)
        {
            cell_size = _wtoi(argv[i + 1]);
            i++;
        }
        else if (lstrcmpW(argv[i], L"-width") == 0 && i + 1 < argc)
        {
            grid_width = _wtoi(argv[i + 1]);
            i++;
        }
        else if (lstrcmpW(argv[i], L"-height") == 0 && i + 1 < argc)
        {
            grid_height = _wtoi(argv[i + 1]);
            i++;
        }
    }
    LocalFree(argv);

    // Если не указан -reset и файл save.dat существует, загружаем состояние
    if (LoadState())
    {
        // Состояние успешно загружено
    }
    else
    {
        SIZE_T gridSize = grid_width * grid_height;
        grid = (LPBYTE)GlobalAlloc(GPTR, gridSize);
        if (grid)
            ZeroMemory(grid, gridSize);
    }

    // Регистрация класса окна
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc))
        return -1;

    // Расчёт размеров окна: клиентская область = (grid_width*cell_size) x (grid_height*cell_size)
    RECT rect = {0, 0, grid_width * cell_size, grid_height * cell_size};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowW(L"MainWindowClass", L"WinApi Window",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              320,
                              240,
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

    // Освобождаем выделенную память
    if (grid)
        GlobalFree(grid);

    return (int)msg.wParam;
}

RECT rect;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RegisterHotKey(hwnd, 1, MOD_CONTROL, 0x51);
    RegisterHotKey(hwnd, 2, 0, VK_ESCAPE);
    switch (uMsg)
    {
    // Левый клик — рисуем круг в выбранной клетке
    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int col = x / cell_size;
            int row = y / cell_size;
            if (col >= 0 && col < grid_width && row >= 0 && row < grid_height)
            {
                int index = row * grid_width + col;
                grid[index] = 1; // 1 = круг
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
    // Правый клик — рисуем крест в выбранной клетке
    case WM_RBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int col = x / cell_size;
            int row = y / cell_size;
            
            if (col >= 0 && col < grid_width && row >= 0 && row < grid_height)
            {
                int index = row * grid_width + col;
                grid[index] = 2; // 2 = крест
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
    case WM_SIZE:
    {
        int new_window_width = LOWORD(lParam);
        int new_window_height = HIWORD(lParam);
        int new_grid_width = new_window_width / cell_size;
        int new_grid_height = new_window_height / cell_size;
        ResizeGrid(new_grid_width, new_grid_height);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    // Перерисовка окна: рисуем сетку и содержимое клеток
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Рисуем линии сетки
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            
            GetClientRect(hwnd, &rect);

            window_height = rect.bottom;
            window_width = rect.right;

            grid_width = window_width / cell_size;
            grid_height = window_height / cell_size;

            // Вертикальные линии
            for (int i = 0; i <= grid_width; i++)
            {
                int x = i * cell_size;
                MoveToEx(hdc, x, 0, NULL);
                LineTo(hdc, x, window_height);
            }
            // Горизонтальные линии
            for (int j = 0; j <= grid_height; j++)
            {
                int y = j * cell_size;
                MoveToEx(hdc, 0, y, NULL);
                LineTo(hdc, window_width, y);
            }
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            DeleteObject(hOldPen);

            // Рисуем фигуры в клетках
            for (int row = 0; row < grid_height; row++)
            {
                for (int col = 0; col < grid_width; col++)
                {
                    int index = row * grid_width + col;
                    int left = col * cell_size;
                    int top = row * cell_size;
                    int right = left + cell_size;
                    int bottom = top + cell_size;
                    int offset = cell_size / 8; // отступ для красоты

                    if (grid[index] == 1)
                    {
                        // Рисуем круг
                        Ellipse(hdc, left + offset, top + offset, right - offset, bottom - offset);
                    }
                    else if (grid[index] == 2)
                    {
                        // Рисуем крест
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
            SaveState();
            UnregisterHotKey(hwnd, 1);
            UnregisterHotKey(hwnd, 2);
            PostQuitMessage(0);
            return 0;
        }
        return 0;
    // При закрытии окна сохраняем состояние и завершаем приложение
    case WM_DESTROY:
        SaveState();
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
