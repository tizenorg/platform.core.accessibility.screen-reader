/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

