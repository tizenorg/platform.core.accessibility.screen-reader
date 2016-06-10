#include <atspi/atspi.h>
#include "screen_reader.h"

#define TTS_MAX_TEXT_SIZE  2000

extern bool read_description;
extern bool haptic;

void navigator_init(void);
void navigator_shutdown(void);
