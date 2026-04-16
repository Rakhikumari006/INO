#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "config.h"
#include "all_frames.h"

enum Emotion {
  IDLE_NEUTRAL = 0,
  HAPPY = 1,
  SLEEPY = 2,
  SURPRISED = 3,
  BLINK = 4,
  CONFUSED = 5
};

enum AttentionDirection {
  LOOK_CENTER = 0,
  LOOK_LEFT = -1,
  LOOK_RIGHT = 1
};

struct EmotionProfile {
  int startFrame;
  int endFrame;
  int baseDelay;
  bool loop;
  bool transient;
};

class AnimationEngine {
private:
  Emotion activeEmotion = IDLE_NEUTRAL;
  Emotion returnEmotion = IDLE_NEUTRAL;
  int currentFrame = FRAME_IDLE_START;
  int frameStart = FRAME_IDLE_START;
  int frameEnd = FRAME_IDLE_END;
  int baseDelay = IDLE_BASE_SPEED;
  bool looping = true;
  bool transient = false;
  unsigned long nextFrameAt = 0;
  unsigned long pauseUntil = 0;
  unsigned long nextBlinkAt = 0;
  unsigned long nextLookAt = 0;
  int frameStride = 1;
  int attentionBias = 0;
  bool cycleCompleted = false;

  EmotionProfile profileFor(Emotion emotion) {
    switch (emotion) {
      case HAPPY:
        return {FRAME_HAPPY_START, FRAME_HAPPY_END, HAPPY_SPEED, true, false};
      case SLEEPY:
        return {FRAME_SLEEPY_START, FRAME_SLEEPY_END, SLEEPY_SPEED, true, false};
      case SURPRISED:
        return {FRAME_SURPRISED_START, FRAME_SURPRISED_END, SURPRISED_SPEED, false, true};
      case BLINK:
        return {FRAME_BLINK_START, FRAME_BLINK_END, BLINK_SPEED, false, true};
      case CONFUSED:
        return {FRAME_CONFUSED_START, FRAME_CONFUSED_END, CONFUSED_SPEED, true, false};
      case IDLE_NEUTRAL:
      default:
        return {FRAME_IDLE_START, FRAME_IDLE_END, IDLE_BASE_SPEED, true, false};
    }
  }

  void scheduleIdleLife(unsigned long now) {
    nextBlinkAt = now + random(BLINK_MIN_INTERVAL, BLINK_MAX_INTERVAL + 1);
    nextLookAt = now + random(LOOK_MIN_INTERVAL, LOOK_MAX_INTERVAL + 1);
  }

  int clampFrame(int value) const {
    if (value < frameStart) return frameStart;
    if (value > frameEnd) return frameEnd;
    return value;
  }

  int currentDelay() const {
    int jitter = 0;
    if (activeEmotion == IDLE_NEUTRAL) {
      jitter = random(0, 18);
    } else if (activeEmotion == SLEEPY) {
      jitter = random(0, 24);
    } else if (activeEmotion == HAPPY) {
      jitter = random(0, 10);
    }
    return max(12, baseDelay + jitter);
  }

public:
  AnimationEngine() {
    setEmotion(IDLE_NEUTRAL, true);
  }

  void setEmotion(Emotion emotion, bool forceRestart = false) {
    if (!forceRestart && emotion == activeEmotion) {
      return;
    }

    if (emotion == BLINK && activeEmotion != IDLE_NEUTRAL) {
      returnEmotion = activeEmotion;
    } else if (emotion != IDLE_NEUTRAL && emotion != BLINK) {
      returnEmotion = emotion;
    }

    EmotionProfile profile = profileFor(emotion);
    activeEmotion = emotion;
    frameStart = profile.startFrame;
    frameEnd = profile.endFrame;
    baseDelay = profile.baseDelay;
    looping = profile.loop;
    transient = profile.transient;

    frameStride = (emotion == IDLE_NEUTRAL) ? random(1, 3) : 1;
    attentionBias = 0;

    if (emotion == IDLE_NEUTRAL) {
      currentFrame = frameStart + random(0, min(8, frameEnd - frameStart + 1));
    } else {
      currentFrame = frameStart;
    }

    unsigned long now = millis();
    nextFrameAt = now + random(0, 120);
    pauseUntil = 0;

    if (emotion == IDLE_NEUTRAL) {
      scheduleIdleLife(now);
    }

    cycleCompleted = false;
  }

  void setAttention(AttentionDirection direction) {
    attentionBias = (int)direction;
  }

  void randomMoodDrift() {
    if (activeEmotion == IDLE_NEUTRAL) {
      int roll = random(100);
      if (roll < 40) {
        setAttention(LOOK_LEFT);
      } else if (roll < 80) {
        setAttention(LOOK_RIGHT);
      } else {
        setAttention(LOOK_CENTER);
      }
    }
  }

  void triggerBlink(bool slowBlink = false) {
    returnEmotion = activeEmotion;
    setEmotion(BLINK, true);
    baseDelay = slowBlink ? BLINK_SPEED + 12 : BLINK_SPEED;
  }

  Emotion getEmotion() const {
    return activeEmotion;
  }

  const char* getEmotionName() const {
    switch (activeEmotion) {
      case HAPPY: return "happy";
      case SLEEPY: return "sleepy";
      case SURPRISED: return "surprised";
      case BLINK: return "blink";
      case CONFUSED: return "confused";
      case IDLE_NEUTRAL:
      default: return "idle";
    }
  }

  int getCurrentFrame() const {
    return clampFrame(currentFrame + attentionBias);
  }

  int getProgress() const {
    int range = frameEnd - frameStart;
    if (range <= 0) {
      return 0;
    }
    int progress = ((currentFrame - frameStart) * 100) / range;
    return constrain(progress, 0, 100);
  }

  bool update() {
    unsigned long now = millis();

    if (pauseUntil != 0 && now < pauseUntil) {
      return false;
    }

    if (activeEmotion == IDLE_NEUTRAL) {
      if (now >= nextBlinkAt) {
        triggerBlink(false);
        nextBlinkAt = now + random(BLINK_MIN_INTERVAL, BLINK_MAX_INTERVAL + 1);
        return true;
      }

      if (now >= nextLookAt) {
        randomMoodDrift();
        nextLookAt = now + random(LOOK_MIN_INTERVAL, LOOK_MAX_INTERVAL + 1);
        pauseUntil = now + random(IDLE_PAUSE_MIN, IDLE_PAUSE_MAX + 1);
        return false;
      }

      if (random(100) < 3) {
        pauseUntil = now + random(IDLE_PAUSE_MIN, IDLE_PAUSE_MAX + 1);
        return false;
      }
    }

    if (now < nextFrameAt) {
      return false;
    }

    nextFrameAt = now + currentDelay();
    currentFrame += frameStride;

    if (currentFrame > frameEnd) {
      if (looping) {
        cycleCompleted = true;
        currentFrame = frameStart + random(0, min(6, frameEnd - frameStart + 1));
      } else if (transient) {
        cycleCompleted = true;
        Emotion resumeEmotion = returnEmotion;
        setEmotion(resumeEmotion, true);
      } else {
        cycleCompleted = true;
        currentFrame = frameEnd;
      }
    }

    return true;
  }

  void reset() {
    setEmotion(activeEmotion, true);
  }

  bool consumeCycleCompleted() {
    bool completed = cycleCompleted;
    cycleCompleted = false;
    return completed;
  }
};

#endif // ANIMATIONS_H
