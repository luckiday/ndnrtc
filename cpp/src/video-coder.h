//
//  video-coder.h
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//  Created: 8/21/13
//

#ifndef __ndnrtc__video_coder__
#define __ndnrtc__video_coder__

#include "ndnrtc-common.h"
#include "camera-capturer.h"
#include "statistics.h"

namespace ndnrtc {
    
    namespace new_api {
        class IEncodedFrameConsumer;
        
        class VideoCoderStatistics : public ObjectStatistics {
        public:
            unsigned int nDroppedByEncoder;
        };
        
        /**
         * This class is a main wrapper for VP8 WebRTC encoder. It consumes raw
         * frames, encodes them using VP8 encoder, configured for specified
         * parameters and passes encoded frames to its' frame consumer class.
         */
        class VideoCoder :  public NdnRtcComponent,
                            public IRawFrameConsumer,
                            public webrtc::EncodedImageCallback
        {
        public:
            VideoCoder();
            ~VideoCoder();
            
            void setFrameConsumer(IEncodedFrameConsumer *frameConsumer) {
                frameConsumer_ = frameConsumer;
            }
            
            int init(const VideoCoderParams& settings);
            
            // interface conformance - webrtc::EncodedImageCallback
            int32_t Encoded(webrtc::EncodedImage& encodedImage,
                            const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                            const webrtc::RTPFragmentationHeader* fragmentation = NULL);
            
            // interface conformance - ndnrtc::IRawFrameConsumer
            void onDeliverFrame(webrtc::I420VideoFrame &frame,
                                double timestamp);
            
            void
            getStatistics(VideoCoderStatistics& statistics)
            { statistics.nDroppedByEncoder = nDroppedByEncoder_; }

            void
            getSettings(VideoCoderParams& settings)
            { settings = settings_; }
            
            static int getCodecFromSetings(const VideoCoderParams &settings,
                                           webrtc::VideoCodec &codec);
            
        private:
            VideoCoderParams settings_;
            unsigned int counter_ = 1;
            unsigned int nDroppedByEncoder_ = 0;
            unsigned int rateMeter_;
            int keyFrameCounter_ = 0;
            
            double deliveredTimestamp_;
            IEncodedFrameConsumer *frameConsumer_ = NULL;
            
            webrtc::VideoCodec codec_;
            std::vector<webrtc::VideoFrameType> keyFrameType_;
            boost::shared_ptr<webrtc::VideoEncoder> encoder_;
            
            webrtc::Scaler frameScaler_;
            webrtc::I420VideoFrame scaledFrame_;
            
            void
            initScaledFrame();
        };
        
        class IEncodedFrameConsumer
        {
        public:
            virtual void
            onEncodedFrameDelivered(const webrtc::EncodedImage &encodedImage,
                                    double captureTimestamp) = 0;
        };
    }
    
    class IEncodedFrameConsumer;
    
    /**
     * This class is a main wrapper for VP8 WebRTC encoder. It consumes raw
     * frames, encodes them using VP8 encoder, configured for specified
     * parameters and passes encoded frames to its' frame consumer class.
     */
    class NdnVideoCoder : public NdnRtcObject, public IRawFrameConsumer,
    public webrtc::EncodedImageCallback
    {
    public:
        NdnVideoCoder(const CodecParams& codecParams) DEPRECATED;//const ParamsStruct &params);
        ~NdnVideoCoder();
        
        void setFrameConsumer(IEncodedFrameConsumer *frameConsumer) {
            frameConsumer_ = frameConsumer;
        }
        
        int init();
        
        // interface conformance - webrtc::EncodedImageCallback
        int32_t Encoded(webrtc::EncodedImage& encodedImage,
                        const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                        const webrtc::RTPFragmentationHeader* fragmentation = NULL);
        
        // interface conformance - ndnrtc::IRawFrameConsumer
        void onDeliverFrame(webrtc::I420VideoFrame &frame, double timestamp);
        
        unsigned int getDroppedFramesNum() { return nDroppedByEncoder_; };
        static int getCodec(const CodecParams &params, webrtc::VideoCodec &codec);
        
    private:
        CodecParams codecParams_;
        unsigned int counter_ = 1;
        unsigned int nDroppedByEncoder_ = 0;
        
        int keyFrameCounter_ = 0;
        int currentFrameRate_;
        double deliveredTimestamp_;
        std::vector<webrtc::VideoFrameType> keyFrameType_;
        // private attributes go here
        IEncodedFrameConsumer *frameConsumer_ = nullptr;
        webrtc::VideoCodec codec_;
        boost::shared_ptr<webrtc::VideoEncoder> encoder_;
        
        webrtc::Scaler frameScaler_;
        webrtc::I420VideoFrame scaledFrame_;
        
        void
        initScaledFrame();
    };
    
    class IEncodedFrameConsumer
    {
    public:
        virtual void
        onEncodedFrameDelivered(const webrtc::EncodedImage &encodedImage,
                                double captureTimestamp) = 0;
    };
}

#endif /* defined(__ndnrtc__video_coder__) */
