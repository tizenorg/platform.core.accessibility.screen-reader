#ifndef SMART_NAVI_POSITION_SORT_H_
#define SMART_NAVI_POSITION_SORT_H_

#include <atspi/atspi.h>
#include <Eina.h>


/**
 * @brief Sort objects by position they appear on screen
 *
 *  example:
 *
 *  ------------------------------
 *  |                   _______  |
 *  |  _______          |     |  |
 *  |  |     |  ______  |     |  |
 *  |  |  A  |  |    |  |     |  |
 *  |  |_____|  | B  |  |  C  |  |
 *  |           |    |  |     |  |
 *  |           |____|  |     |  |
 *  |   _____           |     |  |             line 0: A, B, C
 *  |   |   |           |_____|  |             line 1: D
 *  |   | D |                    |    ====>    line 2: F
 *  |   |___| _____              |             line 3: G
 *  |         |   |              |
 *  |         | F | _____        |
 *  |         |___| |   |        |
 *  |               | G |        |
 *  |               |___|        |
 *  |                            |
 *  ------------------------------
 *
 * @ret list List of lists
 */
Eina_List *position_sort(const Eina_List *obj);

#endif /* end of include guard: POSITION_SORT_H_ */
