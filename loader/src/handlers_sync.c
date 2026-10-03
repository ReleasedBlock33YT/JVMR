#include <stdio.h>
#include "handlers.h"
#include "frame.h"
/* The single-threaded runtime has no contended monitors yet, but monitor
 * instructions still consume their object reference as required by the JVM.
 */
void handler_monitorenter(void) {
	if (current_frame.sp) --current_frame.sp;
}
void handler_monitorexit(void) {
	if (current_frame.sp) --current_frame.sp;
}
