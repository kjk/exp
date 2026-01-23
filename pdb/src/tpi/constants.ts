// Type leaf kind constants from cvinfo.h

// 16-bit type index versions (legacy)
export const LF_MODIFIER_16t = 0x0001;
export const LF_POINTER_16t = 0x0002;
export const LF_ARRAY_16t = 0x0003;
export const LF_CLASS_16t = 0x0004;
export const LF_STRUCTURE_16t = 0x0005;
export const LF_UNION_16t = 0x0006;
export const LF_ENUM_16t = 0x0007;
export const LF_PROCEDURE_16t = 0x0008;
export const LF_MFUNCTION_16t = 0x0009;
export const LF_VTSHAPE = 0x000a;

export const LF_LABEL = 0x000e;

// 16-bit field list indices
export const LF_BCLASS_16t = 0x0400;
export const LF_VBCLASS_16t = 0x0401;
export const LF_IVBCLASS_16t = 0x0402;
export const LF_ENUMERATE_ST = 0x0403;
export const LF_INDEX_16t = 0x0405;
export const LF_MEMBER_16t = 0x0406;
export const LF_STMEMBER_16t = 0x0407;
export const LF_METHOD_16t = 0x0408;
export const LF_NESTTYPE_16t = 0x0409;
export const LF_VFUNCTAB_16t = 0x040a;
export const LF_ONEMETHOD_16t = 0x040c;

// 32-bit type index versions
export const LF_TI16_MAX = 0x1000;

export const LF_MODIFIER = 0x1001;
export const LF_POINTER = 0x1002;
export const LF_ARRAY_ST = 0x1003;
export const LF_CLASS_ST = 0x1004;
export const LF_STRUCTURE_ST = 0x1005;
export const LF_UNION_ST = 0x1006;
export const LF_ENUM_ST = 0x1007;
export const LF_PROCEDURE = 0x1008;
export const LF_MFUNCTION = 0x1009;
export const LF_COBOL0 = 0x100a;
export const LF_BARRAY = 0x100b;
export const LF_VFTPATH = 0x100d;

// Field list indices (32-bit type index)
export const LF_SKIP = 0x1200;
export const LF_ARGLIST = 0x1201;
export const LF_FIELDLIST = 0x1203;
export const LF_DERIVED = 0x1204;
export const LF_BITFIELD = 0x1205;
export const LF_METHODLIST = 0x1206;

export const LF_BCLASS = 0x1400;
export const LF_VBCLASS = 0x1401;
export const LF_IVBCLASS = 0x1402;
export const LF_INDEX = 0x1404;
export const LF_MEMBER_ST = 0x1405;
export const LF_STMEMBER_ST = 0x1406;
export const LF_METHOD_ST = 0x1407;
export const LF_NESTTYPE_ST = 0x1408;
export const LF_VFUNCTAB = 0x1409;
export const LF_ONEMETHOD_ST = 0x140b;
export const LF_VFUNCOFF = 0x140c;

// Types with SZ (null-terminated) names
export const LF_ST_MAX = 0x1500;

export const LF_TYPESERVER = 0x1501;
export const LF_ENUMERATE = 0x1502;
export const LF_ARRAY = 0x1503;
export const LF_CLASS = 0x1504;
export const LF_STRUCTURE = 0x1505;
export const LF_UNION = 0x1506;
export const LF_ENUM = 0x1507;
export const LF_DIMARRAY = 0x1508;
export const LF_PRECOMP = 0x1509;
export const LF_ALIAS = 0x150a;
export const LF_DEFARG = 0x150b;
export const LF_FRIENDFCN = 0x150c;
export const LF_MEMBER = 0x150d;
export const LF_STMEMBER = 0x150e;
export const LF_METHOD = 0x150f;
export const LF_NESTTYPE = 0x1510;
export const LF_ONEMETHOD = 0x1511;
export const LF_NESTTYPEEX = 0x1512;
export const LF_MEMBERMODIFY = 0x1513;
export const LF_MANAGED = 0x1514;
export const LF_TYPESERVER2 = 0x1515;

export const LF_STRIDED_ARRAY = 0x1516;
export const LF_HLSL = 0x1517;
export const LF_MODIFIER_EX = 0x1518;
export const LF_INTERFACE = 0x1519;
export const LF_BINTERFACE = 0x151a;
export const LF_VECTOR = 0x151b;
export const LF_MATRIX = 0x151c;
export const LF_VFTABLE = 0x151d;

export const LF_STRUCTURE19 = 0x1609;

// Id stream record kinds
export const LF_FUNC_ID = 0x1601;
export const LF_MFUNC_ID = 0x1602;
export const LF_BUILDINFO = 0x1603;
export const LF_SUBSTR_LIST = 0x1604;
export const LF_STRING_ID = 0x1605;
export const LF_UDT_SRC_LINE = 0x1606;
export const LF_UDT_MOD_SRC_LINE = 0x1607;

// Numeric leaf constants
export const LF_NUMERIC = 0x8000;
export const LF_CHAR = 0x8000;
export const LF_SHORT = 0x8001;
export const LF_USHORT = 0x8002;
export const LF_LONG = 0x8003;
export const LF_ULONG = 0x8004;
export const LF_REAL32 = 0x8005;
export const LF_REAL64 = 0x8006;
export const LF_REAL80 = 0x8007;
export const LF_REAL128 = 0x8008;
export const LF_QUADWORD = 0x8009;
export const LF_UQUADWORD = 0x800a;

// Padding bytes
export const LF_PAD0 = 0xf0;
