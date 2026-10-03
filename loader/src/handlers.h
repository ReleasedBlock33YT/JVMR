#ifndef JVMR_HANDLERS_H
#define JVMR_HANDLERS_H

void handler_nop(void);

// miscellaneous numeric and reference operations
void handler_iaload(void);
void handler_laload(void);
void handler_faload(void);
void handler_daload(void);
void handler_aaload(void);
void handler_baload(void);
void handler_caload(void);
void handler_saload(void);
void handler_iastore(void);
void handler_lastore(void);
void handler_fastore(void);
void handler_dastore(void);
void handler_aastore(void);
void handler_bastore(void);
void handler_castore(void);
void handler_sastore(void);
void handler_ishl(void);
void handler_lshl(void);
void handler_ishr(void);
void handler_lshr(void);
void handler_iushr(void);
void handler_lushr(void);
void handler_iand(void);
void handler_land(void);
void handler_ior(void);
void handler_lor(void);
void handler_ixor(void);
void handler_lxor(void);
void handler_i2l(void);
void handler_i2f(void);
void handler_i2d(void);
void handler_l2i(void);
void handler_l2f(void);
void handler_l2d(void);
void handler_f2i(void);
void handler_f2l(void);
void handler_f2d(void);
void handler_d2i(void);
void handler_d2l(void);
void handler_d2f(void);
void handler_i2b(void);
void handler_i2c(void);
void handler_i2s(void);
void handler_lcmp(void);
void handler_fcmpl(void);
void handler_fcmpg(void);
void handler_dcmpl(void);
void handler_dcmpg(void);
void handler_iinc(void);
void handler_wide(void);
void handler_if_acmpeq(void);
void handler_if_acmpne(void);
void handler_ifnull(void);
void handler_ifnonnull(void);
void handler_goto_w(void);
void handler_jsr(void);
void handler_jsr_w(void);
void handler_ret(void);
void handler_invokedynamic(void);
void handler_multianewarray(void);

// arithmetic
void handler_iadd(void);
void handler_isub(void);
void handler_imul(void);
void handler_idiv(void);
void handler_irem(void);
void handler_ineg(void);

void handler_ladd(void);
void handler_lsub(void);
void handler_lmul(void);
void handler_ldiv(void);
void handler_lrem(void);
void handler_lneg(void);

void handler_fadd(void);
void handler_fsub(void);
void handler_fmul(void);
void handler_fdiv(void);
void handler_frem(void);
void handler_fneg(void);

void handler_dadd(void);
void handler_dsub(void);
void handler_dmul(void);
void handler_ddiv(void);
void handler_drem(void);
void handler_dneg(void);

// loads
void handler_iload(void);
void handler_lload(void);
void handler_fload(void);
void handler_dload(void);
void handler_iload_0(void);
void handler_iload_1(void);
void handler_iload_2(void);
void handler_iload_3(void);
void handler_lload_0(void); void handler_lload_1(void); void handler_lload_2(void); void handler_lload_3(void);
void handler_fload_0(void); void handler_fload_1(void); void handler_fload_2(void); void handler_fload_3(void);
void handler_dload_0(void); void handler_dload_1(void); void handler_dload_2(void); void handler_dload_3(void);

void handler_aload(void);
void handler_aload_0(void);
void handler_aload_1(void);
void handler_aload_2(void);
void handler_aload_3(void);

// stores
void handler_istore(void);
void handler_lstore(void);
void handler_fstore(void);
void handler_dstore(void);
void handler_istore_0(void);
void handler_istore_1(void);
void handler_istore_2(void);
void handler_istore_3(void);
void handler_lstore_0(void); void handler_lstore_1(void); void handler_lstore_2(void); void handler_lstore_3(void);
void handler_fstore_0(void); void handler_fstore_1(void); void handler_fstore_2(void); void handler_fstore_3(void);
void handler_dstore_0(void); void handler_dstore_1(void); void handler_dstore_2(void); void handler_dstore_3(void);

void handler_astore(void);
void handler_astore_0(void);
void handler_astore_1(void);
void handler_astore_2(void);
void handler_astore_3(void);

// stack ops
void handler_pop(void);
void handler_pop2(void);
void handler_dup(void);
void handler_dup_x1(void);
void handler_dup_x2(void);
void handler_dup2(void);
void handler_dup2_x1(void);
void handler_dup2_x2(void);
void handler_swap(void);

// control flow
void handler_ifeq(void);
void handler_ifne(void);
void handler_iflt(void);
void handler_ifge(void);
void handler_ifgt(void);
void handler_ifle(void);

void handler_if_icmpeq(void);
void handler_if_icmpne(void);
void handler_if_icmplt(void);
void handler_if_icmpge(void);
void handler_if_icmpgt(void);
void handler_if_icmple(void);

void handler_goto(void);
void handler_tableswitch(void);
void handler_lookupswitch(void);

// objects
void handler_new(void);
void handler_newarray(void);
void handler_anewarray(void);
void handler_arraylength(void);

void handler_checkcast(void);
void handler_instanceof(void);

// fields
void handler_getstatic(void);
void handler_putstatic(void);
void handler_getfield(void);
void handler_putfield(void);

// methods
void handler_invokevirtual(void);
void handler_invokespecial(void);
void handler_invokestatic(void);
void handler_invokeinterface(void);

// returns
void handler_ireturn(void);
void handler_lreturn(void);
void handler_freturn(void);
void handler_dreturn(void);
void handler_areturn(void);
void handler_return(void);

// exceptions
void handler_athrow(void);

// sync
void handler_monitorenter(void);
void handler_monitorexit(void);

// constants
void handler_aconst_null(void);

void handler_iconst_m1(void);
void handler_iconst_0(void);
void handler_iconst_1(void);
void handler_iconst_2(void);
void handler_iconst_3(void);
void handler_iconst_4(void);
void handler_iconst_5(void);

void handler_lconst_0(void);
void handler_lconst_1(void);

void handler_fconst_0(void);
void handler_fconst_1(void);
void handler_fconst_2(void);

void handler_dconst_0(void);
void handler_dconst_1(void);

void handler_bipush(void);
void handler_sipush(void);

void handler_ldc(void);
void handler_ldc_w(void);
void handler_ldc2_w(void);

#endif
