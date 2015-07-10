#ifndef LOGGER_H_
#define LOGGER_H_

#include <Eina.h>

extern int _eina_log_dom;

int logger_init(void);
void logger_shutdown(void);

#define INFO(...) EINA_LOG_DOM_INFO(_eina_log_dom, __VA_ARGS__);
#define DEBUG(...) EINA_LOG_DOM_DBG(_eina_log_dom, __VA_ARGS__);
#define ERROR(...) EINA_LOG_DOM_ERR(_eina_log_dom, __VA_ARGS__);
#define WARNING(...) EINA_LOG_DOM_WARN(_eina_log_dom, __VA_ARGS__);

#define MEMORY_ERROR "Memory allocation error"

#ifdef _
#undef _
#endif

#define _(str) gettext(str)

#endif /* end of include guard: LOGGER_H_ */
