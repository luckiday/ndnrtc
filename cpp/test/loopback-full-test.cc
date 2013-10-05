//
//  full-loppback-test.cc
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#define NDN_LOGGING
#define NDN_INFO
#define NDN_WARN
#define NDN_ERROR
#define NDN_TRACE

#include "test-common.h"
#include "sender-channel.h"
#include "receiver-channel.h"

using namespace ndnrtc;

TEST(ReceiverChannelParams, CreateDelete)
{
    NdnParams *p = ReceiverChannelParams::defaultParams();
    delete p;
}
TEST(ReceiverChannelParams, TestParams)
{
    NdnParams *p = ReceiverChannelParams::defaultParams();
    
    
    { // check renrerer params
        int width, height;
        NdnRendererParams *ccp = static_cast<NdnRendererParams*>(p);
        ASSERT_NE(ccp, nullptr);
        
        EXPECT_EQ(0, ccp->getWindowWidth(&width));
        EXPECT_EQ(0, ccp->getWindowHeight(&height));
    }
    
    {// check video decoder params
        int width, height, maxBitRate, startBitRate, frameRate;
        NdnVideoCoderParams *ccp = static_cast<NdnVideoCoderParams*>(p);
        ASSERT_NE(ccp, nullptr);
        
        EXPECT_EQ(0, ccp->getFrameRate(&frameRate));
        EXPECT_EQ(0, ccp->getMaxBitRate(&maxBitRate));
        EXPECT_EQ(0, ccp->getStartBitRate(&startBitRate));
        EXPECT_EQ(0, ccp->getWidth(&width));
        EXPECT_EQ(0, ccp->getHeight(&height));
    }
    
    {// check video sender params
        VideoSenderParams *ccp = static_cast<VideoSenderParams*>(p);
        ASSERT_NE(ccp, nullptr);
        
        int a;
        char *str = (char*)malloc(256);
        
        EXPECT_EQ(0, ccp->getHub(&str));
        EXPECT_EQ(0, ccp->getProducerId(&str));
        EXPECT_EQ(0, ccp->getStreamName(&str));
        EXPECT_EQ(0, ccp->getSegmentSize(&a));
        EXPECT_EQ(0, ccp->getFreshnessInterval(&a));
        
        free(str);
    }
    
    delete p;
}

TEST(LoopbackTests, Transmission)
{
    SenderChannelParams *sp = SenderChannelParams::defaultParams();
    ReceiverChannelParams *rp = ReceiverChannelParams::defaultParams();
    
    sp->setIntParam(CameraCapturerParams::ParamNameDeviceId, 1);
    sp->setIntParam(VideoSenderParams::ParamNameFrameFreshnessInterval, 10);
    
    NdnSenderChannel *sc = new NdnSenderChannel(sp);
    NdnReceiverChannel *rc = new NdnReceiverChannel(rp);
    
    EXPECT_EQ(0, sc->init());
    EXPECT_EQ(0, rc->init());
    
    sc->startTransmission();
    WAIT(200);
    rc->startFetching();
    
    bool f=  false;
    EXPECT_TRUE_WAIT(f, 1000000);
    
    
    sc->stopTransmission();
//    rc->stopFetching();
    
    delete sc;
    delete rc;
    
    delete sp;
    delete rp;
}
