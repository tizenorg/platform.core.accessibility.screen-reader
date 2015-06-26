#include "app_tracker.h"
#include "logger.h"

typedef struct
{
   AtspiAccessible *root;
   GList *callbacks;
   guint timer;
} SubTreeRootData;

typedef struct
{
   AppTrackerEventCB func;
   void *user_data;
} EventCallbackData;

#define APP_TRACKER_INVACTIVITY_TIMEOUT 200

static int _init_count;
static GList *_roots;
static AtspiEventListener *_listener;

static int
_is_descendant(AtspiAccessible *ancestor, AtspiAccessible *descendant)
{
   return 1;

#if 0
   do
      {
         if (ancestor == descendant) return 1;
      }
   while ((descendant = atspi_accessible_get_parent(descendant, NULL)) != NULL);

   return 0;
#endif
}

static void
_subtree_callbacks_call(SubTreeRootData *std)
{
   DEBUG("START");
   GList *l;
   EventCallbackData *ecd;

   for (l = std->callbacks; l != NULL; l = l->next)
      {
         ecd = l->data;
         ecd->func(ecd->user_data);
      }
   DEBUG("END");
}

static gboolean
_on_timeout_cb(gpointer user_data)
{
   DEBUG("START");
   SubTreeRootData *std = user_data;

   _subtree_callbacks_call(std);

   std->timer = 0;
   DEBUG("END");
   return FALSE;
}

static void
_on_atspi_event_cb(const AtspiEvent *event)
{
   DEBUG("START");
   GList *l;
   SubTreeRootData *std;

   if (!event) return;
   if (!event->source)
      {
         ERROR("empty event source");
         return;
      }

   if ((atspi_accessible_get_role(event->source, NULL) == ATSPI_ROLE_DESKTOP_FRAME) ||
         !strcmp(event->type, "object:children-changed"))
      {
         return;
      }

   DEBUG("signal:%s", event->type);

   for (l = _roots; l != NULL; l = l->next)
      {
         std = l->data;
         if (_is_descendant(std->root, event->source))
            {
               if (std->timer)
                  g_source_remove(std->timer);
               else
                  _subtree_callbacks_call(std);

               std->timer = g_timeout_add(APP_TRACKER_INVACTIVITY_TIMEOUT, _on_timeout_cb, std);
            }
      }
}

static int
_app_tracker_init_internal(void)
{
   DEBUG("START");
   _listener = atspi_event_listener_new_simple(_on_atspi_event_cb, NULL);

   atspi_event_listener_register(_listener, "object:state-changed:showing", NULL);
   atspi_event_listener_register(_listener, "object:state-changed:visible", NULL);
   atspi_event_listener_register(_listener, "object:children-changed", NULL);
   atspi_event_listener_register(_listener, "object:bounds-changed", NULL);
   atspi_event_listener_register(_listener, "object:visible-data-changed", NULL);

   return 0;
}

static void
_free_callbacks(gpointer data)
{
   g_free(data);
}

static void
_free_rootdata(gpointer data)
{
   SubTreeRootData *std = data;
   g_list_free_full(std->callbacks, _free_callbacks);
   if (std->timer)
      g_source_remove(std->timer);
   g_free(std);
}

static void
_app_tracker_shutdown_internal(void)
{
   atspi_event_listener_deregister(_listener, "object:state-changed:showing", NULL);
   atspi_event_listener_deregister(_listener, "object:state-changed:visible", NULL);
   atspi_event_listener_deregister(_listener, "object:bounds-changed", NULL);
   atspi_event_listener_deregister(_listener, "object:children-changed", NULL);
   atspi_event_listener_deregister(_listener, "object:visible-data-changed", NULL);

   g_object_unref(_listener);
   _listener = NULL;

   g_list_free_full(_roots, _free_rootdata);
   _roots = NULL;
}

int app_tracker_init(void)
{
   DEBUG("START");
   if (!_init_count)
      if (_app_tracker_init_internal()) return -1;
   return ++_init_count;
}

void app_tracker_shutdown(void)
{
   if (_init_count == 1)
      _app_tracker_shutdown_internal();
   if (--_init_count < 0) _init_count = 0;
}

void app_tracker_callback_register(AtspiAccessible *app, AppTrackerEventCB cb, void *user_data)
{
   DEBUG("START");
   SubTreeRootData *rd = NULL;
   EventCallbackData *cd;
   GList *l;

   if (!_init_count || !cb)
      return;

   for (l = _roots; l != NULL; l = l->next)
      {
         rd = l->data;
         if (((SubTreeRootData*)l->data)->root == app)
            {
               rd = l->data;
               break;
            }
      }

   if (!rd)
      {
         rd = g_new(SubTreeRootData, 1);
         rd->root = app;
         rd->callbacks = NULL;
         rd->timer = 0;
         _roots = g_list_append(_roots, rd);
      }

   cd = g_new(EventCallbackData, 1);
   cd->func = cb;
   cd->user_data = user_data;

   rd->callbacks = g_list_append(rd->callbacks, cd);
   DEBUG("END");
}

void app_tracker_callback_unregister(AtspiAccessible *app, AppTrackerEventCB cb, void *user_data)
{
   DEBUG("START");
   GList *l;
   EventCallbackData *ecd;
   SubTreeRootData *std = NULL;

   for (l = _roots; l != NULL; l = l->next)
      {
         if (((SubTreeRootData*)l->data)->root == app)
            {
               std = l->data;
               break;
            }
      }

   if (!std) return;

   for (l = std->callbacks; l != NULL; l = l->next)
      {
         ecd = l->data;
         if ((ecd->func == cb) && (ecd->user_data == user_data))
            {
               std->callbacks = g_list_delete_link(std->callbacks, l);
               break;
            }
      }

   if (!std->callbacks)
      {
         if (std->timer) g_source_remove(std->timer);
         _roots = g_list_remove(_roots, std);
         g_free(std);
      }
}
