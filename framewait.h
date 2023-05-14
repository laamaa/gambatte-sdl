#ifndef FRAMEWAIT_H_
#define FRAMEWAIT_H_

#include "adaptivesleep.h"

class FrameWait {
public:
	FrameWait() : last_() {}

	void waitForNextFrameTime(usec_t frametime) {
		last_ += asleep_.sleepUntil(last_, frametime);
		last_ += frametime;
	}

private:
	AdaptiveSleep asleep_;
	usec_t last_;
};

#endif