//
// main.cpp
//
//  Created by Peter Gusev on 26 February 2019.
//  Copyright 2013-2019 Regents of the University of California
//

#include <iostream>
#include <iomanip>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread/mutex.hpp>
// #include <boost/thread.hpp>

#include "../../contrib/docopt/docopt.h"
#include "../../include/simple-log.hpp"
#include "../../include/stream.hpp"
#include "../../src/video-codec.hpp"

#include "fetch.hpp"
#include "publish.hpp"

// **** tools
#include <vpx/vpx_encoder.h>
// ****

#define TOOL_NAME "Codec Run"

static const char USAGE[] =
R"(NdnRtc Stream.
    This is a headless NDN-RTC client app that allows you to publish and fetch
    arbitrary videos over NDN-RTC.
    NDN-RTC is for real-time low-latency communication. If you're looking for
    VoD solutions, look elsewhere.
    This application utilizes NDN-RTC library and can be used as an example app
    to learn how to use NDN-RTC library.
    It was never intended to be built as a user-facing app. It is for demo and
    hack purposes only. You may re-use the source code and build on top of it,
    but don't ask for new features, implement them yourself.
    Generally, the stream discovery problem is not part of NDN-RTC and it will
    never be. It is up to an application how discovery and signalling may be
    implemented (and there are a bunch of ways, e.g. ChronoSync, PSync, etc).
    However, this app implements an optional "rendez-vous" mechanism IF it was
    compiled with cnl-cpp library (https://github.com/named-data/cnl-cpp).
    App takes raw frames as input (from stdin, files or file pipes) when in
    publishing mode.
    When in fetching mode, it outputs raw frames (to stdout, files or pipes).
    If you want to view and playback these files, use ffmpeg and ffplay tools.
    See the usage examples below for more info.

    Usage:
      ndnrtc-stream publish <base_prefix> <stream_name> --input=<in_file> --size=<WxH> --signing-identity=<identity> [--bitrate=<bitrate>] [--gop=<gop>] [--fps=<fps>] [--no-drop] [--use-fec] [--i420] [--segment-size=<seg_size>] [--rvp] [--loop] [(--v | --vv | --vvv)] [--log=<file>]
      ndnrtc-stream fetch ( <stream_prefix> | ( <base_prefix> --rvp )) --output=<out_file> [--use-fec] [(--v | --vv | --vvv)] [--log=<file>]

    Arguments:
      <base_prefix>     Base prefix used to form stream prefix from (see NDN-RTC namespace).
      <stream_name>     Stream name that will be used in stream prefix.
      <stream_prefix>   Full stream prefix of NDN-RTC stream to fetch from. This
                        is normally your output from "ndnrtc-stream publish ..."
                        command.

    Options:
      -i --input=<in_file>      Input raw video file (YUV 420 format by default).
      -o --output=<out_file>    Output raw video file (YUV 420 format by default).
      --signing-identity=<ss>   Signing identity prefix that will be used to create
                                an instance identity for signing packets (make
                                sure you created one with ndnsec-list command).
      --segment-size=<n_bytes>  A size of a frame segment (in bytes) used when
                                segmenting frames for publishing (see NDN-RTC
                                namespace) [default: 8000].
      --rvp                     When this option is present (AND app was compiled
                                with cnl-cpp), <base_prefix> will be used for setting
                                up rendez-vous point for multiple app instances
                                to discover currently published streams.
      --loop                    Indicates whether source must be looped.
      --size=<WxH>              Size of incoming video frame; must be in "WIDTHxHEIGHT" format
      --bitrate=<bitrate>       Target encoding bitrate in kbps [default: 3000]
      --gop=<gop>               Target group of picture size inframes [default: 30]
      --fps=<fps>               Target FPS [default: 30]
      --no-drop                 Tells encoder not to drop frames
      --use-fec                 Use Forward Error Correction data
      --i420                    I420 raw frame format
      --v                       Verbose mode: debug
      --vv                      Verbose mode: trace
      --vvv                     Verbose mode: all
      --log=<filename>          Log file, by default logs to stdout [default: ]
)";

using namespace std;
using namespace ndnrtc;

static bool mustExit = false;
std::string AppLog = "";

void handler(int sig)
{
    void *array[10];
    size_t size;

    if (sig == SIGABRT || sig == SIGSEGV)
    {
        fprintf(stderr, "Received signal %d:\n", sig);
        // get void*'s for all entries on the stack
        size = backtrace(array, 10);
        // print out all the frames to stderr
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        exit(1);
    }
    else
        mustExit = true;
}

int getSize(string sizeStr, uint16_t& w, uint16_t& h);
void runEncoder(VideoCodec& codec, string inFile, string outFile);
void runDecoder(VideoCodec& codec, string inFile, string outFile);

int main(int argc, char **argv)
{
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    signal(SIGINT, handler);
    signal(SIGUSR1, handler);

    ndnlog::new_api::Logger::initAsyncLogging();

    std::map<std::string, docopt::value> args
        = docopt::docopt(USAGE,
                         { argv + 1, argv + argc },
                         true,               // show help if requested
                         (string(TOOL_NAME)+string(PACKAGE_VERSION)).c_str());  // version string

    // for(auto const& arg : args)
    //     std::cout << arg.first << " " <<  arg.second << std::endl;

    AppLog = args["--log"].asString();

    if (args["--v"].asBool())
        ndnlog::new_api::Logger::getLogger(AppLog).setLogLevel(ndnlog::NdnLoggerDetailLevelDebug);
    else if (args["--vv"].asBool())
        ndnlog::new_api::Logger::getLogger(AppLog).setLogLevel(ndnlog::NdnLoggerDetailLevelAll);
    else if (args["--vvv"].asBool())
           ndnlog::new_api::Logger::getLogger(AppLog).setLogLevel(ndnlog::NdnLoggerDetailLevelAll);

    try
    {
        if (args["publish"].asBool())
        {
            LogDebug(AppLog) << "initializing publisher" << endl;
            VideoStream::Settings streamSettings;

            streamSettings.codecSettings_ = VideoCodec::defaultCodecSettings();

            if (getSize(args["--size"].asString(),
                        streamSettings.codecSettings_.spec_.encoder_.width_,
                        streamSettings.codecSettings_.spec_.encoder_.height_))
            {
                streamSettings.segmentSize_ = args["--segment-size"].asLong();
                streamSettings.useFec_ = args["--use-fec"].asBool();
                streamSettings.storeInMemCache_ = true;
                // TODO: add memcache
                //streamSettings.memCache_ = ;
                streamSettings.codecSettings_.spec_.encoder_.bitrate_ = args["--bitrate"].asLong();
                streamSettings.codecSettings_.spec_.encoder_.gop_ = args["--gop"].asLong();
                streamSettings.codecSettings_.spec_.encoder_.fps_ = args["--fps"].asLong();
                streamSettings.codecSettings_.spec_.encoder_.dropFrames_ = !args["--no-drop"].asBool();

                LogDebug(AppLog) << "publish settings:"
                             << "\n\tsegment size " << streamSettings.segmentSize_
                             << "\n\tcodec:"
                             << "\n\t\twidth " << streamSettings.codecSettings_.spec_.encoder_.width_
                             << " height " << streamSettings.codecSettings_.spec_.encoder_.height_
                             << "\n\t\tbitrate " << streamSettings.codecSettings_.spec_.encoder_.bitrate_
                             << "\n\t\tgop " << streamSettings.codecSettings_.spec_.encoder_.gop_
                             << " fps " << streamSettings.codecSettings_.spec_.encoder_.fps_
                             << " drop frames " << streamSettings.codecSettings_.spec_.encoder_.dropFrames_
                             << endl;

                runPublisher(args["--input"].asString(),
                             args["<base_prefix>"].asString(),
                             args["<stream_name>"].asString(),
                             args["--signing-identity"].asString(),
                             streamSettings,
                             args["--rvp"].asBool(),
                             args["--loop"].asBool());
            }
            else
            {
                LogError(AppLog) << "bad frame size specified: " << args["--size"] << endl;
            }

        }
        else if (args["fetch"].asBool())
        {
            LogDebug(AppLog) << "initializing fetching" << endl;
        }
    }
    catch (std::exception &e)
    {
        LogError(AppLog) << "caught exception: " << e.what() << endl;
    }

    LogInfo(AppLog) << "shutting down gracefully...	ʕノ•ᴥ•ʔノ" << endl;
    LogInfo(AppLog) << "done" << endl;
}

int getSize(string sizeStr, uint16_t& w, uint16_t& h)
{
    istringstream ss(sizeStr);
    string k;
    int idx = 0;

    while(getline(ss, k, 'x') && idx < 2)
    {
        int val;

        try {
            val = stoi(k);
            if (val > 0)
            {
                if (idx == 0)
                    w = val;
                else
                    h = val;
                idx++;
            }
        }
        catch(std::exception&)
        {
            break;
        }
    }

    return (idx == 2);
}