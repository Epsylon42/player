#pragma once

#include "data.h"
#include "decode.h"

#include <ao/ao.h>

void play(Track* track);
void playPacket(AVPacket* packet, ao_device* device, Track* track);
