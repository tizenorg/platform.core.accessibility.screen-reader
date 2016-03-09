#ifndef SCREEN_READER_SPI_H_
#define SCREEN_READER_SPI_H_

#include <atspi/atspi.h>
#include "screen_reader.h"

void spi_init(Service_Data *sd);
void spi_event_listener_cb(AtspiEvent *event, void *user_data);
char *spi_event_get_text_to_read(AtspiEvent *event, void *user_data);

#endif /* SCREEN_READER_SPI_H_ */
