#include <Windows.h>
#include <TlHelp32.h>
#include <string>

static DWORD FindProcess(const wchar_t* name)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe))
        do { if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; } }
        while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return pid;
}

static std::wstring ExtractDllToTemp()
{
    HMODULE hSelf = GetModuleHandleW(nullptr);
    HRSRC   hRes  = FindResourceW(hSelf, MAKEINTRESOURCEW(1), L"DLL");
    if (!hRes) return L"";
    HGLOBAL hGlob = LoadResource(hSelf, hRes);
    if (!hGlob) return L"";
    void*  pData = LockResource(hGlob);
    DWORD  size  = SizeofResource(hSelf, hRes);
    if (!pData || !size) return L"";

    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring path = std::wstring(tempDir) + L"mkinjector_payload.dll";

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return L"";
    DWORD written = 0;
    WriteFile(hFile, pData, size, &written, nullptr);
    CloseHandle(hFile);
    return (written == size) ? path : L"";
}

static bool Inject(DWORD pid, const std::wstring& dllPath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;

    SIZE_T pathLen = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(hProc, nullptr, pathLen,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { CloseHandle(hProc); return false; }

    WriteProcessMemory(hProc, remote, dllPath.c_str(), pathLen, nullptr);

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(
            GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"),
        remote, 0, nullptr);

    if (!hThread)
    {
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return true;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    std::wstring dllPath = ExtractDllToTemp();
    if (dllPath.empty()) return 1;

    DWORD pid = 0;
    for (int i = 0; i < 300 && pid == 0; i++)
    {
        pid = FindProcess(L"MHUR.exe"); // Please set the games handle name here 
        if (pid == 0) Sleep(100);
    }
    if (pid == 0)
    {
        DeleteFileW(dllPath.c_str());
        return 1;
    }

    Sleep(3000);
    Inject(pid, dllPath);
    Sleep(2000);
    DeleteFileW(dllPath.c_str());
    return 0;
}
