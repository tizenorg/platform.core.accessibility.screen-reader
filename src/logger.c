#include "logger.h"

int _eina_log_dom;

#define SCREEN_READER_LOG_DOMAIN_NAME "screen-reader"

int logger_init(void)
{
   eina_init();

   if (!_eina_log_dom)
      {
         _eina_log_dom = eina_log_domain_register(SCREEN_READER_LOG_DOMAIN_NAME, NULL);
         if (_eina_log_dom  < 0)
            {
               fprintf(stderr, "Unable to register screen-reader log domain");
               return -1;
            }
         eina_log_domain_level_set(SCREEN_READER_LOG_DOMAIN_NAME, EINA_LOG_LEVEL_DBG);
      }
   return 0;
}

void logger_shutdown(void)
{
   eina_shutdown();

   if (_eina_log_dom)
      {
         eina_log_domain_unregister(_eina_log_dom);
         _eina_log_dom = 0;
      }
}

