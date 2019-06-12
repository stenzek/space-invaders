#pragma once
#include "common/simple_audio.h"
#include <cstdint>

class SDLSimpleAudio final : public SimpleAudio
{
public:
  SDLSimpleAudio();
  ~SDLSimpleAudio();

protected:
  bool OpenDevice() override;
  void PauseDevice(bool paused) override;
  void CloseDevice() override;

  static void AudioCallback(void* userdata, uint8_t* stream, int len);

  bool m_is_open = false;
};
