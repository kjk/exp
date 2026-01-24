// Symbol kind constants from cvinfo.h
// https://github.com/Microsoft/microsoft-pdb/blob/082c5290e5aff028ae84e43affa8be717aa7af73/include/cvinfo.h#L2735

export const S_COMPILE = 0x0001;
export const S_REGISTER_16t = 0x0002;
export const S_CONSTANT_16t = 0x0003;
export const S_UDT_16t = 0x0004;
export const S_SSEARCH = 0x0005;
export const S_END = 0x0006;
export const S_SKIP = 0x0007;
export const S_CVRESERVE = 0x0008;
export const S_OBJNAME_ST = 0x0009;
export const S_ENDARG = 0x000a;
export const S_COBOLUDT_16t = 0x000b;
export const S_MANYREG_16t = 0x000c;
export const S_RETURN = 0x000d;
export const S_ENTRYTHIS = 0x000e;

export const S_BPREL16 = 0x0100;
export const S_LDATA16 = 0x0101;
export const S_GDATA16 = 0x0102;
export const S_PUB16 = 0x0103;
export const S_LPROC16 = 0x0104;
export const S_GPROC16 = 0x0105;
export const S_THUNK16 = 0x0106;
export const S_BLOCK16 = 0x0107;
export const S_WITH16 = 0x0108;
export const S_LABEL16 = 0x0109;
export const S_CEXMODEL16 = 0x010a;
export const S_VFTABLE16 = 0x010b;
export const S_REGREL16 = 0x010c;

export const S_BPREL32_16t = 0x0200;
export const S_LDATA32_16t = 0x0201;
export const S_GDATA32_16t = 0x0202;
export const S_PUB32_16t = 0x0203;
export const S_LPROC32_16t = 0x0204;
export const S_GPROC32_16t = 0x0205;
export const S_THUNK32_ST = 0x0206;
export const S_BLOCK32_ST = 0x0207;
export const S_WITH32_ST = 0x0208;
export const S_LABEL32_ST = 0x0209;
export const S_CEXMODEL32 = 0x020a;
export const S_VFTABLE32_16t = 0x020b;
export const S_REGREL32_16t = 0x020c;
export const S_LTHREAD32_16t = 0x020d;
export const S_GTHREAD32_16t = 0x020e;
export const S_SLINK32 = 0x020f;

export const S_LPROCMIPS_16t = 0x0300;
export const S_GPROCMIPS_16t = 0x0301;

export const S_PROCREF_ST = 0x0400;
export const S_DATAREF_ST = 0x0401;
export const S_ALIGN = 0x0402;
export const S_LPROCREF_ST = 0x0403;
export const S_OEM = 0x0404;

// 32-bit type index versions (0x1000 bit set)
export const S_TI16_MAX = 0x1000;

export const S_REGISTER_ST = 0x1001;
export const S_CONSTANT_ST = 0x1002;
export const S_UDT_ST = 0x1003;
export const S_COBOLUDT_ST = 0x1004;
export const S_MANYREG_ST = 0x1005;
export const S_BPREL32_ST = 0x1006;
export const S_LDATA32_ST = 0x1007;
export const S_GDATA32_ST = 0x1008;
export const S_PUB32_ST = 0x1009;
export const S_LPROC32_ST = 0x100a;
export const S_GPROC32_ST = 0x100b;
export const S_VFTABLE32 = 0x100c;
export const S_REGREL32_ST = 0x100d;
export const S_LTHREAD32_ST = 0x100e;
export const S_GTHREAD32_ST = 0x100f;

export const S_LPROCMIPS_ST = 0x1010;
export const S_GPROCMIPS_ST = 0x1011;
export const S_FRAMEPROC = 0x1012;
export const S_COMPILE2_ST = 0x1013;
export const S_MANYREG2_ST = 0x1014;
export const S_LPROCIA64_ST = 0x1015;
export const S_GPROCIA64_ST = 0x1016;
export const S_LOCALSLOT_ST = 0x1017;
export const S_PARAMSLOT_ST = 0x1018;
export const S_ANNOTATION = 0x1019;
export const S_GMANPROC_ST = 0x101a;
export const S_LMANPROC_ST = 0x101b;
export const S_RESERVED1 = 0x101c;
export const S_RESERVED2 = 0x101d;
export const S_RESERVED3 = 0x101e;
export const S_RESERVED4 = 0x101f;
export const S_LMANDATA_ST = 0x1020;
export const S_GMANDATA_ST = 0x1021;
export const S_MANFRAMEREL_ST = 0x1022;
export const S_MANREGISTER_ST = 0x1023;
export const S_MANSLOT_ST = 0x1024;
export const S_MANMANYREG_ST = 0x1025;
export const S_MANREGREL_ST = 0x1026;
export const S_MANMANYREG2_ST = 0x1027;
export const S_MANTYPREF = 0x1028;
export const S_UNAMESPACE_ST = 0x1029;

// SZ name symbols (utf8 null-terminated)
export const S_ST_MAX = 0x1100;

export const S_OBJNAME = 0x1101;
export const S_THUNK32 = 0x1102;
export const S_BLOCK32 = 0x1103;
export const S_WITH32 = 0x1104;
export const S_LABEL32 = 0x1105;
export const S_REGISTER = 0x1106;
export const S_CONSTANT = 0x1107;
export const S_UDT = 0x1108;
export const S_COBOLUDT = 0x1109;
export const S_MANYREG = 0x110a;
export const S_BPREL32 = 0x110b;
export const S_LDATA32 = 0x110c;
export const S_GDATA32 = 0x110d;
export const S_PUB32 = 0x110e;
export const S_LPROC32 = 0x110f;
export const S_GPROC32 = 0x1110;
export const S_REGREL32 = 0x1111;
export const S_LTHREAD32 = 0x1112;
export const S_GTHREAD32 = 0x1113;

export const S_LPROCMIPS = 0x1114;
export const S_GPROCMIPS = 0x1115;
export const S_COMPILE2 = 0x1116;
export const S_MANYREG2 = 0x1117;
export const S_LPROCIA64 = 0x1118;
export const S_GPROCIA64 = 0x1119;
export const S_LOCALSLOT = 0x111a;
export const S_PARAMSLOT = 0x111b;
export const S_LMANDATA = 0x111c;
export const S_GMANDATA = 0x111d;
export const S_MANFRAMEREL = 0x111e;
export const S_MANREGISTER = 0x111f;
export const S_MANSLOT = 0x1120;
export const S_MANMANYREG = 0x1121;
export const S_MANREGREL = 0x1122;
export const S_MANMANYREG2 = 0x1123;
export const S_UNAMESPACE = 0x1124;

export const S_PROCREF = 0x1125;
export const S_DATAREF = 0x1126;
export const S_LPROCREF = 0x1127;
export const S_ANNOTATIONREF = 0x1128;
export const S_TOKENREF = 0x1129;
export const S_GMANPROC = 0x112a;
export const S_LMANPROC = 0x112b;
export const S_TRAMPOLINE = 0x112c;
export const S_MANCONSTANT = 0x112d;
export const S_ATTR_FRAMEREL = 0x112e;
export const S_ATTR_REGISTER = 0x112f;
export const S_ATTR_REGREL = 0x1130;
export const S_ATTR_MANYREG = 0x1131;
export const S_SEPCODE = 0x1132;
export const S_LOCAL_2005 = 0x1133;
export const S_DEFRANGE_2005 = 0x1134;
export const S_DEFRANGE2_2005 = 0x1135;
export const S_SECTION = 0x1136;
export const S_COFFGROUP = 0x1137;
export const S_EXPORT = 0x1138;
export const S_CALLSITEINFO = 0x1139;
export const S_FRAMECOOKIE = 0x113a;
export const S_DISCARDED = 0x113b;
export const S_COMPILE3 = 0x113c;
export const S_ENVBLOCK = 0x113d;
export const S_LOCAL = 0x113e;
export const S_DEFRANGE = 0x113f;
export const S_DEFRANGE_SUBFIELD = 0x1140;
export const S_DEFRANGE_REGISTER = 0x1141;
export const S_DEFRANGE_FRAMEPOINTER_REL = 0x1142;
export const S_DEFRANGE_SUBFIELD_REGISTER = 0x1143;
export const S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE = 0x1144;
export const S_DEFRANGE_REGISTER_REL = 0x1145;
export const S_LPROC32_ID = 0x1146;
export const S_GPROC32_ID = 0x1147;
export const S_LPROCMIPS_ID = 0x1148;
export const S_GPROCMIPS_ID = 0x1149;
export const S_LPROCIA64_ID = 0x114a;
export const S_GPROCIA64_ID = 0x114b;
export const S_BUILDINFO = 0x114c;
export const S_INLINESITE = 0x114d;
export const S_INLINESITE_END = 0x114e;
export const S_PROC_ID_END = 0x114f;
export const S_DEFRANGE_HLSL = 0x1150;
export const S_GDATA_HLSL = 0x1151;
export const S_LDATA_HLSL = 0x1152;
export const S_FILESTATIC = 0x1153;
export const S_LOCAL_DPC_GROUPSHARED = 0x1154;
export const S_LPROC32_DPC = 0x1155;
export const S_LPROC32_DPC_ID = 0x1156;
export const S_DEFRANGE_DPC_PTR_TAG = 0x1157;
export const S_DPC_SYM_TAG_MAP = 0x1158;
export const S_ARMSWITCHTABLE = 0x1159;
export const S_CALLEES = 0x115a;
export const S_CALLERS = 0x115b;
export const S_POGODATA = 0x115c;
export const S_INLINESITE2 = 0x115d;
export const S_HEAPALLOCSITE = 0x115e;
export const S_MOD_TYPEREF = 0x115f;
export const S_REF_MINIPDB = 0x1160;
export const S_PDBMAP = 0x1161;
export const S_GDATA_HLSL32 = 0x1162;
export const S_LDATA_HLSL32 = 0x1163;
export const S_GDATA_HLSL32_EX = 0x1164;
export const S_LDATA_HLSL32_EX = 0x1165;
export const S_FASTLINK = 0x1167;
export const S_INLINEES = 0x1168;

/** CV_CPU_TYPE_e enumeration. */
export enum CPUType {
  Intel8080 = 0x00,
  Intel8086 = 0x01,
  Intel80286 = 0x02,
  Intel80386 = 0x03,
  Intel80486 = 0x04,
  Pentium = 0x05,
  PentiumPro = 0x06,
  Pentium3 = 0x07,
  MIPS = 0x10,
  MIPS16 = 0x11,
  MIPS32 = 0x12,
  MIPS64 = 0x13,
  MIPSI = 0x14,
  MIPSII = 0x15,
  MIPSIII = 0x16,
  MIPSIV = 0x17,
  MIPSV = 0x18,
  M68000 = 0x20,
  M68010 = 0x21,
  M68020 = 0x22,
  M68030 = 0x23,
  M68040 = 0x24,
  Alpha = 0x30,
  Alpha21164 = 0x31,
  Alpha21164A = 0x32,
  Alpha21264 = 0x33,
  Alpha21364 = 0x34,
  PPC601 = 0x40,
  PPC603 = 0x41,
  PPC604 = 0x42,
  PPC620 = 0x43,
  PPCFP = 0x44,
  PPCBE = 0x45,
  SH3 = 0x50,
  SH3E = 0x51,
  SH3DSP = 0x52,
  SH4 = 0x53,
  SHMedia = 0x54,
  ARM3 = 0x60,
  ARM4 = 0x61,
  ARM4T = 0x62,
  ARM5 = 0x63,
  ARM5T = 0x64,
  ARM6 = 0x65,
  ARM_XMAC = 0x66,
  ARM_WMMX = 0x67,
  ARM7 = 0x68,
  ARM64 = 0x69,
  Omni = 0x70,
  Ia64 = 0x80,
  Ia64_2 = 0x81,
  CEE = 0x90,
  AM33 = 0xa0,
  M32R = 0xb0,
  TriCore = 0xc0,
  X64 = 0xd0,
  EBC = 0xe0,
  Thumb = 0xf0,
  ARMNT = 0xf4,
  D3D11_Shader = 0x100,
}

/** Convert a raw u16 to a CPUType. */
export function cpuTypeFromU16(value: number): CPUType {
  const known: number[] = Object.values(CPUType).filter(
    (v) => typeof v === "number",
  ) as number[];
  if (known.includes(value)) {
    return value as CPUType;
  }
  return CPUType.Intel8080;
}

/** CV_CFL_LANG enumeration - compiler source languages. */
export enum SourceLanguage {
  C = 0x00,
  Cpp = 0x01,
  Fortran = 0x02,
  Masm = 0x03,
  Pascal = 0x04,
  Basic = 0x05,
  Cobol = 0x06,
  Link = 0x07,
  Cvtres = 0x08,
  Cvtpgd = 0x09,
  CSharp = 0x0a,
  VB = 0x0b,
  ILAsm = 0x0c,
  Java = 0x0d,
  JScript = 0x0e,
  MSIL = 0x0f,
  HLSL = 0x10,
  D = 0x44,
}

/** Convert a raw u8 to a SourceLanguage. */
export function sourceLanguageFromU8(value: number): SourceLanguage {
  const known: number[] = Object.values(SourceLanguage).filter(
    (v) => typeof v === "number",
  ) as number[];
  if (known.includes(value)) {
    return value as SourceLanguage;
  }
  return SourceLanguage.Masm;
}

/** Thunk type for S_THUNK32. */
export enum ThunkKind {
  NoType = 0,
  Adjustor = 1,
  VCall = 2,
  PCode = 3,
  UnknownLoad = 4,
  TrampIncremental = 5,
  BranchIsland = 6,
}

/** Trampoline type for S_TRAMPOLINE. */
export enum TrampolineType {
  Incremental = 0,
  BranchIsland = 1,
}

/** CV_cookietype_e - Frame cookie type for S_FRAMECOOKIE. */
export enum FrameCookieType {
  Copy = 0,
  XorStackPointer = 1,
  XorBasePointer = 2,
  XorR13 = 3,
}

/** CV_DISCARDED_e - Reason a symbol was discarded. */
export enum DiscardedReason {
  Unknown = 0,
  NotSelected = 1,
  NotReferenced = 2,
}

/** CV_LABEL_TYPE_e - Label address mode. */
export enum LabelType {
  Near = 0,
  Far = 4,
}

/** CV_armswitchtype - ARM switch table entry type. */
export enum ArmSwitchType {
  Int1 = 0,
  UInt1 = 1,
  Int2 = 2,
  UInt2 = 3,
  Int4 = 4,
  UInt4 = 5,
  Pointer = 6,
  UInt1Shl1 = 7,
  UInt2Shl1 = 8,
  Int1Shl1 = 9,
  Int2Shl1 = 10,
}

/** CV_BuildInfo_e - Build information field indices. */
export enum BuildInfoIndex {
  CurrentDirectory = 0,
  BuildTool = 1,
  SourceFile = 2,
  ProgramDatabaseFile = 3,
  CommandArguments = 4,
}
