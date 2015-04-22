#ifndef SMART_NAVI_PIVOT_CHOOSER_H_
#define SMART_NAVI_PIVOT_CHOOSER_H_

#include <atspi/atspi.h>

/**
 * @brief Some heuristic choosing candidate to reacieve highlight.
 *
 * @param win Accessibility search tree object root.
 *
 * @return Highlight candidate
 */
AtspiAccessible *pivot_chooser_pivot_get(AtspiAccessible *win);

#endif /* end of include guard: PIVOT_CHOOSER_H_ */
