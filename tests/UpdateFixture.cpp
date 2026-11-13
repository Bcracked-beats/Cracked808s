#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>

// Test utility only. Never copied into a plugin, installer or public binary ZIP.
int wmain(int argc,wchar_t** argv){
    std::filesystem::path folder;
    for(int i=1;i<argc;++i)if(std::wstring(argv[i])==L"--cache-dir"&&i+1<argc)folder=argv[++i];
    if(folder.empty())return 2;std::filesystem::create_directories(folder);
    wchar_t event[64]{};if(GetEnvironmentVariableW(L"CRACKED808S_TEST_EVENT",event,64))SetEvent(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(_wcstoui64(event,nullptr,10))));
    {std::ofstream started(folder/L"started.txt");started<<"started";}
    Sleep(1600);
    const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::ofstream result(folder/L"result.json");result<<"{\"ok\":true,\"version\":\"9.0.0\",\"checkedAt\":"<<now<<"}";return result?0:1;
}
