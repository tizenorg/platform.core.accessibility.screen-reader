#include "logger.h"

int _eina_log_dom;

int logger_init(void)
{
   if (!_eina_log_dom)
     {
        _eina_log_dom = eina_log_domain_register("screen-reader", NULL);
        if (_eina_log_dom  < 0)
          {
             fprintf(stderr, "Unable to register screen-reader log domain");
             return -1;
          }
     }
   return 0;
}

void logger_shutdown(void)
{
   if (_eina_log_dom)
     {
        eina_log_domain_unregister(_eina_log_dom);
        _eina_log_dom = 0;
     }
}

