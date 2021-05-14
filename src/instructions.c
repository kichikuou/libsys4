/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "system4.h"
#include "system4/instructions.h"

#define SYS(syscode, sysname, rt, nargs, ...)	\
	[syscode] = {				\
		.code = syscode,		\
		.name = "system." #sysname ,	\
		.implemented = true,		\
		.return_type = rt,		\
		.nr_args = nargs,		\
		.argtypes = { __VA_ARGS__ }	\
	}

const struct syscall syscalls[NR_SYSCALLS] = {
	SYS  ( SYS_EXIT,                 Exit,               AIN_VOID_TYPE,   1, AIN_INT ),
	SYS  ( SYS_GLOBAL_SAVE,          GlobalSave,         AIN_INT_TYPE,    2, AIN_STRING, AIN_STRING ),
	SYS  ( SYS_GLOBAL_LOAD,          GlobalLoad,         AIN_INT_TYPE,    2, AIN_STRING, AIN_STRING ),
	SYS  ( SYS_LOCK_PEEK,            LockPeek,           AIN_INT_TYPE,    0 ),
	SYS  ( SYS_UNLOCK_PEEK,          UnlockPeek,         AIN_INT_TYPE,    0 ),
	SYS  ( SYS_RESET,                Reset,              AIN_VOID_TYPE,   0 ),
	SYS  ( SYS_OUTPUT,               Output,             AIN_STRING_TYPE, 1, AIN_STRING ),
	SYS  ( SYS_MSGBOX,               MsgBox,             AIN_STRING_TYPE, 1, AIN_STRING ),
	SYS  ( SYS_RESUME_SAVE,          ResumeSave,         AIN_INT_TYPE,    3, AIN_STRING, AIN_STRING, AIN_REF_INT ),
	SYS  ( SYS_RESUME_LOAD,          ResumeLoad,         AIN_VOID_TYPE,   2, AIN_STRING, AIN_STRING ),
	SYS  ( SYS_EXISTS_FILE,          ExistsFile,         AIN_INT_TYPE,    1, AIN_STRING ),
	SYS  ( SYS_OPEN_WEB,             OpenWeb,            AIN_VOID_TYPE,   1, AIN_STRING ),
	SYS  ( SYS_GET_SAVE_FOLDER_NAME, GetSaveFolderName,  AIN_STRING_TYPE, 0 ),
	SYS  ( SYS_GET_TIME,             GetTime,            AIN_INT_TYPE,    0 ),
	SYS  ( SYS_GET_GAME_NAME,        GetGameName,        AIN_STRING_TYPE, 0 ),
	SYS  ( SYS_ERROR,                Error,              AIN_STRING_TYPE, 1, AIN_STRING ),
	SYS  ( SYS_EXISTS_SAVE_FILE,     ExistsSaveFile,     AIN_INT_TYPE,    1, AIN_STRING ),
	SYS  ( SYS_IS_DEBUG_MODE,        IsDebugMode,        AIN_INT_TYPE,    0 ),
	SYS  ( SYS_MSGBOX_OK_CANCEL,     MsgBoxOkCancel,     AIN_INT_TYPE,    1, AIN_STRING ),
	SYS  ( SYS_GET_FUNC_STACK_NAME,  GetFuncStackName,   AIN_STRING_TYPE, 1, AIN_INT ),
	SYS  ( SYS_PEEK,                 Peek,               AIN_VOID_TYPE,   0 ),
	SYS  ( SYS_SLEEP,                Sleep,              AIN_VOID_TYPE,   1, AIN_INT ),
	SYS  ( SYS_GROUP_SAVE,           GroupSave,          AIN_INT_TYPE,    4, AIN_STRING, AIN_STRING, AIN_STRING, AIN_REF_INT ),
	SYS  ( SYS_GROUP_LOAD,           GroupLoad,          AIN_INT_TYPE,    4, AIN_STRING, AIN_STRING, AIN_STRING, AIN_REF_INT ),
	SYS  ( SYS_RESUME_WRITE_COMMENT, ResumeWriteComment, AIN_BOOL_TYPE,   3, AIN_STRING, AIN_STRING, AIN_REF_ARRAY_STRING ),
	SYS  ( SYS_RESUME_READ_COMMENT,  ResumeReadComment,  AIN_BOOL_TYPE,   3, AIN_STRING, AIN_STRING, AIN_REF_ARRAY_STRING ),
	SYS  ( SYS_DELETE_SAVE_FILE,     DeleteSaveFile,     AIN_INT_TYPE,    1, AIN_STRING ),
	SYS  ( SYS_EXIST_FUNC,           ExistFunc,          AIN_BOOL_TYPE,   1, AIN_STRING ),
	SYS  ( SYS_COPY_SAVE_FILE,       CopySaveFile,       AIN_INT_TYPE,    2, AIN_STRING, AIN_STRING ),
};

// JMP = instruction that modifies the instruction pointer
// OP  = everything else
#define _OP(code, opname, nargs, ...)		\
	[code] = {				\
		.opcode = code,			\
		.name = opname,			\
		.nr_args = nargs,		\
		.ip_inc = 2 + nargs * 4,	\
		.implemented = true,		\
		.args = { __VA_ARGS__ }		\
	}

#define OP(code, nargs, ...)			\
	[code] = {				\
		.opcode = code,			\
		.name = #code ,			\
		.nr_args = nargs,		\
		.ip_inc = 2 + nargs * 4,	\
		.implemented = true,		\
		.args = { __VA_ARGS__ }		\
	}
#define JMP(code, nargs, ...)			\
	[code] = {				\
		.opcode = code,			\
		.name = #code ,			\
		.nr_args = nargs,		\
		.ip_inc = 0,			\
		.implemented = true,		\
		.args = { __VA_ARGS__ }		\
	}

#define TODO(code, nargs, ...)			\
	[code] = {				\
		.opcode = code,			\
		.name = #code,			\
		.nr_args = nargs,		\
		.ip_inc = 2 + nargs * 4,	\
		.implemented = false,		\
		.args = { __VA_ARGS__ }		\
	}

struct instruction instructions[NR_OPCODES] = {
//      TYPE   OPCODE          NR_ARGS, ARG_TYPES...
	OP   ( PUSH,           1, T_INT ),
        OP   ( POP,            0 ),
        OP   ( REF,            0 ),
        OP   ( REFREF,         0 ),
        OP   ( PUSHGLOBALPAGE, 0 ),
        OP   ( PUSHLOCALPAGE,  0 ),
        OP   ( INV,            0 ),
        OP   ( NOT,            0 ),
        OP   ( COMPL,          0 ),
        OP   ( ADD,            0 ),
        OP   ( SUB,            0 ),
        OP   ( MUL,            0 ),
        OP   ( DIV,            0 ),
        OP   ( MOD,            0 ),
        OP   ( AND,            0 ),
        OP   ( OR,             0 ),
        OP   ( XOR,            0 ),
        OP   ( LSHIFT,         0 ),
        OP   ( RSHIFT,         0 ),
        OP   ( LT,             0 ),
        OP   ( GT,             0 ),
        OP   ( LTE,            0 ),
        OP   ( GTE,            0 ),
        OP   ( NOTE,           0 ),
        OP   ( EQUALE,         0 ),
        OP   ( ASSIGN,         0 ),
        OP   ( PLUSA,          0 ),
        OP   ( MINUSA,         0 ),
        OP   ( MULA,           0 ),
        OP   ( DIVA,           0 ),
        OP   ( MODA,           0 ),
        OP   ( ANDA,           0 ),
        OP   ( ORA,            0 ),
        OP   ( XORA,           0 ),
        OP   ( LSHIFTA,        0 ),
        OP   ( RSHIFTA,        0 ),
        OP   ( F_ASSIGN,       0 ),
        OP   ( F_PLUSA,        0 ),
        OP   ( F_MINUSA,       0 ),
        OP   ( F_MULA,         0 ),
        OP   ( F_DIVA,         0 ),
        OP   ( DUP2,           0 ),
        OP   ( DUP_X2,         0 ),
        TODO ( CMP,            0 ),
        JMP  ( JUMP,           1, T_ADDR ),
        JMP  ( IFZ,            1, T_ADDR ),
        JMP  ( IFNZ,           1, T_ADDR ),
        JMP  ( RETURN,         0 ),
        JMP  ( CALLFUNC,       1, T_FUNC ),
        OP   ( INC,            0 ),
        OP   ( DEC,            0 ),
        OP   ( FTOI,           0 ),
        OP   ( ITOF,           0 ),
        OP   ( F_INV,          0 ),
        OP   ( F_ADD,          0 ),
        OP   ( F_SUB,          0 ),
        OP   ( F_MUL,          0 ),
        OP   ( F_DIV,          0 ),
        OP   ( F_LT,           0 ),
        OP   ( F_GT,           0 ),
        OP   ( F_LTE,          0 ),
        OP   ( F_GTE,          0 ),
        OP   ( F_NOTE,         0 ),
        OP   ( F_EQUALE,       0 ),
        OP   ( F_PUSH,         1, T_FLOAT ),
        OP   ( S_PUSH,         1, T_STRING ),
        OP   ( S_POP,          0 ),
        OP   ( S_ADD,          0 ),
        OP   ( S_ASSIGN,       0 ),
        TODO ( S_PLUSA,        0 ),
        OP   ( S_REF,          0 ),
        TODO ( S_REFREF,       0 ),
        OP   ( S_NOTE,         0 ),
        OP   ( S_EQUALE,       0 ),
        TODO ( SF_CREATE,      0 ),
        TODO ( SF_CREATEPIXEL, 0 ),
        TODO ( SF_CREATEALPHA, 0 ),

        OP   ( SR_POP,         0 ),
        OP   ( SR_ASSIGN,      0 ),
        OP   ( SR_REF,         1, T_STRUCT ),
        TODO ( SR_REFREF,      0 ),
        OP   ( A_ALLOC,        0 ),
        OP   ( A_REALLOC,      0 ),
        OP   ( A_FREE,         0 ),
        OP   ( A_NUMOF,        0 ),
        OP   ( A_COPY,         0 ),
        OP   ( A_FILL,         0 ),
        OP   ( C_REF,          0 ),
        OP   ( C_ASSIGN,       0 ),
        JMP  ( MSG,            1, T_MSG ),
        OP   ( CALLHLL,        2, T_HLL, T_HLLFUNC, T_INT ), // XXX: changed in ain version > 8
        OP   ( PUSHSTRUCTPAGE, 0 ),
        JMP  ( CALLMETHOD,     1, T_FUNC ),
        OP   ( SH_GLOBALREF,   1, T_GLOBAL ),
        OP   ( SH_LOCALREF,    1, T_LOCAL ),
        JMP  ( SWITCH,         1, T_SWITCH ),
        JMP  ( STRSWITCH,      1, T_SWITCH ),
        OP   ( FUNC,           1, T_FUNC ),
        _OP  ( _EOF, "EOF",    1, T_FILE ),
        OP   ( CALLSYS,        1, T_SYSCALL ),
        JMP  ( SJUMP,          0 ),
        OP   ( CALLONJUMP,     0 ),
        OP   ( SWAP,           0 ),
        OP   ( SH_STRUCTREF,   1, T_MEMB ),
        OP   ( S_LENGTH,       0 ),
        OP   ( S_LENGTHBYTE,   0 ),
        OP   ( I_STRING,       0 ),
        JMP  ( CALLFUNC2,      0 ),
        OP   ( DUP2_X1,        0 ),
        OP   ( R_ASSIGN,       0 ),
        OP   ( FT_ASSIGNS,     0 ),
        OP   ( ASSERT,         0 ),
        OP   ( S_LT,           0 ),
        OP   ( S_GT,           0 ),
        OP   ( S_LTE,          0 ),
        OP   ( S_GTE,          0 ),
        OP   ( S_LENGTH2,      0 ),
        TODO ( S_LENGTHBYTE2,  0 ),
        OP   ( NEW,            0, T_STRUCT, T_INT ), // FIXME: 2nd arg is T_FUNC *OR* -1
        OP   ( DELETE,         0 ),
        TODO ( CHECKUDO,       0 ),
        OP   ( A_REF,          0 ),
        OP   ( DUP,            0 ),
        OP   ( DUP_U2,         0 ),
        OP   ( SP_INC,         0 ),
        TODO ( SP_DEC,         0 ),
        OP   ( ENDFUNC,        1, T_FUNC ),
        TODO ( R_EQUALE,       0 ),
        TODO ( R_NOTE,         0 ),
        OP   ( SH_LOCALCREATE, 2, T_LOCAL, T_STRUCT ),
        OP   ( SH_LOCALDELETE, 1, T_LOCAL ),
        OP   ( STOI,           0 ),
        OP   ( A_PUSHBACK,     0 ),
        OP   ( A_POPBACK,      0 ),
        OP   ( S_EMPTY,        0 ),
        OP   ( A_EMPTY,        0 ),
        OP   ( A_ERASE,        0 ),
        OP   ( A_INSERT,       0 ),
        OP   ( SH_LOCALINC,    1, T_LOCAL ),
        OP   ( SH_LOCALDEC,    1, T_LOCAL ),
        OP   ( SH_LOCALASSIGN, 2, T_LOCAL, T_INT ),
        OP   ( ITOB,           0 ),
        OP   ( S_FIND,         0 ),
        OP   ( S_GETPART,      0 ),
        OP   ( A_SORT,         0 ),
        TODO ( S_PUSHBACK,     0 ),
        TODO ( S_POPBACK,      0 ),
        OP   ( FTOS,           0 ),
        OP   ( S_MOD,          0, T_INT ), // XXX: changed in ain version > 8
        OP   ( S_PLUSA2,       0 ),
        OP   ( OBJSWAP,        0, T_INT ), // XXX: changed in ain version > 8
        TODO ( S_ERASE,        0 ),
        TODO ( SR_REF2,        1, T_INT ),
        OP   ( S_ERASE2,       0 ),
        OP   ( S_PUSHBACK2,    0 ),
        OP   ( S_POPBACK2,     0 ),
        OP   ( ITOLI,          0 ),
        OP   ( LI_ADD,         0 ),
        OP   ( LI_SUB,         0 ),
        OP   ( LI_MUL,         0 ),
        OP   ( LI_DIV,         0 ),
        OP   ( LI_MOD,         0 ),
        OP   ( LI_ASSIGN,      0 ),
        OP   ( LI_PLUSA,       0 ),
        OP   ( LI_MINUSA,      0 ),
        OP   ( LI_MULA,        0 ),
        OP   ( LI_DIVA,        0 ),
        OP   ( LI_MODA,        0 ),
        OP   ( LI_ANDA,        0 ),
        OP   ( LI_ORA,         0 ),
        OP   ( LI_XORA,        0 ),
        OP   ( LI_LSHIFTA,     0 ),
        OP   ( LI_RSHIFTA,     0 ),
        OP   ( LI_INC,         0 ),
        OP   ( LI_DEC,         0 ),
        OP   ( A_FIND,         0 ),
        OP   ( A_REVERSE,      0 ),

        OP   ( SH_SR_ASSIGN, 0 ),
        OP   ( SH_MEM_ASSIGN_LOCAL, 2, T_MEMB, T_LOCAL ),
        OP   ( A_NUMOF_GLOB_1, 1, T_GLOBAL ),
        OP   ( A_NUMOF_STRUCT_1, 1, T_MEMB ),
        OP   ( SH_MEM_ASSIGN_IMM, 2, T_MEMB, T_INT ),
        OP   ( SH_LOCALREFREF, 1, T_LOCAL ),
        OP   ( SH_LOCALASSIGN_SUB_IMM, 2, T_LOCAL, T_INT ),
        JMP  ( SH_IF_LOC_LT_IMM, 3, T_LOCAL, T_INT, T_ADDR ),
        JMP  ( SH_IF_LOC_GE_IMM, 3, T_LOCAL, T_INT, T_ADDR ),
        OP   ( SH_LOCREF_ASSIGN_MEM, 2, T_LOCAL, T_MEMB ),
        OP   ( PAGE_REF, 1, T_INT ),
        OP   ( SH_GLOBAL_ASSIGN_LOCAL, 2, T_GLOBAL, T_LOCAL ),
        TODO ( SH_STRUCTREF_GT_IMM, 2, T_MEMB, T_INT ),
        TODO ( SH_STRUCT_ASSIGN_LOCALREF_ITOB, 2, T_MEMB, T_LOCAL ),
        OP   ( SH_LOCAL_ASSIGN_STRUCTREF, 2, T_LOCAL, T_MEMB ),
        JMP  ( SH_IF_STRUCTREF_NE_LOCALREF, 3, T_MEMB, T_LOCAL, T_ADDR ),
        JMP  ( SH_IF_STRUCTREF_GT_IMM, 3, T_MEMB, T_INT, T_ADDR ),
        JMP  ( SH_STRUCTREF_CALLMETHOD_NO_PARAM, 2, T_MEMB, T_FUNC ),
        OP   ( SH_STRUCTREF2, 2, T_MEMB, T_MEMB2 ),
        OP   ( SH_REF_STRUCTREF2, 2, T_MEMB, T_MEMB2 ),
        OP   ( SH_STRUCTREF3, 3, T_MEMB, T_MEMB2, T_MEMB3 ),
        JMP  ( SH_STRUCTREF2_CALLMETHOD_NO_PARAM, 3, T_MEMB, T_MEMB2, T_FUNC ),
        JMP  ( SH_IF_STRUCTREF_Z, 2, T_MEMB, T_ADDR ),
        JMP  ( SH_IF_STRUCT_A_NOT_EMPTY, 2, T_MEMB, T_ADDR ),
        JMP  ( SH_IF_LOC_GT_IMM, 3, T_LOCAL, T_INT, T_ADDR ),
        JMP  ( SH_IF_STRUCTREF_NE_IMM, 3, T_MEMB, T_INT, T_ADDR ),
        JMP  ( THISCALLMETHOD_NOPARAM, 1, T_FUNC ),
        JMP  ( SH_IF_LOC_NE_IMM, 3, T_LOCAL, T_INT, T_ADDR ),
        JMP  ( SH_IF_STRUCTREF_EQ_IMM, 3, T_MEMB, T_INT, T_ADDR ),
        OP   ( SH_GLOBAL_ASSIGN_IMM, 2, T_GLOBAL, T_INT ),
        OP   ( SH_LOCALSTRUCT_ASSIGN_IMM, 3, T_LOCAL, T_LOCMEMB, T_INT ),
        OP   ( SH_STRUCT_A_PUSHBACK_LOCAL_STRUCT, 2, T_MEMB, T_LOCAL ),
        OP   ( SH_GLOBAL_A_PUSHBACK_LOCAL_STRUCT, 2, T_GLOBAL, T_LOCAL ),
        OP   ( SH_LOCAL_A_PUSHBACK_LOCAL_STRUCT, 2, T_LOCAL, T_LOCAL ),
        JMP  ( SH_IF_SREF_NE_STR0, 2, T_STRING, T_ADDR ),
        OP   ( SH_S_ASSIGN_REF, 0 ),
        TODO ( SH_A_FIND_SREF, 0 ),
        OP   ( SH_SREF_EMPTY, 0 ),
        OP   ( SH_STRUCTSREF_EQ_LOCALSREF, 2 , T_MEMB, T_LOCAL ),
        OP   ( SH_LOCALSREF_EQ_STR0, 2, T_LOCAL, T_STRING ),
        OP   ( SH_STRUCTSREF_NE_LOCALSREF, 2, T_MEMB, T_LOCAL ),
        OP   ( SH_LOCALSREF_NE_STR0, 2, T_LOCAL, T_STRING ),
        OP   ( SH_STRUCT_SR_REF, 2, T_MEMB, T_STRUCT ),
        OP   ( SH_STRUCT_S_REF, 1, T_MEMB ),
        OP   ( S_REF2, 1, T_MEMB ),
        OP   ( SH_REF_LOCAL_ASSIGN_STRUCTREF2, 3, T_MEMB, T_LOCAL, T_MEMB2 ),
        OP   ( SH_GLOBAL_S_REF, 1, T_GLOBAL ),
        OP   ( SH_LOCAL_S_REF, 1, T_LOCAL ),
        OP   ( SH_LOCALREF_SASSIGN_LOCALSREF, 2, T_LOCAL, T_LOCAL ),
        OP   ( SH_LOCAL_APUSHBACK_LOCALSREF, 2, T_LOCAL, T_LOCAL ),
        OP   ( SH_S_ASSIGN_CALLSYS19, 0 ),
        OP   ( SH_S_ASSIGN_STR0, 1, T_STRING ),
        OP   ( SH_SASSIGN_LOCALSREF, 1, T_LOCAL ),
        OP   ( SH_STRUCTREF_SASSIGN_LOCALSREF, 2, T_MEMB, T_LOCAL ),
        OP   ( SH_LOCALSREF_EMPTY, 1, T_LOCAL ),
        OP   ( SH_GLOBAL_APUSHBACK_LOCALSREF, 2, T_GLOBAL, T_LOCAL ),
        OP   ( SH_STRUCT_APUSHBACK_LOCALSREF, 2, T_MEMB, T_LOCAL ),
        OP   ( SH_STRUCTSREF_EMPTY, 1, T_MEMB ),
        OP   ( SH_GLOBALSREF_EMPTY, 1, T_GLOBAL ),
        TODO ( SH_SASSIGN_STRUCTSREF, 1, T_MEMB ),
        TODO ( SH_SASSIGN_GLOBALSREF, 1, T_GLOBAL ),
        TODO ( SH_STRUCTSREF_NE_STR0, 2, T_MEMB, T_STRING ),
        TODO ( SH_GLOBALSREF_NE_STR0, 2, T_GLOBAL, T_STRING ),
        TODO ( SH_LOC_LT_IMM_OR_LOC_GE_IMM, 3, T_LOCAL, T_INT, T_INT ),

        TODO ( A_SORT_MEM, 0 ),
        TODO ( DG_ADD, 0 ),
        TODO ( DG_SET, 0 ),
        TODO ( DG_CALL, 2, T_DLG, T_ADDR ),
        TODO ( DG_NUMOF, 0 ),
        TODO ( DG_EXIST, 0 ),
        TODO ( DG_ERASE, 0 ),
        TODO ( DG_CLEAR, 0 ),
        TODO ( DG_COPY, 0 ),
        TODO ( DG_ASSIGN, 0 ),
        TODO ( DG_PLUSA, 0 ),
        TODO ( DG_POP, 0 ),
        TODO ( DG_NEW_FROM_METHOD, 0 ),
        TODO ( DG_MINUSA, 0 ),
        TODO ( DG_CALLBEGIN, 1, T_DLG ),
        TODO ( DG_NEW, 0 ),
        TODO ( DG_STR_TO_METHOD, 0, T_DLG ), // XXX: changed in ain version > 8

	TODO ( OP_0X102, 0 ),
	TODO ( X_GETENV, 0 ),
	TODO ( X_SET,    0 ),
	TODO ( X_ICAST,  1, T_STRUCT ),
	TODO ( X_OP_SET, 1, T_INT ),
	TODO ( OP_0X107, 0 ),
	TODO ( OP_0X108, 0 ),
	TODO ( OP_0X109, 0 ),
	TODO ( X_DUP,    1, T_INT ),
	TODO ( X_MOV,    2, T_INT, T_INT ),
	TODO ( X_REF,    1, T_INT ),
	TODO ( X_ASSIGN, 1, T_INT ),
	TODO ( X_A_INIT, 1, T_INT ),
	TODO ( X_A_SIZE, 0 ),
	TODO ( X_TO_STR, 1, T_INT ),
};

void initialize_instructions(void)
{
	// this is a dumb hack to force linking/initialization of the above
	// arrays on Windows.
}
