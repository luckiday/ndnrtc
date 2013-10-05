//
//  video-coder.cpp
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev 
//  Created: 8/21/13
//

#undef DEBUG

#include "video-coder.h"

using namespace ndnrtc;
using namespace webrtc;

//********************************************************************************
//********************************************************************************
const std::string NdnVideoCoderParams::ParamNameFrameRate = "frameRate";
const std::string NdnVideoCoderParams::ParamNameStartBitRate = "startBitRate";
const std::string NdnVideoCoderParams::ParamNameMaxBitRate = "maxBitRate";
const std::string NdnVideoCoderParams::ParamNameWidth = "encodeWidth";
const std::string NdnVideoCoderParams::ParamNameHeight = "encodeHeight";

char* plotCodec(webrtc::VideoCodec codec)
{
    static char msg[256];
    
    sprintf(msg, "\t\tMax Framerate:\t%d\n \
         \tStart Bitrate:\t%d\n \
         \tMax Bitrate:\t%d\n \
         \tWidth:\t%d\n \
         \tHeight:\t%d\
         \0", codec.maxFramerate, codec.startBitrate, codec.maxBitrate,
         codec.width, codec.height);
    
    return msg;
}

VideoCodec NdnVideoCoderParams::getCodec()
{
    VideoCodec codec;
    // setup default params first
    if (!webrtc::VCMCodecDataBase::Codec(VCM_VP8_IDX, &codec))
    {
        TRACE("can't get deafult params");
        strncpy(codec.plName, "VP8", 31);
        codec.maxFramerate = 30;
        codec.startBitrate  = 300;
        codec.maxBitrate = 4000;
        codec.width = 640;
        codec.height = 480;
        codec.plType = VCM_VP8_PAYLOAD_TYPE;
        codec.qpMax = 56;
        codec.codecType = webrtc::kVideoCodecVP8;
        codec.codecSpecific.VP8.denoisingOn = true;
        codec.codecSpecific.VP8.complexity = webrtc::kComplexityNormal;
        codec.codecSpecific.VP8.numberOfTemporalLayers = 1;
    }
    // customize parameteres if possible
    getFrameRate((int*)&codec.maxFramerate);
    getStartBitRate((int*)&codec.startBitrate);
    getMaxBitRate((int*)&codec.maxBitrate);
    getWidth((int*)&codec.width);
    getHeight((int*)&codec.height);
    
    return codec;
}

//********************************************************************************
#pragma mark - construction/destruction
NdnVideoCoder::NdnVideoCoder(const NdnParams *params) : NdnRtcObject(params), frameConsumer_(nullptr)
{
    memset(&codec_, 0, sizeof(codec_));
}
//********************************************************************************
#pragma mark - public
int NdnVideoCoder::init()
{
    if (!hasParams())
        notifyErrorNoParams();

    codec_ = getParams()->getCodec();
    encoder_.reset(VP8Encoder::Create());
    
    if (!encoder_.get())
        return notifyError(-1, "can't create VP8 encoder");
    
    encoder_->RegisterEncodeCompleteCallback(this);
    
#warning how payload can be changed?
    int maxPayload = 3900;
    
    if (encoder_->InitEncode(&codec_, 1, maxPayload) != WEBRTC_VIDEO_CODEC_OK)
        return notifyError(-1, "can't initialize VP8 encoder");
    
    INFO("Initialized encoder with max payload %d and parameters: \n%s", maxPayload, plotCodec(codec_));
    
    return 0;
}
//********************************************************************************
#pragma mark - intefaces realization - EncodedImageCallback
int32_t NdnVideoCoder::Encoded(webrtc::EncodedImage& encodedImage,
                const webrtc::CodecSpecificInfo* codecSpecificInfo,
                const webrtc::RTPFragmentationHeader* fragmentation)
{
    TRACE("got encoded byte length: %d", encodedImage._length);
    
    if (frameConsumer_)
        frameConsumer_->onEncodedFrameDelivered(encodedImage);

#warning need to handle return value
    return 0;
}
//********************************************************************************
#pragma mark - intefaces realization - IRawFrameConsumer
void NdnVideoCoder::onDeliverFrame(webrtc::I420VideoFrame &frame)
{
    TRACE("encoding...");
    int err = encoder_->Encode(frame, NULL, NULL);
    
    if (err != WEBRTC_VIDEO_CODEC_OK)
        notifyError(-1, "can't encode frame due to error %d",err);
}

//********************************************************************************
#pragma mark - private

