#ifndef AUDIO_OUT_H_
#define AUDIO_OUT_H_

#include "audiosink.h"
#include "resample/resampler.h"
#include "resample/resamplerinfo.h"

class AudioOut {
public:
	struct Status {
		long rate;
		bool low;

		Status(long rate, bool low) : rate(rate), low(low) {}
	};

	AudioOut(long sampleRate, int latency, int periods,
	         ResamplerInfo const &resamplerInfo, std::size_t maxInSamplesPerWrite)
	: resampler_(resamplerInfo.create(2097152, sampleRate, maxInSamplesPerWrite))
	, resampleBuf_(resampler_->maxOut(maxInSamplesPerWrite) * 2)
	, sink_(sampleRate, latency, periods)
	{
	}

	Status write(Uint32 const *data, std::size_t samples) {
		long const outsamples = resampler_->resample(
			resampleBuf_, reinterpret_cast<Sint16 const *>(data), samples);
		AudioSink::Status const &stat = sink_.write(resampleBuf_, outsamples);
		bool low = stat.fromUnderrun + outsamples < (stat.fromOverflow - outsamples) * 2;
		return Status(stat.rate, low);
	}

private:
	scoped_ptr<Resampler> const resampler_;
	Array<Sint16> const resampleBuf_;
	AudioSink sink_;
};
#endif