#ifndef LOGGER_H_
#define LOGGER_H_

#define TIZEN_ENGINEER_MODE
#include <libintl.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "SCREEN-READER"

#define INFO(format, arg...) LOGI(format, ##arg)
#define DEBUG(format, arg...) LOGD(format, ##arg)
#define ERROR(format, arg...) LOGE(format, ##arg)
#define WARNING(format, arg...) LOGW(format, ##arg)

#define MEMORY_ERROR "Memory allocation error"

#ifdef _
#undef _
#endif

#define _(str) (gettext(str))

#endif /* end of include guard: LOGGER_H_ */
