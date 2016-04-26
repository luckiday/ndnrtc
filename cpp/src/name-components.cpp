//
//  name-components.cpp
//  libndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#include <string> 
#include <sstream>
#include <algorithm>
#include <iterator>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "name-components.h"

using namespace std;
using namespace ndnrtc;
using namespace ndn;

const string NameComponents::NameComponentApp = "ndnrtc";
#if 0
const string NameComponents::NameComponentUser = "user";
const string NameComponents::NameComponentSession = "session-info";
const string NameComponents::NameComponentBroadcast = "/ndn/broadcast";
const string NameComponents::NameComponentDiscovery = "discovery";
const string NameComponents::NameComponentUserStreams = "streams";
const string NameComponents::NameComponentStreamAccess = "access";
const string NameComponents::NameComponentStreamKey = "key";
const string NameComponents::NameComponentStreamFramesDelta = "delta";
const string NameComponents::NameComponentStreamFramesKey = "key";
const string NameComponents::NameComponentStreamInfo = "info";
const string NameComponents::NameComponentFrameSegmentData = "data";
const string NameComponents::NameComponentFrameSegmentParity = "parity";
const string NameComponents::KeyComponent = "DSK-1408";
const string NameComponents::CertificateComponent = "KEY/ID-CERT/0";
#endif

const string NameComponents::NameComponentAudio = "audio";
const string NameComponents::NameComponentVideo = "video";
const string NameComponents::NameComponentMeta = "_meta";
const string NameComponents::NameComponentDelta = "d";
const string NameComponents::NameComponentKey = "k";
const string NameComponents::NameComponentParity = "_parity";

#if 0
string
NameComponents::getUserPrefix(const string& username,
                              const string& prefix)
{
    return *NdnRtcNamespace::getProducerPrefix(prefix, username);
}

string
NameComponents::getStreamPrefix(const string& streamName,
                                const string& username,
                                const string& prefix)
{
    return *NdnRtcNamespace::getStreamPath(prefix, username, streamName);
}

string
NameComponents::getThreadPrefix(const string& threadName,
                                const string& streamName,
                                const string& username,
                                const string& prefix)
{
    return NdnRtcNamespace::getThreadPrefix(*NdnRtcNamespace::getStreamPath(prefix, username, streamName), threadName);
}

string 
NameComponents::getUserName(const string& prefix)
{
    size_t userComp = prefix.find(NameComponents::NameComponentUser);
    
    if (userComp != string::npos)
    {
        size_t startPos = userComp+NameComponents::NameComponentUser.size()+1;
        if (prefix.size() >= startPos)
        {
            size_t endPos = prefix.find("/", startPos);

            if (endPos == string::npos)
                endPos = prefix.size();
            return prefix.substr(startPos, endPos-startPos);
        }
    }
        
    return "";
}

string 
NameComponents::getStreamName(const string& prefix)
{
    size_t userComp = prefix.find(NameComponents::NameComponentUserStreams);
    
    if (userComp != string::npos)
    {
        size_t startPos = userComp+NameComponents::NameComponentUserStreams.size()+1;
        if (prefix.size() >= startPos)
        {
            size_t endPos = prefix.find("/", startPos);

            if (endPos == string::npos)
                endPos = prefix.size();
            return prefix.substr(startPos, endPos-startPos);
        }
    }

#if 0 // this code if for updated namespace
	string userName = NameComponents::getUserName(prefix);

	if (userName == "") return "";

    size_t p = prefix.find(userName);
    size_t startPos = p+userName.size()+1;
    
    if (prefix.size() >= startPos)
    {
        size_t endPos = prefix.find("/", startPos);

        if (endPos == string::npos)
            endPos = prefix.size();
        return prefix.substr(startPos, endPos-startPos);
    }
#endif
    return "";
}

string 
NameComponents::getThreadName(const string& prefix)
{
	string streamName = NameComponents::getStreamName(prefix);

	if (streamName == "") return "";

    size_t p = prefix.find(streamName);
    size_t startPos = p+streamName.size()+1;

    if (prefix.size() >= startPos)
    {
        size_t endPos = prefix.find("/", startPos);

        if (endPos == string::npos)
            endPos = prefix.size();
        return prefix.substr(startPos, endPos-startPos);
    }
        
    return "";
}
#endif

//******************************************************************************
vector<string> ndnrtcVersionComponents()
{
    string version(PACKAGE_VERSION);
    std::vector<string> components;
    
    boost::split(components, version, boost::is_any_of("."), boost::token_compress_on);
    
    return components;
}

unsigned int
NameComponents::nameApiVersion()
{
    return (unsigned int)atoi(ndnrtcVersionComponents().front().c_str());
}

Name
NameComponents::ndnrtcSuffix()
{
    return Name(NameComponentApp).appendVersion(nameApiVersion());
}

Name
NameComponents::streamPrefix(MediaStreamParams::MediaStreamType type, std::string basePrefix)
{
    Name n = Name(basePrefix);
    n.append(ndnrtcSuffix());
    return ((type == MediaStreamParams::MediaStreamType::MediaStreamTypeAudio) ? 
        n.append(NameComponentAudio) : n.append(NameComponentVideo));
}

Name
NameComponents::audioStreamPrefix(string basePrefix)
{
    return streamPrefix(MediaStreamParams::MediaStreamType::MediaStreamTypeAudio, basePrefix);
}

Name
NameComponents::videoStreamPrefix(string basePrefix)
{
    return streamPrefix(MediaStreamParams::MediaStreamType::MediaStreamTypeVideo, basePrefix);
}

//******************************************************************************
bool extractMeta(const ndn::Name& name, NamespaceInfo& info)
{
    if (name.size() >= 2 && name[0].isVersion())
    {
        info.metaVersion_ = name[0].toVersion();
        info.segNo_ = name[1].toSegment();
        return true;
    }

    return false;
}

bool extractVideoStreamInfo(const ndn::Name& name, NamespaceInfo& info)
{
    if (name.size() < 4)
        return false;

    info.streamName_ = name[0].toEscapedString();
    info.isMeta_ = (name[1] == Name::Component(NameComponents::NameComponentMeta));
    
    if (info.isMeta_)
    {
        info.threadName_ = "";
        return extractMeta(name.getSubName(2), info);
    }
    else
    {
        info.threadName_ = name[1].toEscapedString();
        info.isMeta_ = (name[2] == Name::Component(NameComponents::NameComponentMeta));

        if (info.isMeta_ && extractMeta(name.getSubName(3), info))
            return true;

        if (name[2] == Name::Component(NameComponents::NameComponentDelta) || 
            name[2] == Name::Component(NameComponents::NameComponentKey))
        {
            info.isDelta_ = (name[2] == Name::Component(NameComponents::NameComponentDelta));

            try{
                if (name.size() > 3)
                    info.sampleNo_ = (PacketNumber)name[3].toSequenceNumber();
            
                if (name.size() > 4)
                {
                    info.isParity_ = (name[4] == Name::Component(NameComponents::NameComponentParity));
                    if (info.isParity_ && name.size() > 5)
                    {
                        info.segNo_ = name[5].toSegment();
                        return true;
                    }
                    else 
                    {
                        if (info.isParity_) 
                            return false;
                        else
                            info.segNo_ = name[4].toSegment();
                        return true;
                    }
                }
            }
            catch (std::runtime_error& e)
            {
                return false;
            }
        }
    }

    return false;
}

bool extractAudioStreamInfo(const ndn::Name& name, NamespaceInfo& info)
{
    if (name.size() < 4)
        return false;

    info.streamName_ = name[0].toEscapedString();
    info.isMeta_ = (name[1] == Name::Component(NameComponents::NameComponentMeta));
    
    if (info.isMeta_)
    {
        info.threadName_ = "";
        return extractMeta(name.getSubName(2), info);;
    }
    else
    {
        info.threadName_ = name[1].toEscapedString();
        info.isMeta_ = (name[2] == Name::Component(NameComponents::NameComponentMeta));

        if (info.isMeta_ && extractMeta(name.getSubName(3), info))
            return true;

        info.isDelta_ = true;

        try
        {
            if (name.size() > 2)
                info.sampleNo_ = (PacketNumber)name[2].toSequenceNumber();
                
            if (name.size() > 3)
            {
                info.segNo_ = name[3].toSegment();
                return true;
            }
        }
        catch (std::runtime_error& e)
        {
            return false;
        }
    }

    return false;
}

bool
NameComponents::extractInfo(const ndn::Name& name, NamespaceInfo& info)
{
    bool goodName = false;
    static Name ndnrtcSubName(NameComponents::NameComponentApp);
    Name subName;
    int i;

    for (i = name.size()-2; i > 0 && !goodName; --i)
    {
        subName = name.getSubName(i);
        goodName = ndnrtcSubName.match(subName);
    }

    if (goodName)
    {
        info.basePrefix_ = name.getSubName(0, i+1);

        if ((goodName = subName[1].isVersion()))
        {
            info.apiVersion_ = subName[1].toVersion();

            if (subName.size() > 2 &&
                (goodName = (subName[2] == Name::Component(NameComponents::NameComponentAudio) ||
                            subName[2] == Name::Component(NameComponents::NameComponentVideo)))  )
            {
                info.streamType_ = (subName[2] == Name::Component(NameComponents::NameComponentAudio) ? 
                                MediaStreamParams::MediaStreamType::MediaStreamTypeAudio : 
                                MediaStreamParams::MediaStreamType::MediaStreamTypeVideo );

                if (info.streamType_ == MediaStreamParams::MediaStreamType::MediaStreamTypeAudio)
                    return extractAudioStreamInfo(subName.getSubName(3), info);
                else
                    return extractVideoStreamInfo(subName.getSubName(3), info);
            }
        }
    }

    return false;
}
