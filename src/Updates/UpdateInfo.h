#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <optional>

namespace cracked::updates {
inline constexpr const char* pageUrl="https://www.crackedvault.workers.dev/plugins/cracked808s";
inline constexpr juce::int64 dayMs=86400000;
inline std::optional<std::array<int,3>> parseVersion(const juce::String& text){
    const auto parts=juce::StringArray::fromTokens(text,".","");if(parts.size()!=3)return {};
    std::array<int,3> result{};
    for(int i=0;i<3;++i){const auto& p=parts[i];if(p.isEmpty()||p.length()>4||!p.containsOnly("0123456789")||(p.length()>1&&p.startsWithChar('0')))return {};result[(size_t)i]=p.getIntValue();}
    return result;
}
inline bool newer(const juce::String& candidate,const juce::String& installed){const auto a=parseVersion(candidate),b=parseVersion(installed);return a&&b&&*a>*b;}
inline juce::String manifestVersion(const juce::String& body){
    if(body.getNumBytesAsUTF8()>16384)return {};
    const auto value=juce::JSON::parse(body);auto* object=value.getDynamicObject();if(!object)return {};
    if(!object->getProperty("schema").isInt()||(int)object->getProperty("schema")!=1||object->getProperty("product").toString()!="Cracked808s"||object->getProperty("page").toString()!=pageUrl)return {};
    const auto version=object->getProperty("version");return version.isString()&&parseVersion(version.toString())?version.toString():juce::String{};
}
inline bool due(juce::int64 last,juce::int64 now){return last<=0||last>now+dayMs||now-last>=dayMs;}
inline juce::File cacheDirectory(){return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Cracked Vault/Cracked808s/Updates");}
inline juce::var readCache(const juce::File& folder){const auto file=folder.getChildFile("result.json");return file.getSize()>0&&file.getSize()<=16384?juce::JSON::parse(file.loadFileAsString()):juce::var{};}
inline bool writeResult(const juce::File& folder,bool ok,const juce::String& version,juce::int64 checkedAt){
    if(folder.createDirectory().failed())return false;
    juce::DynamicObject::Ptr data=new juce::DynamicObject();data->setProperty("ok",ok);data->setProperty("version",ok?version:juce::String{});data->setProperty("checkedAt",checkedAt);
    return folder.getChildFile("result.json").replaceWithText(juce::JSON::toString(juce::var(data.get())));
}
}
