#ifndef NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
#define NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H

#include <string>

#include "asset.h"

class IAudioPlayer
{
public:
  virtual ~IAudioPlayer() = default;

  virtual bool load(const std::string &path) = 0;
  virtual void play() = 0;
  virtual void stop() = 0;

  virtual double getPositionMs() const = 0;     // posizione corrente in ms
  virtual double getBpm() const { return 0.0; } // se non disponibile, 0
  virtual int getRow() const { return -1; }     // valido solo se supportato
  virtual int getPattern() const { return -1; } // idem

protected:
  Asset asset;
};

#endif // NAG_ENGINE_INTERFACE_AUDIO_PLAYER_H
