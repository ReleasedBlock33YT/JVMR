#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "errors.h"
#include "bytecode.h"
#include "handlers.h"
#include "frame.h"

void (*bytecode[JVMR_OPCODE_COUNT])(void);

int bytecode_init() {
	for (int i = 0; i < JVMR_OPCODE_COUNT; i++) {
		bytecode[i] = handler_nop;
	}

	// arithmetic
	bytecode[IADD] = handler_iadd;
	bytecode[ISUB] = handler_isub;
	bytecode[IMUL] = handler_imul;
	bytecode[IDIV] = handler_idiv;
	bytecode[IREM] = handler_irem;
	bytecode[INEG] = handler_ineg;

	bytecode[LADD] = handler_ladd;
	bytecode[LSUB] = handler_lsub;
	bytecode[LMUL] = handler_lmul;
	bytecode[LDIV] = handler_ldiv;
	bytecode[LREM] = handler_lrem;
	bytecode[LNEG] = handler_lneg;

	bytecode[FADD] = handler_fadd;
	bytecode[FSUB] = handler_fsub;
	bytecode[FMUL] = handler_fmul;
	bytecode[FDIV] = handler_fdiv;
	bytecode[FREM] = handler_frem;
	bytecode[FNEG] = handler_fneg;

	bytecode[DADD] = handler_dadd;
	bytecode[DSUB] = handler_dsub;
	bytecode[DMUL] = handler_dmul;
	bytecode[DDIV] = handler_ddiv;
	bytecode[DREM] = handler_drem;
	bytecode[DNEG] = handler_dneg;
	bytecode[ISHL] = handler_ishl; bytecode[LSHL] = handler_lshl;
	bytecode[ISHR] = handler_ishr; bytecode[LSHR] = handler_lshr;
	bytecode[IUSHR] = handler_iushr; bytecode[LUSHR] = handler_lushr;
	bytecode[IAND] = handler_iand; bytecode[LAND] = handler_land;
	bytecode[IOR] = handler_ior; bytecode[LOR] = handler_lor;
	bytecode[IXOR] = handler_ixor; bytecode[LXOR] = handler_lxor;
	bytecode[I2L] = handler_i2l; bytecode[I2F] = handler_i2f; bytecode[I2D] = handler_i2d;
	bytecode[L2I] = handler_l2i; bytecode[L2F] = handler_l2f; bytecode[L2D] = handler_l2d;
	bytecode[F2I] = handler_f2i; bytecode[F2L] = handler_f2l; bytecode[F2D] = handler_f2d;
	bytecode[D2I] = handler_d2i; bytecode[D2L] = handler_d2l; bytecode[D2F] = handler_d2f;

	// Constants
	bytecode[ACONST_NULL] = handler_aconst_null;
	bytecode[ICONST_M1] = handler_iconst_m1; bytecode[ICONST_0] = handler_iconst_0;
	bytecode[ICONST_1] = handler_iconst_1; bytecode[ICONST_2] = handler_iconst_2;
	bytecode[ICONST_3] = handler_iconst_3; bytecode[ICONST_4] = handler_iconst_4; bytecode[ICONST_5] = handler_iconst_5;
	bytecode[LCONST_0] = handler_lconst_0; bytecode[LCONST_1] = handler_lconst_1;
	bytecode[FCONST_0] = handler_fconst_0; bytecode[FCONST_1] = handler_fconst_1; bytecode[FCONST_2] = handler_fconst_2;
	bytecode[DCONST_0] = handler_dconst_0; bytecode[DCONST_1] = handler_dconst_1;
	bytecode[BIPUSH] = handler_bipush; bytecode[SIPUSH] = handler_sipush;
	bytecode[LDC] = handler_ldc; bytecode[LDC_W] = handler_ldc_w; bytecode[LDC2_W] = handler_ldc2_w;

	bytecode[ILOAD] = handler_iload; bytecode[LLOAD] = handler_lload; bytecode[FLOAD] = handler_fload;
	bytecode[DLOAD] = handler_dload; bytecode[ALOAD] = handler_aload;
	bytecode[ILOAD_0] = handler_iload_0; bytecode[ILOAD_1] = handler_iload_1; bytecode[ILOAD_2] = handler_iload_2; bytecode[ILOAD_3] = handler_iload_3;
	bytecode[LLOAD_0] = handler_lload_0; bytecode[LLOAD_1] = handler_lload_1; bytecode[LLOAD_2] = handler_lload_2; bytecode[LLOAD_3] = handler_lload_3;
	bytecode[FLOAD_0] = handler_fload_0; bytecode[FLOAD_1] = handler_fload_1; bytecode[FLOAD_2] = handler_fload_2; bytecode[FLOAD_3] = handler_fload_3;
	bytecode[DLOAD_0] = handler_dload_0; bytecode[DLOAD_1] = handler_dload_1; bytecode[DLOAD_2] = handler_dload_2; bytecode[DLOAD_3] = handler_dload_3;
	bytecode[ALOAD_0] = handler_aload_0; bytecode[ALOAD_1] = handler_aload_1; bytecode[ALOAD_2] = handler_aload_2; bytecode[ALOAD_3] = handler_aload_3;
	bytecode[ISTORE] = handler_istore; bytecode[LSTORE] = handler_lstore; bytecode[FSTORE] = handler_fstore; bytecode[DSTORE] = handler_dstore; bytecode[ASTORE] = handler_astore;
	bytecode[ISTORE_0] = handler_istore_0; bytecode[ISTORE_1] = handler_istore_1; bytecode[ISTORE_2] = handler_istore_2; bytecode[ISTORE_3] = handler_istore_3;
	bytecode[LSTORE_0] = handler_lstore_0; bytecode[LSTORE_1] = handler_lstore_1; bytecode[LSTORE_2] = handler_lstore_2; bytecode[LSTORE_3] = handler_lstore_3;
	bytecode[FSTORE_0] = handler_fstore_0; bytecode[FSTORE_1] = handler_fstore_1; bytecode[FSTORE_2] = handler_fstore_2; bytecode[FSTORE_3] = handler_fstore_3;
	bytecode[DSTORE_0] = handler_dstore_0; bytecode[DSTORE_1] = handler_dstore_1; bytecode[DSTORE_2] = handler_dstore_2; bytecode[DSTORE_3] = handler_dstore_3;
	bytecode[ASTORE_0] = handler_astore_0; bytecode[ASTORE_1] = handler_astore_1; bytecode[ASTORE_2] = handler_astore_2; bytecode[ASTORE_3] = handler_astore_3;
	bytecode[IALOAD] = handler_iaload; bytecode[LALOAD] = handler_laload; bytecode[FALOAD] = handler_faload; bytecode[DALOAD] = handler_daload; bytecode[AALOAD] = handler_aaload;
	bytecode[BALOAD] = handler_baload; bytecode[CALOAD] = handler_caload; bytecode[SALOAD] = handler_saload;
	bytecode[IASTORE] = handler_iastore; bytecode[LASTORE] = handler_lastore; bytecode[FASTORE] = handler_fastore; bytecode[DASTORE] = handler_dastore; bytecode[AASTORE] = handler_aastore; bytecode[BASTORE] = handler_bastore; bytecode[CASTORE] = handler_castore; bytecode[SASTORE] = handler_sastore;

	// Stack
	bytecode[POP] = handler_pop; bytecode[POP2] = handler_pop2; bytecode[DUP] = handler_dup;
	bytecode[DUP_X1] = handler_dup_x1; bytecode[DUP_X2] = handler_dup_x2; bytecode[DUP2] = handler_dup2;
	bytecode[DUP2_X1] = handler_dup2_x1; bytecode[DUP2_X2] = handler_dup2_x2; bytecode[SWAP] = handler_swap;

	bytecode[IFEQ] = handler_ifeq; bytecode[IFNE] = handler_ifne; bytecode[IFLT] = handler_iflt; bytecode[IFGE] = handler_ifge; bytecode[IFGT] = handler_ifgt; bytecode[IFLE] = handler_ifle;
	bytecode[IF_ICMPEQ] = handler_if_icmpeq; bytecode[IF_ICMPNE] = handler_if_icmpne; bytecode[IF_ICMPLT] = handler_if_icmplt; bytecode[IF_ICMPGE] = handler_if_icmpge; bytecode[IF_ICMPGT] = handler_if_icmpgt; bytecode[IF_ICMPLE] = handler_if_icmple;
	bytecode[GOTO] = handler_goto; bytecode[TABLESWITCH] = handler_tableswitch; bytecode[LOOKUPSWITCH] = handler_lookupswitch;
	bytecode[NEW] = handler_new; bytecode[NEWARRAY] = handler_newarray; bytecode[ANEWARRAY] = handler_anewarray; bytecode[ARRAYLENGTH] = handler_arraylength;
	bytecode[CHECKCAST] = handler_checkcast; bytecode[INSTANCEOF] = handler_instanceof;
	bytecode[GETSTATIC] = handler_getstatic; bytecode[PUTSTATIC] = handler_putstatic; bytecode[GETFIELD] = handler_getfield; bytecode[PUTFIELD] = handler_putfield;
	bytecode[INVOKEVIRTUAL] = handler_invokevirtual; bytecode[INVOKESPECIAL] = handler_invokespecial; bytecode[INVOKESTATIC] = handler_invokestatic; bytecode[INVOKEINTERFACE] = handler_invokeinterface;
	bytecode[IRETURN] = handler_ireturn; bytecode[LRETURN] = handler_lreturn; bytecode[FRETURN] = handler_freturn; bytecode[DRETURN] = handler_dreturn; bytecode[ARETURN] = handler_areturn; bytecode[RETURN] = handler_return;
	bytecode[ATHROW] = handler_athrow; bytecode[MONITORENTER] = handler_monitorenter; bytecode[MONITOREXIT] = handler_monitorexit;
	bytecode[IINC] = handler_iinc; bytecode[I2B] = handler_i2b; bytecode[I2C] = handler_i2c; bytecode[I2S] = handler_i2s;
	bytecode[LCMP] = handler_lcmp; bytecode[FCMPL] = handler_fcmpl; bytecode[FCMPG] = handler_fcmpg; bytecode[DCMPL] = handler_dcmpl; bytecode[DCMPG] = handler_dcmpg;
	bytecode[IF_ACMPEQ] = handler_if_acmpeq; bytecode[IF_ACMPNE] = handler_if_acmpne; bytecode[IFNULL] = handler_ifnull; bytecode[IFNONNULL] = handler_ifnonnull;
	bytecode[GOTO_W] = handler_goto_w; bytecode[JSR] = handler_jsr; bytecode[JSR_W] = handler_jsr_w; bytecode[RET] = handler_ret;
	bytecode[INVOKEDYNAMIC] = handler_invokedynamic; bytecode[WIDE] = handler_wide; bytecode[MULTIANEWARRAY] = handler_multianewarray;

	return JVM_SUCCESS;
}

uint8_t fetch_bytecode(void) {
	return current_frame.code[current_frame.pc++];
}

void handler_nop(void) {
	printf("[JVMR/INFO] NOP executed.\n");
}
