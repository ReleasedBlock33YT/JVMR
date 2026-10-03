#include "handlers.h"
#include "bytecode.h"
#include "runtime.h"
void handler_invokevirtual(void) { uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode()); (void)jvmr_invoke_instance(index); }
void handler_invokespecial(void) { uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode()); (void)jvmr_invoke_special(index); }
void handler_invokeinterface(void) { uint16_t index=(uint16_t)((fetch_bytecode()<<8)|fetch_bytecode()); (void)fetch_bytecode(); (void)fetch_bytecode(); (void)jvmr_invoke_interface(index); }
void handler_invokestatic(void) {
	uint16_t index = (uint16_t)((fetch_bytecode() << 8) | fetch_bytecode());
	(void)jvmr_invoke_static(index);
}
