

#ifndef __ATSPI_H__
#define __ATSPI_H__

#include <glib-2.0/glib.h>
#include <glib-2.0/glib-object.h>
#include <dbus/dbus.h>

#define ATSPI_ACCESSIBLE_OBJECT_TYPE                (atspi_accessible_get_type ())
#define ATSPI_ACCESSIBLE(obj)                       (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_ACCESSIBLE_OBJECT_TYPE, AtspiAccessible))
#define ATSPI_ACESSIBLE_IS_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_ACCESSIBLE_OBJECT_TYPE))
#define ATSPI_ACCESSIBLE_CLASS(_class)              (G_TYPE_CHECK_CLASS_CAST ((_class), ATSPI_ACCESSIBLE_OBJECT_TYPE, AtspiAccessibleClass))
#define ATSPI_ACCESSIBLE_IS_OBJECT_CLASS(_class)    (G_TYPE_CHECK_CLASS_TYPE ((_class), ATSPI_ACCESSIBLE_OBJECT_TYPE))
#define ATSPI_ACCESSIBLE_GET_CLASS(obj)             (G_TYPE_INSTANCE_GET_CLASS ((obj), ATSPI_ACCESSIBLE_OBJECT_TYPE, AtspiAccessibleClass))

#define ATSPI_COMPONENT_OBJECT_TYPE                 (atspi_component_get_type ())
#define ATSPI_COMPONENT(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_COMPONENT_OBJECT_TYPE, AtspiAccessible))
#define ATSPI_COMPONENT_IS_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_COMPONENT_OBJECT_TYPE))
#define ATSPI_COMPONENT_CLASS(_class)               (G_TYPE_CHECK_CLASS_CAST ((_class), ATSPI_COMPONENT_OBJECT_TYPE, AtspiAccessibleClass))
#define ATSPI_COMPONENT_IS_OBJECT_CLASS(_class)     (G_TYPE_CHECK_CLASS_TYPE ((_class), ATSPI_COMPONENT_OBJECT_TYPE))
#define ATSPI_COMPONENT_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), ATSPI_COMPONENT_OBJECT_TYPE, AtspiAccessibleClass))

#define ATSPI_STATE_OBJECT_TYPE                     (atspi_state_set_get_type ())
#define ATSPI_STATE(obj)                            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ATSPI_STATE_OBJECT_TYPE, AtspiStateSet))
#define ATSPI_STATE_IS_OBJECT(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATSPI_STATE_OBJECT_TYPE))
#define ATSPI_STATE_CLASS(_class)                   (G_TYPE_CHECK_CLASS_CAST ((_class), ATSPI_STATE_OBJECT_TYPE, AtspiStateSetClass))
#define ATSPI_STATE_IS_OBJECT_CLASS(_class)         (G_TYPE_CHECK_CLASS_TYPE ((_class), ATSPI_STATE_OBJECT_TYPE))
#define ATSPI_STATE_GET_CLASS(obj)                  (G_TYPE_INSTANCE_GET_CLASS ((obj), ATSPI_STATE_OBJECT_TYPE, AtspiStateSetClass))


typedef struct _AtspiApplication AtspiApplication;
typedef struct _AtspiObject AtspiObject;
typedef struct _AtspiAccessible AtspiAccessible;
typedef struct _AtspiEvent AtspiEvent;
typedef struct _AtspiStateSet AtspiStateSet;
typedef struct _AtspiEventListener AtspiEventListener;
typedef struct _AtspiText AtspiText;
typedef struct _AtspiValue AtspiValue;
typedef struct _AtspiComponent AtspiComponent;
typedef struct _AtspiScrollable AtspiScrollable;
typedef struct _AtspiRect AtspiRect;
typedef struct _AtspiEditableText AtspiEditableText;
typedef struct _AtspiRelation AtspiRelation;

typedef struct _AtspiAccessibleClass AtspiAccessibleClass;
typedef struct _AtspiComponentClass AtspiComponentClass;
typedef struct _AtspiStateSetClass AtspiStateSetClass;

typedef void (*AtspiEventListenerCB) (AtspiEvent *event, void *user_data);

typedef enum
{
  ATSPI_CACHE_NONE     = 0,
  ATSPI_CACHE_PARENT   = 1 << 0,
  ATSPI_CACHE_CHILDREN    = 1 << 1,
  ATSPI_CACHE_NAME        = 1 << 2,
  ATSPI_CACHE_DESCRIPTION = 1 << 3,
  ATSPI_CACHE_STATES      = 1 << 4,
  ATSPI_CACHE_ROLE        = 1 << 5,
  ATSPI_CACHE_INTERFACES  = 1 << 6,
  ATSPI_CACHE_ATTRIBUTES = 1 << 7,
  ATSPI_CACHE_ALL         = 0x3fffffff,
  ATSPI_CACHE_DEFAULT = ATSPI_CACHE_PARENT | ATSPI_CACHE_CHILDREN |
                        ATSPI_CACHE_NAME | ATSPI_CACHE_DESCRIPTION |
                        ATSPI_CACHE_STATES | ATSPI_CACHE_ROLE |
                        ATSPI_CACHE_INTERFACES,
  ATSPI_CACHE_UNDEFINED   = 0x40000000,
} AtspiCache;

typedef enum {
    ATSPI_ROLE_INVALID,
    ATSPI_ROLE_ACCELERATOR_LABEL,
    ATSPI_ROLE_ALERT,
    ATSPI_ROLE_ANIMATION,
    ATSPI_ROLE_ARROW,
    ATSPI_ROLE_CALENDAR,
    ATSPI_ROLE_CANVAS,
    ATSPI_ROLE_CHECK_BOX,
    ATSPI_ROLE_CHECK_MENU_ITEM,
    ATSPI_ROLE_COLOR_CHOOSER,
    ATSPI_ROLE_COLUMN_HEADER,
    ATSPI_ROLE_COMBO_BOX,
    ATSPI_ROLE_DATE_EDITOR,
    ATSPI_ROLE_DESKTOP_ICON,
    ATSPI_ROLE_DESKTOP_FRAME,
    ATSPI_ROLE_DIAL,
    ATSPI_ROLE_DIALOG,
    ATSPI_ROLE_DIRECTORY_PANE,
    ATSPI_ROLE_DRAWING_AREA,
    ATSPI_ROLE_FILE_CHOOSER,
    ATSPI_ROLE_FILLER,
    ATSPI_ROLE_FOCUS_TRAVERSABLE,
    ATSPI_ROLE_FONT_CHOOSER,
    ATSPI_ROLE_FRAME,
    ATSPI_ROLE_GLASS_PANE,
    ATSPI_ROLE_HTML_CONTAINER,
    ATSPI_ROLE_ICON,
    ATSPI_ROLE_IMAGE,
    ATSPI_ROLE_INTERNAL_FRAME,
    ATSPI_ROLE_LABEL,
    ATSPI_ROLE_LAYERED_PANE,
    ATSPI_ROLE_LIST,
    ATSPI_ROLE_LIST_ITEM,
    ATSPI_ROLE_MENU,
    ATSPI_ROLE_MENU_BAR,
    ATSPI_ROLE_MENU_ITEM,
    ATSPI_ROLE_OPTION_PANE,
    ATSPI_ROLE_PAGE_TAB,
    ATSPI_ROLE_PAGE_TAB_LIST,
    ATSPI_ROLE_PANEL,
    ATSPI_ROLE_PASSWORD_TEXT,
    ATSPI_ROLE_POPUP_MENU,
    ATSPI_ROLE_PROGRESS_BAR,
    ATSPI_ROLE_PUSH_BUTTON,
    ATSPI_ROLE_RADIO_BUTTON,
    ATSPI_ROLE_RADIO_MENU_ITEM,
    ATSPI_ROLE_ROOT_PANE,
    ATSPI_ROLE_ROW_HEADER,
    ATSPI_ROLE_SCROLL_BAR,
    ATSPI_ROLE_SCROLL_PANE,
    ATSPI_ROLE_SEPARATOR,
    ATSPI_ROLE_SLIDER,
    ATSPI_ROLE_SPIN_BUTTON,
    ATSPI_ROLE_SPLIT_PANE,
    ATSPI_ROLE_STATUS_BAR,
    ATSPI_ROLE_TABLE,
    ATSPI_ROLE_TABLE_CELL,
    ATSPI_ROLE_TABLE_COLUMN_HEADER,
    ATSPI_ROLE_TABLE_ROW_HEADER,
    ATSPI_ROLE_TEAROFF_MENU_ITEM,
    ATSPI_ROLE_TERMINAL,
    ATSPI_ROLE_TEXT,
    ATSPI_ROLE_TOGGLE_BUTTON,
    ATSPI_ROLE_TOOL_BAR,
    ATSPI_ROLE_TOOL_TIP,
    ATSPI_ROLE_TREE,
    ATSPI_ROLE_TREE_TABLE,
    ATSPI_ROLE_UNKNOWN,
    ATSPI_ROLE_VIEWPORT,
    ATSPI_ROLE_WINDOW,
    ATSPI_ROLE_EXTENDED,
    ATSPI_ROLE_HEADER,
    ATSPI_ROLE_FOOTER,
    ATSPI_ROLE_PARAGRAPH,
    ATSPI_ROLE_RULER,
    ATSPI_ROLE_APPLICATION,
    ATSPI_ROLE_AUTOCOMPLETE,
    ATSPI_ROLE_EDITBAR,
    ATSPI_ROLE_EMBEDDED,
    ATSPI_ROLE_ENTRY,
    ATSPI_ROLE_CHART,
    ATSPI_ROLE_CAPTION,
    ATSPI_ROLE_DOCUMENT_FRAME,
    ATSPI_ROLE_HEADING,
    ATSPI_ROLE_PAGE,
    ATSPI_ROLE_SECTION,
    ATSPI_ROLE_REDUNDANT_OBJECT,
    ATSPI_ROLE_FORM,
    ATSPI_ROLE_LINK,
    ATSPI_ROLE_INPUT_METHOD_WINDOW,
    ATSPI_ROLE_TABLE_ROW,
    ATSPI_ROLE_TREE_ITEM,
    ATSPI_ROLE_DOCUMENT_SPREADSHEET,
    ATSPI_ROLE_DOCUMENT_PRESENTATION,
    ATSPI_ROLE_DOCUMENT_TEXT,
    ATSPI_ROLE_DOCUMENT_WEB,
    ATSPI_ROLE_DOCUMENT_EMAIL,
    ATSPI_ROLE_COMMENT,
    ATSPI_ROLE_LIST_BOX,
    ATSPI_ROLE_GROUPING,
    ATSPI_ROLE_IMAGE_MAP,
    ATSPI_ROLE_NOTIFICATION,
    ATSPI_ROLE_INFO_BAR,
    ATSPI_ROLE_LEVEL_BAR,
    ATSPI_ROLE_LAST_DEFINED,
} AtspiRole;

typedef enum {
    ATSPI_STATE_INVALID,
    ATSPI_STATE_ACTIVE,
    ATSPI_STATE_ARMED,
    ATSPI_STATE_BUSY,
    ATSPI_STATE_CHECKED,
    ATSPI_STATE_COLLAPSED,
    ATSPI_STATE_DEFUNCT,
    ATSPI_STATE_EDITABLE,
    ATSPI_STATE_ENABLED,
    ATSPI_STATE_EXPANDABLE,
    ATSPI_STATE_EXPANDED,
    ATSPI_STATE_FOCUSABLE,
    ATSPI_STATE_FOCUSED,
    ATSPI_STATE_HAS_TOOLTIP,
    ATSPI_STATE_HORIZONTAL,
    ATSPI_STATE_ICONIFIED,
    ATSPI_STATE_MODAL,
    ATSPI_STATE_MULTI_LINE,
    ATSPI_STATE_MULTISELECTABLE,
    ATSPI_STATE_OPAQUE,
    ATSPI_STATE_PRESSED,
    ATSPI_STATE_RESIZABLE,
    ATSPI_STATE_SELECTABLE,
    ATSPI_STATE_SELECTED,
    ATSPI_STATE_SENSITIVE,
    ATSPI_STATE_SHOWING,
    ATSPI_STATE_SINGLE_LINE,
    ATSPI_STATE_STALE,
    ATSPI_STATE_TRANSIENT,
    ATSPI_STATE_VERTICAL,
    ATSPI_STATE_VISIBLE,
    ATSPI_STATE_MANAGES_DESCENDANTS,
    ATSPI_STATE_INDETERMINATE,
    ATSPI_STATE_REQUIRED,
    ATSPI_STATE_TRUNCATED,
    ATSPI_STATE_ANIMATED,
    ATSPI_STATE_INVALID_ENTRY,
    ATSPI_STATE_SUPPORTS_AUTOCOMPLETION,
    ATSPI_STATE_SELECTABLE_TEXT,
    ATSPI_STATE_IS_DEFAULT,
    ATSPI_STATE_VISITED,
    ATSPI_STATE_HIGHLIGHTED,
    ATSPI_STATE_LAST_DEFINED,
} AtspiStateType;

typedef enum
{
    ATSPI_RELATION_NULL,
    ATSPI_RELATION_LABEL_FOR,
    ATSPI_RELATION_LABELLED_BY,
} AtspiRelationType;

struct _AtspiApplication
{
  GObject parent;
  GHashTable *hash;
  char *bus_name;
  DBusConnection *bus;
  AtspiAccessible *root;
  AtspiCache cache;
  gchar *toolkit_name;
  gchar *toolkit_version;
  gchar *atspi_version;
  struct timeval time_added;
};

struct _AtspiObject
{
    GObject parent;
    AtspiApplication *app;
    char *path;
};

struct _AtspiAccessible
{
    //GObject parent;
    AtspiObject parent;
    AtspiAccessible *accessible_parent;
    GList *children;
    AtspiRole role;
    gint interfaces;
    char *name;
    char *description;
    AtspiStateSet *states;
    GHashTable *attributes;
    guint cached_properties;
};

struct _AtspiAccessibleClass
{
    GObjectClass parent_class;
};

struct _AtspiComponentClass
{
    GObjectClass parent_class;
};

struct _AtspiEvent
{
    gchar  *type;
    AtspiAccessible  *source;
    gint         detail1;
    gint         detail2;
    GValue any_data;
};

struct _AtspiStateSet
{
  GObject parent;
  struct _AtspiAccessible *accessible;
  gint64 states;
};

struct _AtspiStateSetClass
{
    GObjectClass parent_class;
};

struct _AtspiEventListener
{
  GObject parent;
  AtspiEventListenerCB callback;
  void *user_data;
  GDestroyNotify cb_destroyed;
};

struct _AtspiText
{
    GTypeInterface parent;
};

struct _AtspiEditableText
{
    GTypeInterface parent;
};

struct _AtspiValue
{
    GTypeInterface parent;
};

struct _AtspiComponent
{
  GTypeInterface parent;
  AtspiRole *role;
};

struct _AtspiScrollable
{
    GTypeInterface parent;
};

struct _AtspiRelation
{
    GTypeInterface parent;
};

struct _AtspiRect
{
  gint x;
  gint y;
  gint width;
  gint height;
};

typedef enum {
    ATSPI_COORD_TYPE_SCREEN,
    ATSPI_COORD_TYPE_WINDOW,
} AtspiCoordType;

gchar * atspi_accessible_get_name (AtspiAccessible *obj, GError **error);
gchar * atspi_accessible_get_role_name (AtspiAccessible *obj, GError **error);
gchar * atspi_accessible_get_toolkit_name (AtspiAccessible *obj, GError **error);
gchar * atspi_accessible_get_description (AtspiAccessible *obj, GError **error);
AtspiText * atspi_accessible_get_text (AtspiAccessible *obj);
gint atspi_text_get_character_count (AtspiText *obj, GError **error);
gint atspi_text_get_caret_offset (AtspiText *obj, GError **error);
gchar * atspi_text_get_text (AtspiText *obj, gint start_offset, gint end_offset, GError **error);
AtspiValue * atspi_accessible_get_value (AtspiAccessible *obj);
gdouble atspi_value_get_current_value (AtspiValue *obj, GError **error);
gdouble atspi_value_get_maximum_value (AtspiValue *obj, GError **error);
gdouble atspi_value_get_minimum_value (AtspiValue *obj, GError **error);
AtspiEventListener *atspi_event_listener_new (AtspiEventListenerCB callback,
                                                gpointer user_data,
                                                GDestroyNotify callback_destroyed);
gboolean atspi_event_listener_register (AtspiEventListener *listener,
                                        const gchar *event_type,
                                        GError **error);
gboolean atspi_event_listener_deregister (AtspiEventListener *listener,
                                          const gchar *event_type,
                                          GError **error);
AtspiStateSet * atspi_accessible_get_state_set (AtspiAccessible *obj);
gboolean atspi_state_set_contains (AtspiStateSet *set, AtspiStateType state);
void atspi_state_set_add (AtspiStateSet *set, AtspiStateType state);
GArray *atspi_state_set_get_states (AtspiStateSet *set);
AtspiStateSet * atspi_state_set_new (GArray *states);

void atspi_alloc_memory(void);

void atspi_free_memory(void);
gboolean atspi_component_grab_highlight (AtspiComponent *obj, GError **error);
AtspiScrollable *atspi_accessible_get_scrollable (AtspiAccessible *accessible);
gboolean atspi_component_clear_highlight (AtspiComponent *obj, GError **error);
AtspiRole atspi_accessible_get_role (AtspiAccessible *obj, GError **error);
gint atspi_accessible_get_child_count (AtspiAccessible *obj, GError **error);
AtspiAccessible * atspi_accessible_get_child_at_index (AtspiAccessible *obj, gint child_index, GError **error);
AtspiComponent * atspi_accessible_get_component (AtspiAccessible *obj);
AtspiRect *atspi_component_get_extents (AtspiComponent *obj, AtspiCoordType ctype, GError **error);
AtspiAccessible *atspi_create_accessible(void);
void atspi_delete_accessible(AtspiAccessible *obj);
void atspi_accessible_add_child(AtspiAccessible *obj, AtspiAccessible *child);
void atpis_accessible_remove_children(AtspiAccessible *obj);
AtspiEditableText * atspi_accessible_get_editable_text (AtspiAccessible *obj);
GArray * atspi_accessible_get_relation_set (AtspiAccessible *obj, GError **error);
AtspiRelationType atspi_relation_get_relation_type (AtspiRelation *obj);
gint atspi_relation_get_n_targets (AtspiRelation *obj);
AtspiAccessible * atspi_relation_get_target (AtspiRelation *obj, gint i);
int atspi_exit(void);

#endif /*__ATSPI_H__*/
