#include "screen_reader_actions.h"
#include "logger.h"

#define GERROR_CHECK(error)\
  if (error)\
   {\
     ERROR("Error_log:%s",error->message);\
     g_error_free(error);\
     error = NULL;\
   }

static UserTriggerableAction _user_triggerable_actions[] = {
	USER_TRIGGERABLE_ACTION_ACTIVATE,
	USER_TRIGGERABLE_ACTION_PAUSE_PLAY,
	USER_TRIGGERABLE_ACTION_LAST_DEFINED
};

static char *_user_triggerable_action_names[] = {
	[USER_TRIGGERABLE_ACTION_ACTIVATE] = "activate",
	[USER_TRIGGERABLE_ACTION_PAUSE_PLAY] = "pause_play",
	[USER_TRIGGERABLE_ACTION_LAST_DEFINED] = NULL
};

static Eina_Bool _atspi_action_iface_contains_user_triggerable_actions(AtspiAction *action_iface, const UserTriggerableAction *user_actions, MatchType match_type/*, Eina_Hash *matched_atspi_actions_indexes*/)
{
	unsigned int matches_bitmap = 0; // used to count unique matches between user actions names and atspi actions names
	unsigned int actions_bitmap = 0; // used to count unique user actions used for query 
	Eina_Bool ret = EINA_FALSE;
	GError *err = NULL;

	if (action_iface && user_actions) {
		int i = 0;
		for (; i < atspi_action_get_n_actions(action_iface, NULL); i++) {
			gchar *action_name = atspi_action_get_action_name(action_iface, i, &err);
			GERROR_CHECK(err);
			Eina_Bool match = EINA_FALSE;
			int idx = 0;
			while (user_actions[idx] != USER_TRIGGERABLE_ACTION_LAST_DEFINED && !match) {
				// mark that particular user action was used in query by seting to 1 bit on position user_actions[idx] in the action bitmap
				actions_bitmap ^= (-1 ^ actions_bitmap) & (1 << user_actions[idx]);
				match = !strcmp(action_name, user_triggerable_action_get_name(user_actions[idx]));
				if (match) {
					// mark match in the match bitmap by seting to 1 bit on position user_actions[idx]
					matches_bitmap ^= (-1 ^ matches_bitmap) & (1 << user_actions[idx]);
				}
				idx++;
			}
			g_free(action_name);
			if ((match_type == ANY && match)) {
				ret = EINA_TRUE;
				break;
			}
		}
		if (match_type == ALL) {
			//check if matches covers all user actions used in query 
			int unique_match_count = __builtin_popcount(matches_bitmap);
			int unique_action_count = __builtin_popcount(actions_bitmap);
			if (unique_match_count == unique_action_count)
				ret = EINA_TRUE;
		}
	}
	return ret;
}

const UserTriggerableAction *get_user_triggerable_actions() {
	return &_user_triggerable_actions[0];
}

const char *user_triggerable_action_get_name(UserTriggerableAction user_action) {
	return _user_triggerable_action_names[user_action];
}

gint atspi_action_iface_find_user_triggerable_action(AtspiAction *action_iface, UserTriggerableAction user_action) {
	GError *err = NULL;
	const char *user_action_name = user_triggerable_action_get_name(user_action);

	int number_of_actions = atspi_action_get_n_actions(action_iface, &err);
	GERROR_CHECK(err);
	Eina_Bool found = EINA_FALSE;
	int i = 0;
	while (i < number_of_actions && !found) {
		char* action_name = atspi_action_get_action_name(action_iface, i, &err);
		GERROR_CHECK(err);
		if (action_name && !strcmp(user_action_name, action_name)) {
			found = EINA_TRUE;
		} else {
			i++;
		}
		g_free(action_name);
	}
	return found ? i : -1;
}

Eina_Bool atspi_action_iface_contains_user_triggerable_action(AtspiAction *action_iface, UserTriggerableAction user_action/*, int *matched_atspi_action_index, GError **error*/) {
	UserTriggerableAction user_actions[2] = {user_action, USER_TRIGGERABLE_ACTION_LAST_DEFINED};
	Eina_Bool ret = _atspi_action_iface_contains_user_triggerable_actions(action_iface, &user_actions[0], ANY/*, matched_atspi_actions_indexes*/);
	return ret;
}

Eina_Bool atspi_action_iface_contains_user_triggerable_actions(AtspiAction *action_iface, const UserTriggerableAction *user_actions, MatchType match_type/*, Eina_Hash *matched_atspi_actions_indexes*/) {
	return _atspi_action_iface_contains_user_triggerable_actions(action_iface, user_actions, match_type/*, matched_atspi_actions_indexes*/);
}
