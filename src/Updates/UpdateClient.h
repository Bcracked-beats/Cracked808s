#pragma once
#include "UpdateInfo.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#endif

namespace cracked::updates {
// Only launches a bundled process and reads a small local cache. No DLL network worker.
class Client {
public:
    explicit Client(juce::File cache=cacheDirectory(),juce::File executable=juce::File::getSpecialLocation(juce::File::currentApplicationFile).getSiblingFile("Cracked808sUpdate.exe")):folder(std::move(cache)),helper(std::move(executable)){
        readPreference();
        refresh();
    }
    ~Client(){
#if JUCE_WINDOWS
        if(process)CloseHandle(process);
#endif
    }
    bool automatic()const{return enabled;}
    bool setAutomatic(bool value){
        if(folder.createDirectory().failed()||!folder.getChildFile("preferences.json").replaceWithText(value?"{\"automatic\":true}":"{\"automatic\":false}")){message="Could not save update preference.";return false;}
        enabled=value;return true;
    }
    void start(bool manual){
        if(running())return;
        if(!manual)readPreference();
        refresh();const auto cached=readCache(folder);const auto last=(juce::int64)cached["checkedAt"];
        if(!manual&&(!enabled||!due(last,juce::Time::currentTimeMillis())))return;
        if(!helper.existsAsFile()){message="Update helper missing. Reinstall the complete bundle.";return;}
        if(folder.createDirectory().failed()){message="Could not access the update cache.";return;}
        started=juce::Time::getMillisecondCounterHiRes();previousCheck=last;
        waiting=launch(manual);message=waiting?"Checking for updates...":"Update helper could not start.";
    }
    void poll(){
        const auto now=juce::Time::getMillisecondCounterHiRes();if(now-lastPoll<1000)return;lastPoll=now;
        if(waiting&&(running()&&now-started<12000))return;
        if(waiting){waiting=false;const auto result=readCache(folder);if((juce::int64)result["checkedAt"]<=previousCheck){message="No new result. Check again later.";return;}}
        refresh();
        readPreference();const auto cached=readCache(folder);
        if(enabled&&!waiting&&due((juce::int64)cached["checkedAt"],juce::Time::currentTimeMillis())&&now-lastAutoAttempt>=60000){lastAutoAttempt=now;start(false);}
    }
    bool checking()const{return waiting;}
    bool available()const{return newer(latest,JucePlugin_VersionString);}
    juce::String status()const{return message;}
private:
    bool running()const{
#if JUCE_WINDOWS
        return process&&WaitForSingleObject(process,0)==WAIT_TIMEOUT;
#else
        return false;
#endif
    }
    bool launch(bool manual){
#if JUCE_WINDOWS
        if(process){CloseHandle(process);process=nullptr;}
        const auto command=helper.getFullPathName().quoted()+" --cache-dir "+folder.getFullPathName().quoted()+(manual?" --force":"");std::wstring buffer(command.toWideCharPointer());
        STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
        // No pipes or inheritable DAW handles. Closing this client only closes our process handle.
        if(!CreateProcessW(helper.getFullPathName().toWideCharPointer(),buffer.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT,nullptr,nullptr,&startup,&child))return false;
        CloseHandle(child.hThread);process=child.hProcess;return true;
#else
        juce::ignoreUnused(manual);return false;
#endif
    }
    void readPreference(){const auto file=folder.getChildFile("preferences.json");const auto value=file.getSize()>0&&file.getSize()<1024?juce::JSON::parse(file.loadFileAsString()):juce::var{};enabled=value["automatic"].isBool()&&(bool)value["automatic"];}
    void refresh(){
        const auto data=readCache(folder);const auto version=data["version"].toString();
        if(data["ok"].isBool()&&(bool)data["ok"]&&parseVersion(version)){
            latest=version;message=available()?"Version "+latest+" is available.":"No newer public release. Installed: "+juce::String(JucePlugin_VersionString);
        }else{latest.clear();message=data.isObject()?"Update check unavailable. Audio works offline.":"Updates are optional. No check made yet.";}
    }
    juce::File folder,helper;
#if JUCE_WINDOWS
    HANDLE process=nullptr;
#endif
    bool enabled=false,waiting=false;juce::String latest,message;double lastPoll=0,started=0,lastAutoAttempt=-60000;juce::int64 previousCheck=0;
    JUCE_DECLARE_NON_COPYABLE(Client)
};
}
