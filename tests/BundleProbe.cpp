#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <pluginterfaces/base/ipluginbase.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <pluginterfaces/vst/vstspeaker.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/gui/iplugview.h>
#include <iostream>
#include <array>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <thread>

using namespace Steinberg;
using namespace Steinberg::Vst;
class ProbeHost final:public IHostApplication {
public:
    tresult PLUGIN_API queryInterface(const TUID iid,void** obj)override{*obj=nullptr;if(std::memcmp(iid,IHostApplication_iid,16)==0||std::memcmp(iid,FUnknown_iid,16)==0){*obj=this;return kResultOk;}return kNoInterface;}
    uint32 PLUGIN_API addRef()override{return 1;}
    uint32 PLUGIN_API release()override{return 1;}
    tresult PLUGIN_API getName(String128 name)override{std::memset(name,0,sizeof(String128));constexpr char16_t text[]=u"Cracked808s Bundle Probe";std::memcpy(name,text,sizeof(text));return kResultOk;}
    tresult PLUGIN_API createInstance(TUID,TUID,void** obj)override{*obj=nullptr;return kNoInterface;}
};

int wmain(int argc,wchar_t** argv){
    std::cout.setf(std::ios::unitbuf);
    if(argc<2){std::cerr<<"usage: BundleProbe module.vst3 [required CLSID]\n";return 2;}
    if(FAILED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED)))return 16;
    auto dll=LoadLibraryW(argv[1]);if(!dll){std::cerr<<"LoadLibrary failed: "<<GetLastError()<<'\n';return 3;}
    auto init=(bool(*)())GetProcAddress(dll,"InitDll");auto exit=(bool(*)())GetProcAddress(dll,"ExitDll");if(init&&!init()){std::cerr<<"InitDll failed\n";return 4;}
    auto get=(IPluginFactory*(*)())GetProcAddress(dll,"GetPluginFactory");if(!get){std::cerr<<"factory export missing\n";return 5;}auto* factory=get();PFactoryInfo fi{};factory->getFactoryInfo(&fi);std::cout<<"vendor: "<<fi.vendor<<"; classes: "<<factory->countClasses()<<'\n';
    TUID componentId{};bool found=false;for(int i=0;i<factory->countClasses();++i){PClassInfo ci{};factory->getClassInfo(i,&ci);wchar_t id[64]{};GUID guid{};std::memcpy(&guid,ci.cid,16);StringFromGUID2(guid,id,64);std::wcout<<ci.name<<L" | "<<id<<L"\n";if(std::strcmp(ci.category,kVstAudioEffectClass)==0){std::memcpy(componentId,ci.cid,16);found=true;}}
    if(!found)return 6;
    if(argc>2){GUID required{};if(FAILED(CLSIDFromString(argv[2],&required)))return 7;std::memcpy(componentId,&required,16);}
    IComponent* component=nullptr;auto created=factory->createInstance(componentId,IComponent_iid,(void**)&component);std::cout<<"create requested component: "<<created<<'\n';if(created!=kResultOk||!component){factory->release();if(exit)exit();FreeLibrary(dll);return 8;}
    ProbeHost host;auto initialized=component->initialize(&host);std::cout<<"initialize: "<<initialized<<'\n';if(initialized!=kResultOk)return 9;
    IEditController* controller=nullptr;IConnectionPoint* componentConnection=nullptr;IConnectionPoint* controllerConnection=nullptr;
    if(argc>3&&std::wcscmp(argv[3],L"--editor")==0){
        TUID controllerId{};component->getControllerClassId(controllerId);
        if(factory->createInstance(controllerId,IEditController_iid,(void**)&controller)!=kResultOk||!controller||controller->initialize(&host)!=kResultOk)return 12;
        component->queryInterface(IConnectionPoint_iid,(void**)&componentConnection);controller->queryInterface(IConnectionPoint_iid,(void**)&controllerConnection);
        if(!componentConnection||!controllerConnection)return 13;
        componentConnection->connect(controllerConnection);controllerConnection->connect(componentConnection);
        // Audio-only probes never instantiate the font, image caches or UI timer.
        for(int cycle=0;cycle<3;++cycle){
            auto* view=controller->createView("editor");if(!view)return 14;
            auto window=CreateWindowExW(WS_EX_NOACTIVATE,L"STATIC",L"Cracked808s shutdown regression",WS_OVERLAPPEDWINDOW,0,0,1160,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            if(!window||view->attached(window,kPlatformTypeHWND)!=kResultOk)return 15;
            ShowWindow(window,SW_SHOWNOACTIVATE);
            const auto until=GetTickCount64()+350;
            do{MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(5);}while(GetTickCount64()<until);
            std::cout<<"remove editor "<<cycle<<'\n';if(view->removed()!=kResultOk)return 17;view->release();DestroyWindow(window);
        }
        std::cout<<"editors removed\n";
    }
    IAudioProcessor* processor=nullptr;component->queryInterface(IAudioProcessor_iid,(void**)&processor);if(!processor)return 10;
    SpeakerArrangement arrangement=SpeakerArr::kStereo;auto arranged=processor->setBusArrangements(&arrangement,1,&arrangement,1);ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;setup.maxSamplesPerBlock=256;setup.sampleRate=48000;
    auto prepared=processor->setupProcessing(setup);component->activateBus(kAudio,kInput,0,true);component->activateBus(kAudio,kOutput,0,true);auto active=component->setActive(true);auto processing=processor->setProcessing(true);
    std::cout<<"arrange/setup/active/process: "<<arranged<<'/'<<prepared<<'/'<<active<<'/'<<processing<<"; latency "<<processor->getLatencySamples()<<'\n';
    std::array<std::array<float,256>,2> buffer{};float* channels[]={buffer[0].data(),buffer[1].data()};AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=channels;
    ProcessData data{};data.processMode=kRealtime;data.symbolicSampleSize=kSample32;data.numSamples=256;data.numInputs=1;data.numOutputs=1;data.inputs=&bus;data.outputs=&bus;
    bool ok=arranged==kResultOk&&prepared==kResultOk&&active==kResultOk;
    for(int block=0;block<16;++block){for(int j=0;j<256;++j)buffer[0][(size_t)j]=buffer[1][(size_t)j]=.2f*std::sin((float)(block*256+j)*.03f);ok&=processor->process(data)==kResultOk;for(auto& channel:buffer)for(auto sample:channel)ok&=std::isfinite(sample);}
    processor->setProcessing(false);component->setActive(false);processor->release();
    if(controller){componentConnection->disconnect(controllerConnection);controllerConnection->disconnect(componentConnection);componentConnection->release();controllerConnection->release();controller->terminate();controller->release();}
    component->terminate();component->release();factory->release();std::cout<<"instances released\n";
    if(argc>4&&std::wcscmp(argv[4],L"--process-exit")==0){std::cout<<"host process exit with module still loaded\n";ExitProcess(0);}
    if(exit&&!exit())return 18;std::cout<<"FreeLibrary begin\n";
    bool unloaded=false;
    if(argc>4&&std::wcscmp(argv[4],L"--worker-unload")==0){
        // Host UI objects have already been removed/terminated on their proper
        // thread. Windows may run final DLL cleanup on a different thread; that
        // cleanup must not depend on the host's stopped UI message pump.
        std::thread worker([&]{unloaded=FreeLibrary(dll)!=0;});worker.join();
    }else unloaded=FreeLibrary(dll)!=0;
    if(!unloaded)return 19;std::cout<<"FreeLibrary complete\n";CoUninitialize();std::cout<<(ok?"BUNDLE PASS\n":"BUNDLE FAIL\n");return ok?0:11;
}
