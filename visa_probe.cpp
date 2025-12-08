#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#include <cstdint>

static void print_version_info(HMODULE h) {
    char path[MAX_PATH] = {0};
    if (!GetModuleFileNameA(h, path, sizeof(path))) {
        std::printf("(no module path)\n");
        return;
    }
    DWORD dummy = 0;
    DWORD sz = GetFileVersionInfoSizeA(path, &dummy);
    if (sz == 0) {
        std::printf("File: %s\n  (no version resource)\n", path);
        return;
    }
    void* buf = HeapAlloc(GetProcessHeap(), 0, sz);
    if (!buf) {
        std::printf("File: %s\n  (alloc failure)\n", path);
        return;
    }
    if (!GetFileVersionInfoA(path, 0, sz, buf)) {
        std::printf("File: %s\n  (GetFileVersionInfo failed)\n", path);
        HeapFree(GetProcessHeap(), 0, buf);
        return;
    }
    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT ffiLen = 0;
    if (VerQueryValueA(buf, "\\", reinterpret_cast<LPVOID*>(&ffi), &ffiLen) && ffi && ffiLen >= sizeof(VS_FIXEDFILEINFO)) {
        auto hi = [](DWORD x){ return static_cast<unsigned>(HIWORD(x)); };
        auto lo = [](DWORD x){ return static_cast<unsigned>(LOWORD(x)); };
        std::printf("File: %s\n  FileVersion: %u.%u.%u.%u\n  ProductVersion: %u.%u.%u.%u\n",
            path,
            hi(ffi->dwFileVersionMS), lo(ffi->dwFileVersionMS), hi(ffi->dwFileVersionLS), lo(ffi->dwFileVersionLS),
            hi(ffi->dwProductVersionMS), lo(ffi->dwProductVersionMS), hi(ffi->dwProductVersionLS), lo(ffi->dwProductVersionLS)
        );
    } else {
        std::printf("File: %s\n  (no VS_FIXEDFILEINFO)\n", path);
    }
    HeapFree(GetProcessHeap(), 0, buf);
}

static const char* kVisaCandidates[] = {
    "visa64.dll",
    "nivisa64.dll",
    "visa32.dll",
    "nivisa32.dll",
};

static const char* kVisaSymbols[] = {
    "viOpenDefaultRM",
    "viOpen",
    "viClose",
    "viRead",
    "viWrite",
    "viSetAttribute",
    "viGetAttribute",
    "viFindRsrc",
};

int main() {
    // Allow override via environment variable
    char customPath[1024];
    DWORD got = GetEnvironmentVariableA("VISA_DLL_PATH", customPath, sizeof(customPath));

    HMODULE h = nullptr;
    if (got > 0 && got < sizeof(customPath)) {
        h = LoadLibraryA(customPath);
        std::printf("VISA_DLL_PATH set -> %s\n", customPath);
    }

    const char* loaded = nullptr;
    if (!h) {
        for (const char* name : kVisaCandidates) {
            h = LoadLibraryA(name);
            if (h) { loaded = name; break; }
        }
    } else {
        loaded = customPath;
    }

    if (!h) {
        std::fprintf(stderr, "visa-probe: could not load any VISA DLL (tried default candidates).\n");
        std::fprintf(stderr, "Set VISA_DLL_PATH to a specific DLL to try that first.\n");
        return 1;
    }

    std::printf("Loaded: %s\n", loaded ? loaded : "(custom path)");
    print_version_info(h);
    int present = 0;
    for (const char* sym : kVisaSymbols) {
        FARPROC p = GetProcAddress(h, sym);
        std::printf("  %-16s : %s\n", sym, p ? "present" : "missing");
        if (p) present++;
    }

    FreeLibrary(h);
    std::printf("Summary: %d/%zu expected symbols present\n", present, sizeof(kVisaSymbols)/sizeof(kVisaSymbols[0]));
    return 0;
}
#else
int main(){return 0;}
#endif
