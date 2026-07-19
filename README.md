# DLL Injector

A lightweight Windows DLL injector that embeds the payload as a resource inside the executable — no loose DLL file needed.

## How it works

1. The target DLL is embedded at compile time via the `.rc` resource file
2. On launch, it extracts the DLL to `%TEMP%` as a hidden file
3. Waits for the target process to start (up to 30 seconds)
4. Injects via `CreateRemoteThread` + `LoadLibraryW`
5. Cleans up the temp file after injection

## Setup

**1. Set your target process**

In `Injector.cpp`, find:
```cpp
pid = FindProcess(L"SetGameName.exe");
```
Replace `SetGameName.exe` with your target process name.

**2. Set your DLL**

Place your DLL in the project root and make sure `Injector.rc` references it:
```rc
1 DLL "your_payload.dll"
```

**3. Build**

Open `Mk_Injector.vcxproj` in Visual Studio (2019/2022), set to `Release x64`, build.

## Requirements

- Windows 10+
- Visual Studio 2019 or 2022
- Target process must be running or start within 30 seconds of launching the injector

## Notes

- Injector must be run as **Administrator** for `PROCESS_ALL_ACCESS` to work
- The embedded DLL is written to `%TEMP%` briefly then deleted after injection
