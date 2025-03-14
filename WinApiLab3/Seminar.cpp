#include <Windows.h>
#include <iostream>

int main()
{
    HANDLE hFile = CreateFile(
        L"data.txt",
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Error create file:" << GetLastError() << std::endl;
        return -1;
    }

    HANDLE hFileMap = CreateFileMapping(
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        256,
        NULL
    );

    if (hFileMap == NULL) {
        std::cout << "Error create file mapping:" << GetLastError() << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    LPVOID hFileView = MapViewOfFile(
        hFileMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        3
    );

    if (hFileView == NULL) {
        std::cout << "Error create file view:" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(hFileMap);
        return -1;
    }

    const char* message = "This new text test";
    CopyMemory((LPVOID)hFileView, message, strlen(message) + 1);

    const char data[] = "This test text";
    DWORD writedBytes = 0;

    BOOL isWrite = WriteFile(
        hFile,
        data,
        strlen(data),
        &writedBytes,
        NULL
    );

    if (!isWrite) {
        std::cout << "Error write file" << GetLastError() << std::endl;
        CloseHandle(hFile);
        return -1;
    }

    std::cout << "Success" << std::endl;
    UnmapViewOfFile(hFileView);
    CloseHandle(hFile);
    CloseHandle(hFileMap);

    return 0;
}
