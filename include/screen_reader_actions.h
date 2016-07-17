#ifndef SCREEN_READER_ACTIONS_H_
#define SCREEN_READER_ACTIONS_H_

#include <atspi/atspi.h>
#include <Eina.h>

#define USER_TRIGGERABLE_ACTIONS_MAX_COUNT 32

enum _MatchType {
    ALL,
    ANY
};

typedef enum _MatchType MatchType;

// Number of all user triggerable actions may not exceed USER_TRIGGERABLE_ACTIONS_MAX_COUNT
enum _UserTriggerableAction {
    USER_TRIGGERABLE_ACTION_ACTIVATE,
    USER_TRIGGERABLE_ACTION_PAUSE_PLAY,
    USER_TRIGGERABLE_ACTION_LAST_DEFINED
};

typedef enum _UserTriggerableAction UserTriggerableAction;

/**
 * Returns full set of user triggerable actions defined by screen reader.
 *
 * @return UserTriggerableAction* pointer to the first element of UserTriggerableAction set
 *
 */
const UserTriggerableAction *get_user_triggerable_actions();

const char *user_triggerable_action_get_name(UserTriggerableAction user_action);

gint atspi_action_iface_find_user_triggerable_action(AtspiAction *action_iface, UserTriggerableAction user_action);

/**
 * Checks if given AT-SPI2 action interface contains action which name match
 * the name of given user triggerable action
 *
 * @param action_iface pointer to the AtspiAction interface
 * @param user_action  user triggerable action to be searched
 *
 * @return Eina_Bool returns EINA_TRUE if AT-SPI2 action with matching name was found, otherwise EINA_FALSE
 */
Eina_Bool atspi_action_iface_contains_user_triggerable_action(AtspiAction *action_iface, UserTriggerableAction user_action);

/**
 * Checks if given AT-SPI2 action interface contains actions which names match
 * the names of given user triggerable actions accordingly to the match type.
 * Supported match types are:
 *    ANY - success if at least one user triggerable action used in query was matched
 *    ALL - success if all user triggerable actions used in query was matched
 *
 * @param action_iface pointer to the AtspiAction interface
 * @param user_actions  set of user triggerable actions to be searched
 * @param match_type type of match, suported values are: ALL, ANY
 *
 * @return Eina_Bool returns EINA_TRUE if AT-SPI2 actions with matching names were found respecting given match type,
 *  otherwise EINA_FALSE
 *
 * @note size of user_actions may not exceed USER_TRIGGERABLE_ACTIONS_MAX_COUNT
 */
Eina_Bool atspi_action_iface_contains_user_triggerable_actions(AtspiAction *action_iface, const UserTriggerableAction *user_actions, MatchType match_type);

#endif /* end of include guard: SCREEN_READER_ACTIONS_H_ */
