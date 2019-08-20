//
// stream.hpp
//
//  Created by Peter Gusev on 19 July 2016.
//  Copyright 2013-2019 Regents of the University of California
//

#ifndef __stream_h__
#define __stream_h__

#include "statistics.hpp"
#include "simple-log.hpp"
#include "ndnrtc-common.hpp"

namespace ndn {
	class Data;
	class KeyChain;
	class MemoryContentCache;
}

namespace ndnrtc {
	namespace statistics {
		class StatisticsStorage;
	}
    class StorageEngine;

    /**
     * Stream interface class used as a base class for remote and local streams.
     * Defines trivial methods common for both types of streams.
     */
	class IStream {
	public:
        /**
         * Returns base prefix for the stream
         */
		virtual std::string getBasePrefix() const = 0;

        /**
         * Returns stream name
         */
		virtual std::string getStreamName() const = 0;

        /**
         * Returns full stream for the stream used for fetching data
         */
        virtual std::string getPrefix() const = 0;

        /**
         * Returns statistics storage for the current stream.
         * Each stream has internal statistics storage with counters for
         * various statistics. This call is non-blocking - user may invoke
         * this call at regular intervals to query running statistics.
         */
		virtual statistics::StatisticsStorage getStatistics() const = 0;

        /**
         * Sets logger for this stream. By default, logger is nil - no logging is
         * performed.
         */
		virtual void setLogger(std::shared_ptr<ndnlog::new_api::Logger> logger) = 0;

        /**
         * Returns stream's storage, if it was set up.
         */
        virtual std::shared_ptr<StorageEngine> getStorage() const = 0;

        virtual ~IStream() {}
	};

	// TODO: rename after refactor finished
	class VideoStreamImpl2;

	class VideoStream : public IStream {
	public:
		typedef struct _Settings {
			size_t segmentSize_;
			std::shared_ptr<ndn::MemoryContentCache> memCache_;
			bool useFec_, storeInMemCache_;
			CodecSettings codecSettings_;
		} Settings;

		VideoStream(std::string basePrefix, std::string streamName,
					Settings settings,
					std::shared_ptr<ndn::KeyChain> keyChain);
		~VideoStream();

		std::vector<std::shared_ptr<ndn::Data>> processImage(const ImageFormat& fmt, uint8_t *imageData);

		std::string getBasePrefix() const;
		std::string getStreamName() const;
		std::string getPrefix() const;

        uint64_t getSeqNo() const;
        uint64_t getGopNo() const;
        uint64_t getGopPos() const;

		statistics::StatisticsStorage getStatistics() const;
		void setLogger(std::shared_ptr<ndnlog::new_api::Logger> logger);
		std::shared_ptr<StorageEngine> getStorage() const;

		static Settings& defaultSettings();

	private:
		std::shared_ptr<VideoStreamImpl2> pimpl_;
	};
}

#endif
