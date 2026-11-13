#include "UpdateInfo.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <memory>

namespace {
using Http=std::unique_ptr<void,decltype(&WinHttpCloseHandle)>;
juce::String fetchVersion(){
    Http session(WinHttpOpen(L"Cracked808s-update-check/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0),WinHttpCloseHandle);if(!session)return {};
    if(!WinHttpSetTimeouts(session.get(),1500,1500,1500,1500))return {};
    DWORD retries=1;if(!WinHttpSetOption(session.get(),WINHTTP_OPTION_CONNECT_RETRIES,&retries,sizeof(retries)))return {};
    Http connection(WinHttpConnect(session.get(),L"www.crackedvault.workers.dev",INTERNET_DEFAULT_HTTPS_PORT,0),WinHttpCloseHandle);if(!connection)return {};
    Http request(WinHttpOpenRequest(connection.get(),L"GET",L"/updates/cracked808s.json",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE),WinHttpCloseHandle);if(!request)return {};
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
    if(!WinHttpSetOption(request.get(),WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||!WinHttpSetOption(request.get(),WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled)))return {};
    if(!WinHttpSendRequest(request.get(),L"Accept: application/json\r\nDNT: 1\r\nSec-GPC: 1\r\n",(DWORD)-1L,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.get(),nullptr))return {};
    DWORD status=0,size=sizeof(status);if(!WinHttpQueryHeaders(request.get(),WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)||status!=200)return {};
    wchar_t type[256]{};size=sizeof(type);if(!WinHttpQueryHeaders(request.get(),WINHTTP_QUERY_CONTENT_TYPE,WINHTTP_HEADER_NAME_BY_INDEX,type,&size,WINHTTP_NO_HEADER_INDEX)||!juce::String(type).startsWithIgnoreCase("application/json"))return {};
    juce::MemoryOutputStream body;const auto start=GetTickCount64();
    for(;;){char bytes[2048];DWORD count=0;if(GetTickCount64()-start>6000||!WinHttpReadData(request.get(),bytes,sizeof(bytes),&count))return {};if(count==0)break;if(body.getDataSize()+count>16384)return {};body.write(bytes,count);}
    return cracked::updates::manifestVersion(body.toString());
}
}
int wmain(int argc,wchar_t** argv){
    juce::File folder=cracked::updates::cacheDirectory();bool force=false;
    for(int i=1;i<argc;++i){const juce::String arg(argv[i]);if(arg=="--force")force=true;else if(arg=="--cache-dir"&&i+1<argc){const juce::String path(argv[++i]);if(!juce::File::isAbsolutePath(path))return 2;folder=juce::File(path);}else return 2;}
    // The same per-user cache names the mutex across all DAWs and plugin instances.
    const auto name="Global\\Cracked808sUpdate-"+juce::String::toHexString(folder.getFullPathName().toLowerCase().hashCode64());
    HANDLE mutex=CreateMutexW(nullptr,TRUE,name.toWideCharPointer());if(!mutex)return 2;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);return 0;}
    const auto now=juce::Time::currentTimeMillis();const auto cached=cracked::updates::readCache(folder);
    if(!force&&!cracked::updates::due((juce::int64)cached["checkedAt"],now)){ReleaseMutex(mutex);CloseHandle(mutex);return 0;}
    // Failed attempts are cached too, so offline users are not polled repeatedly.
    if(!cracked::updates::writeResult(folder,false,{},now)){ReleaseMutex(mutex);CloseHandle(mutex);return 2;}
    const auto version=fetchVersion();const bool saved=cracked::updates::writeResult(folder,version.isNotEmpty(),version,now);
    ReleaseMutex(mutex);CloseHandle(mutex);return saved&&version.isNotEmpty()?0:1;
}
