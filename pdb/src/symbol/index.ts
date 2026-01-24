import { ParseBuffer, type Variant } from "../buffer.js";
import { PdbError } from "../error.js";
import type { MsfStream } from "../msf.js";
import type {
  TypeIndex,
  IdIndex,
  PdbInternalSectionOffset,
  Register,
  SymbolIndex,
} from "../common.js";
import {
  S_END, S_SKIP, S_ALIGN, S_PROC_ID_END, S_INLINESITE_END, S_ST_MAX,
  S_OBJNAME_ST, S_OBJNAME,
  S_REGISTER_ST, S_REGISTER,
  S_CONSTANT_ST, S_CONSTANT,
  S_UDT_ST, S_UDT,
  S_LDATA32_ST, S_LDATA32, S_GDATA32_ST, S_GDATA32,
  S_LMANDATA, S_GMANDATA,
  S_PUB32_ST, S_PUB32,
  S_LPROC32_ST, S_LPROC32, S_GPROC32_ST, S_GPROC32,
  S_LPROC32_ID, S_GPROC32_ID,
  S_LPROC32_DPC, S_LPROC32_DPC_ID,
  S_LTHREAD32_ST, S_LTHREAD32, S_GTHREAD32_ST, S_GTHREAD32,
  S_PROCREF_ST, S_PROCREF, S_LPROCREF_ST, S_LPROCREF,
  S_DATAREF_ST, S_DATAREF,
  S_ANNOTATIONREF,
  S_TRAMPOLINE,
  S_EXPORT,
  S_LOCAL,
  S_BUILDINFO,
  S_INLINESITE, S_INLINESITE2,
  S_COMPILE2, S_COMPILE3,
  S_UNAMESPACE_ST, S_UNAMESPACE,
  S_BPREL32, S_BPREL32_ST,
  S_REGREL32, S_REGREL32_ST,
  S_BLOCK32_ST, S_BLOCK32,
  S_LABEL32_ST, S_LABEL32,
  S_THUNK32_ST, S_THUNK32,
  S_SEPCODE,
  S_FRAMEPROC,
  S_CALLSITEINFO,
  S_ENVBLOCK,
  S_SECTION,
  S_COFFGROUP,
  S_MANYREG_ST, S_MANYREG,
  S_MANYREG2_ST, S_MANYREG2,
  S_MANCONSTANT,
  S_DEFRANGE, S_DEFRANGE_SUBFIELD,
  S_DEFRANGE_REGISTER, S_DEFRANGE_FRAMEPOINTER_REL,
  S_DEFRANGE_SUBFIELD_REGISTER,
  S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE,
  S_DEFRANGE_REGISTER_REL,
  S_CALLEES, S_CALLERS, S_INLINEES,
  type CPUType, type SourceLanguage, type ThunkKind, type TrampolineType,
} from "./constants.js";
import { cpuTypeFromU16, sourceLanguageFromU8 } from "./constants.js";

// Re-export everything from sub-modules
export * from "./constants.js";
export * from "./annotations.js";

/** A raw symbol record. */
export interface RawSymbol {
  index: SymbolIndex;
  kind: number;
  data: Uint8Array;
}

// ─── Flag Structures ───

/** Procedure flags (CV_PROCFLAGS). */
export interface ProcedureFlags {
  nofpo: boolean;
  interrupt: boolean;
  farReturn: boolean;
  neverReturn: boolean;
  notReached: boolean;
  customCallingConvention: boolean;
  noInline: boolean;
  optimizedDebugInfo: boolean;
}

/** Parse a u8 into ProcedureFlags. */
function parseProcedureFlags(raw: number): ProcedureFlags {
  return {
    nofpo: (raw & 0x01) !== 0,
    interrupt: (raw & 0x02) !== 0,
    farReturn: (raw & 0x04) !== 0,
    neverReturn: (raw & 0x08) !== 0,
    notReached: (raw & 0x10) !== 0,
    customCallingConvention: (raw & 0x20) !== 0,
    noInline: (raw & 0x40) !== 0,
    optimizedDebugInfo: (raw & 0x80) !== 0,
  };
}

/** Compile flags from S_COMPILE2/S_COMPILE3. */
export interface CompileFlags {
  editAndContinue: boolean;
  noDebugInfo: boolean;
  linkTimeCodegen: boolean;
  noDataAlign: boolean;
  managed: boolean;
  securityChecks: boolean;
  hotPatch: boolean;
  cvtcil: boolean;
  msilModule: boolean;
  sdl: boolean;
  pgo: boolean;
  expModule: boolean;
}

/** Parse compile flags from bits 8..31 of the flags u32. */
function parseCompileFlags(raw: number): CompileFlags {
  const flags = raw >>> 8;
  return {
    editAndContinue: (flags & 0x01) !== 0,
    noDebugInfo: (flags & 0x02) !== 0,
    linkTimeCodegen: (flags & 0x04) !== 0,
    noDataAlign: (flags & 0x08) !== 0,
    managed: (flags & 0x10) !== 0,
    securityChecks: (flags & 0x20) !== 0,
    hotPatch: (flags & 0x40) !== 0,
    cvtcil: (flags & 0x80) !== 0,
    msilModule: (flags & 0x100) !== 0,
    sdl: (flags & 0x200) !== 0,
    pgo: (flags & 0x400) !== 0,
    expModule: (flags & 0x800) !== 0,
  };
}

/** Compiler version information. */
export interface CompilerVersion {
  major: number;
  minor: number;
  build: number;
  qfe: number | null;
}

/** Local variable flags. */
export interface LocalVariableFlags {
  isParam: boolean;
  addressTaken: boolean;
  compilerGenerated: boolean;
  isAggregate: boolean;
  isAliased: boolean;
  isAlias: boolean;
  isReturnValue: boolean;
  isOptimizedOut: boolean;
  isEnregisteredGlobal: boolean;
  isEnregisteredStatic: boolean;
}

/** Parse a u16 into LocalVariableFlags. */
function parseLocalVariableFlags(raw: number): LocalVariableFlags {
  return {
    isParam: (raw & 0x0001) !== 0,
    addressTaken: (raw & 0x0002) !== 0,
    compilerGenerated: (raw & 0x0004) !== 0,
    isAggregate: (raw & 0x0008) !== 0,
    isAliased: (raw & 0x0010) !== 0,
    isAlias: (raw & 0x0020) !== 0,
    isReturnValue: (raw & 0x0040) !== 0,
    isOptimizedOut: (raw & 0x0080) !== 0,
    isEnregisteredGlobal: (raw & 0x0100) !== 0,
    isEnregisteredStatic: (raw & 0x0200) !== 0,
  };
}

/** Export symbol flags. */
export interface ExportSymbolFlags {
  isConstant: boolean;
  isData: boolean;
  isPrivate: boolean;
  hasNoName: boolean;
  hasExplicitOrdinal: boolean;
  isForwarder: boolean;
}

/** Parse a u16 into ExportSymbolFlags. */
function parseExportSymbolFlags(raw: number): ExportSymbolFlags {
  return {
    isConstant: (raw & 0x0001) !== 0,
    isData: (raw & 0x0002) !== 0,
    isPrivate: (raw & 0x0004) !== 0,
    hasNoName: (raw & 0x0008) !== 0,
    hasExplicitOrdinal: (raw & 0x0010) !== 0,
    isForwarder: (raw & 0x0020) !== 0,
  };
}

/** Separated code flags. */
export interface SeparatedCodeFlags {
  isLexicalScope: boolean;
  returnsToParent: boolean;
}

/** Parse a u32 into SeparatedCodeFlags. */
function parseSeparatedCodeFlags(raw: number): SeparatedCodeFlags {
  return {
    isLexicalScope: (raw & 0x01) !== 0,
    returnsToParent: (raw & 0x02) !== 0,
  };
}

// ─── Symbol Data Types ───

export interface ObjNameSymbol {
  kind: "ObjName";
  signature: number;
  name: string;
}

export interface RegisterVariableSymbol {
  kind: "RegisterVariable";
  type: TypeIndex;
  register: Register;
  name: string;
}

export interface MultiRegisterVariableSymbol {
  kind: "MultiRegisterVariable";
  type: TypeIndex;
  registers: { register: Register; name: string }[];
}

export interface ConstantSymbol {
  kind: "Constant";
  managed: boolean;
  type: TypeIndex;
  value: Variant;
  name: string;
}

export interface UserDefinedTypeSymbol {
  kind: "UserDefinedType";
  type: TypeIndex;
  name: string;
}

export interface DataSymbol {
  kind: "Data";
  global: boolean;
  managed: boolean;
  type: TypeIndex;
  offset: PdbInternalSectionOffset;
  name: string;
}

export interface PublicSymbol {
  kind: "Public";
  code: boolean;
  function: boolean;
  managed: boolean;
  msil: boolean;
  offset: PdbInternalSectionOffset;
  name: string;
}

export interface ProcedureSymbol {
  kind: "Procedure";
  global: boolean;
  dpc: boolean;
  parent: SymbolIndex | null;
  end: SymbolIndex;
  next: SymbolIndex | null;
  len: number;
  dbgStart: number;
  dbgEnd: number;
  type: TypeIndex;
  offset: PdbInternalSectionOffset;
  flags: ProcedureFlags;
  name: string;
}

export interface ThreadStorageSymbol {
  kind: "ThreadStorage";
  global: boolean;
  type: TypeIndex;
  offset: PdbInternalSectionOffset;
  name: string;
}

export interface UsingNamespaceSymbol {
  kind: "UsingNamespace";
  name: string;
}

export interface ProcedureReferenceSymbol {
  kind: "ProcedureReference";
  global: boolean;
  sumName: number;
  symbolIndex: SymbolIndex;
  module: number;
  name: string | null;
}

export interface DataReferenceSymbol {
  kind: "DataReference";
  sumName: number;
  symbolIndex: SymbolIndex;
  module: number;
  name: string | null;
}

export interface AnnotationReferenceSymbol {
  kind: "AnnotationReference";
  sumName: number;
  symbolIndex: SymbolIndex;
  module: number;
  name: string;
}

export interface TrampolineSymbol {
  kind: "Trampoline";
  trampolineType: TrampolineType;
  thunkSize: number;
  thunkOffset: PdbInternalSectionOffset;
  targetOffset: PdbInternalSectionOffset;
}

export interface ExportSymbol {
  kind: "Export";
  ordinal: number;
  flags: ExportSymbolFlags;
  name: string;
}

export interface LocalSymbol {
  kind: "Local";
  type: TypeIndex;
  flags: LocalVariableFlags;
  name: string;
}

export interface BuildInfoSymbol {
  kind: "BuildInfo";
  id: IdIndex;
}

export interface InlineSiteSymbol {
  kind: "InlineSite";
  parent: SymbolIndex | null;
  end: SymbolIndex;
  inlinee: IdIndex;
  invocations: number | null;
  annotations: Uint8Array;
}

export interface BlockSymbol {
  kind: "Block";
  parent: SymbolIndex | null;
  end: SymbolIndex;
  len: number;
  offset: PdbInternalSectionOffset;
  name: string;
}

export interface LabelSymbol {
  kind: "Label";
  offset: PdbInternalSectionOffset;
  flags: ProcedureFlags;
  name: string;
}

export interface ScopeEndSymbol {
  kind: "ScopeEnd";
}

export interface BasePointerRelativeSymbol {
  kind: "BasePointerRelative";
  offset: number;
  type: TypeIndex;
  name: string;
}

export interface RegisterRelativeSymbol {
  kind: "RegisterRelative";
  offset: number;
  type: TypeIndex;
  register: Register;
  name: string;
}

export interface FrameProcedureSymbol {
  kind: "FrameProcedure";
  frameSize: number;
  paddingSize: number;
  paddingOffset: number;
  savedRegistersSize: number;
  exceptionHandlerOffset: number;
  exceptionHandlerSection: number;
  flags: number;
}

export interface CompileFlagsSymbol {
  kind: "CompileFlags";
  language: SourceLanguage;
  flags: CompileFlags;
  cpuType: CPUType;
  frontendVersion: CompilerVersion;
  backendVersion: CompilerVersion;
  compilerVersion: string;
}

export interface ThunkSymbol {
  kind: "Thunk";
  parent: SymbolIndex | null;
  end: SymbolIndex;
  next: SymbolIndex | null;
  offset: PdbInternalSectionOffset;
  len: number;
  thunkKind: ThunkKind;
  name: string;
}

export interface SeparatedCodeSymbol {
  kind: "SeparatedCode";
  parent: SymbolIndex | null;
  end: SymbolIndex;
  len: number;
  flags: SeparatedCodeFlags;
  offset: PdbInternalSectionOffset;
  parentOffset: PdbInternalSectionOffset;
}

export interface CallSiteInfoSymbol {
  kind: "CallSiteInfo";
  offset: PdbInternalSectionOffset;
  type: TypeIndex;
}

export interface EnvironmentBlockSymbol {
  kind: "EnvironmentBlock";
  entries: string[];
}

export interface SectionSymbol {
  kind: "Section";
  sectionNumber: number;
  alignment: number;
  rva: number;
  length: number;
  characteristics: number;
  name: string;
}

export interface CoffGroupSymbol {
  kind: "CoffGroup";
  length: number;
  characteristics: number;
  offset: PdbInternalSectionOffset;
  name: string;
}

// ─── DefRange Types ───

/** Address range where a variable definition is valid. */
export interface LocalAddressRange {
  offsetStart: number;
  sectionStart: number;
  length: number;
}

/** A gap in the address range where the variable is not available. */
export interface LocalAddressGap {
  startOffset: number;
  length: number;
}

export interface DefRangeSymbol {
  kind: "DefRange";
  program: number;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

export interface DefRangeSubfieldSymbol {
  kind: "DefRangeSubfield";
  program: number;
  offsetInParent: number;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

export interface DefRangeRegisterSymbol {
  kind: "DefRangeRegister";
  register: Register;
  mayHaveNoName: boolean;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

export interface DefRangeFramePointerRelSymbol {
  kind: "DefRangeFramePointerRel";
  offset: number;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

export interface DefRangeSubfieldRegisterSymbol {
  kind: "DefRangeSubfieldRegister";
  register: Register;
  mayHaveNoName: boolean;
  offsetInParent: number;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

export interface DefRangeFramePointerRelFullScopeSymbol {
  kind: "DefRangeFramePointerRelFullScope";
  offset: number;
}

export interface DefRangeRegisterRelSymbol {
  kind: "DefRangeRegisterRel";
  baseRegister: Register;
  spilledUdtMember: boolean;
  offsetInParent: number;
  basePointerOffset: number;
  range: LocalAddressRange;
  gaps: LocalAddressGap[];
}

/** List of called or calling functions (S_CALLEES / S_CALLERS). */
export interface FunctionListSymbol {
  kind: "Callees" | "Callers";
  functions: TypeIndex[];
  invocations: number[] | null;
}

/** List of inlined function IDs in a module (S_INLINEES). */
export interface InlineesSymbol {
  kind: "Inlinees";
  inlinees: IdIndex[];
}

export type SymbolData =
  | ObjNameSymbol
  | RegisterVariableSymbol
  | MultiRegisterVariableSymbol
  | ConstantSymbol
  | UserDefinedTypeSymbol
  | DataSymbol
  | PublicSymbol
  | ProcedureSymbol
  | ThreadStorageSymbol
  | UsingNamespaceSymbol
  | ProcedureReferenceSymbol
  | DataReferenceSymbol
  | AnnotationReferenceSymbol
  | TrampolineSymbol
  | ExportSymbol
  | LocalSymbol
  | BuildInfoSymbol
  | InlineSiteSymbol
  | BlockSymbol
  | LabelSymbol
  | ScopeEndSymbol
  | BasePointerRelativeSymbol
  | RegisterRelativeSymbol
  | FrameProcedureSymbol
  | CompileFlagsSymbol
  | ThunkSymbol
  | SeparatedCodeSymbol
  | CallSiteInfoSymbol
  | EnvironmentBlockSymbol
  | SectionSymbol
  | CoffGroupSymbol
  | DefRangeSymbol
  | DefRangeSubfieldSymbol
  | DefRangeRegisterSymbol
  | DefRangeFramePointerRelSymbol
  | DefRangeSubfieldRegisterSymbol
  | DefRangeFramePointerRelFullScopeSymbol
  | DefRangeRegisterRelSymbol
  | FunctionListSymbol
  | InlineesSymbol;

/** Iterator for symbol records, skipping alignment padding. */
export function* iterateSymbols(data: Uint8Array): Generator<RawSymbol> {
  const buf = new ParseBuffer(data);

  while (!buf.isEmpty) {
    const startPos = buf.pos;
    const length = buf.readU16();
    if (length < 2) {
      throw PdbError.symbolTooShort();
    }
    const recordData = buf.take(length);
    const kind = recordData[0] | (recordData[1] << 8);
    // Skip alignment/padding records
    if (kind === S_ALIGN || kind === S_SKIP) continue;
    yield { index: startPos, kind, data: recordData };
  }
}

/** Parse the global symbol table stream. */
export function parseSymbolTable(stream: MsfStream): RawSymbol[] {
  return [...iterateSymbols(stream.data)];
}

/** Parse a raw symbol into structured SymbolData. */
export function parseSymbolData(symbol: RawSymbol): SymbolData {
  const buf = new ParseBuffer(symbol.data);
  const kind = buf.readU16();
  return parseSymbolRecord(buf, kind);
}

/** Parse a symbol record body given its kind. */
function parseSymbolRecord(buf: ParseBuffer, kind: number): SymbolData {
  switch (kind) {
    case S_END:
    case S_PROC_ID_END:
    case S_INLINESITE_END:
      return { kind: "ScopeEnd" };

    case S_OBJNAME_ST:
    case S_OBJNAME: {
      const signature = buf.readU32();
      const name = parseSymbolName(buf, kind);
      return { kind: "ObjName", signature, name };
    }

    case S_REGISTER_ST:
    case S_REGISTER: {
      const type: TypeIndex = buf.readU32();
      const register: Register = buf.readU16();
      const name = parseSymbolName(buf, kind);
      return { kind: "RegisterVariable", type, register, name };
    }

    case S_MANYREG_ST:
    case S_MANYREG: {
      const type: TypeIndex = buf.readU32();
      const count = buf.readU8();
      const registers: { register: Register; name: string }[] = [];
      for (let i = 0; i < count; i++) {
        const register: Register = buf.readU8();
        const name = parseSymbolName(buf, kind);
        registers.push({ register, name });
      }
      return { kind: "MultiRegisterVariable", type, registers };
    }

    case S_MANYREG2_ST:
    case S_MANYREG2: {
      const type: TypeIndex = buf.readU32();
      const count = buf.readU16();
      const registers: { register: Register; name: string }[] = [];
      for (let i = 0; i < count; i++) {
        const register: Register = buf.readU16();
        const name = parseSymbolName(buf, kind);
        registers.push({ register, name });
      }
      return { kind: "MultiRegisterVariable", type, registers };
    }

    case S_CONSTANT_ST:
    case S_CONSTANT: {
      const type: TypeIndex = buf.readU32();
      const value = buf.readVariant();
      const name = parseSymbolName(buf, kind);
      return { kind: "Constant", managed: false, type, value, name };
    }

    case S_MANCONSTANT: {
      const type: TypeIndex = buf.readU32();
      const value = buf.readVariant();
      const name = buf.readCString();
      return { kind: "Constant", managed: true, type, value, name };
    }

    case S_UDT_ST:
    case S_UDT: {
      const type: TypeIndex = buf.readU32();
      const name = parseSymbolName(buf, kind);
      return { kind: "UserDefinedType", type, name };
    }

    case S_LDATA32_ST:
    case S_LDATA32:
    case S_GDATA32_ST:
    case S_GDATA32: {
      const global = kind === S_GDATA32 || kind === S_GDATA32_ST;
      const type: TypeIndex = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = parseSymbolName(buf, kind);
      return { kind: "Data", global, managed: false, type, offset, name };
    }

    case S_LMANDATA:
    case S_GMANDATA: {
      const global = kind === S_GMANDATA;
      const type: TypeIndex = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = buf.readCString();
      return { kind: "Data", global, managed: true, type, offset, name };
    }

    case S_PUB32_ST:
    case S_PUB32: {
      const flags = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = parseSymbolName(buf, kind);
      return {
        kind: "Public",
        code: (flags & 0x01) !== 0,
        function: (flags & 0x02) !== 0,
        managed: (flags & 0x04) !== 0,
        msil: (flags & 0x08) !== 0,
        offset,
        name,
      };
    }

    case S_LPROC32_ST:
    case S_LPROC32:
    case S_GPROC32_ST:
    case S_GPROC32:
    case S_LPROC32_ID:
    case S_GPROC32_ID:
    case S_LPROC32_DPC:
    case S_LPROC32_DPC_ID: {
      const global =
        kind === S_GPROC32 ||
        kind === S_GPROC32_ST ||
        kind === S_GPROC32_ID;
      const dpc =
        kind === S_LPROC32_DPC ||
        kind === S_LPROC32_DPC_ID;
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const next = readOptionalSymbolIndex(buf);
      const len = buf.readU32();
      const dbgStart = buf.readU32();
      const dbgEnd = buf.readU32();
      const type: TypeIndex = buf.readU32();
      const offset = readSectionOffset(buf);
      const flags = parseProcedureFlags(buf.readU8());
      const name = parseSymbolName(buf, kind);
      return {
        kind: "Procedure",
        global,
        dpc,
        parent,
        end,
        next,
        len,
        dbgStart,
        dbgEnd,
        type,
        offset,
        flags,
        name,
      };
    }

    case S_THUNK32_ST:
    case S_THUNK32: {
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const next = readOptionalSymbolIndex(buf);
      const offsetVal = buf.readU32();
      const section = buf.readU16();
      const len = buf.readU16();
      const thunkKind = buf.readU8() as ThunkKind;
      const name = parseSymbolName(buf, kind);
      return {
        kind: "Thunk",
        parent,
        end,
        next,
        offset: { offset: offsetVal, section },
        len,
        thunkKind,
        name,
      };
    }

    case S_SEPCODE: {
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const len = buf.readU32();
      const flags = parseSeparatedCodeFlags(buf.readU32());
      const offsetVal = buf.readU32();
      const parentOffsetVal = buf.readU32();
      const section = buf.readU16();
      const parentSection = buf.readU16();
      return {
        kind: "SeparatedCode",
        parent,
        end,
        len,
        flags,
        offset: { offset: offsetVal, section },
        parentOffset: { offset: parentOffsetVal, section: parentSection },
      };
    }

    case S_LTHREAD32_ST:
    case S_LTHREAD32:
    case S_GTHREAD32_ST:
    case S_GTHREAD32: {
      const global = kind === S_GTHREAD32 || kind === S_GTHREAD32_ST;
      const type: TypeIndex = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = parseSymbolName(buf, kind);
      return { kind: "ThreadStorage", global, type, offset, name };
    }

    case S_UNAMESPACE_ST:
    case S_UNAMESPACE: {
      const name = parseSymbolName(buf, kind);
      return { kind: "UsingNamespace", name };
    }

    case S_PROCREF_ST:
    case S_PROCREF:
    case S_LPROCREF_ST:
    case S_LPROCREF: {
      const global = kind === S_PROCREF || kind === S_PROCREF_ST;
      const sumName = buf.readU32();
      const symbolIndex: SymbolIndex = buf.readU32();
      const module = buf.readU16();
      const name = parseOptionalName(buf, kind);
      return { kind: "ProcedureReference", global, sumName, symbolIndex, module, name };
    }

    case S_DATAREF_ST:
    case S_DATAREF: {
      const sumName = buf.readU32();
      const symbolIndex: SymbolIndex = buf.readU32();
      const module = buf.readU16();
      const name = parseOptionalName(buf, kind);
      return { kind: "DataReference", sumName, symbolIndex, module, name };
    }

    case S_ANNOTATIONREF: {
      const sumName = buf.readU32();
      const symbolIndex: SymbolIndex = buf.readU32();
      const module = buf.readU16();
      const name = buf.readCString();
      return { kind: "AnnotationReference", sumName, symbolIndex, module, name };
    }

    case S_TRAMPOLINE: {
      const trampolineType = buf.readU16() as TrampolineType;
      const thunkSize = buf.readU16();
      const thunkOff = buf.readU32();
      const targetOff = buf.readU32();
      const thunkSection = buf.readU16();
      const targetSection = buf.readU16();
      return {
        kind: "Trampoline",
        trampolineType,
        thunkSize,
        thunkOffset: { offset: thunkOff, section: thunkSection },
        targetOffset: { offset: targetOff, section: targetSection },
      };
    }

    case S_EXPORT: {
      const ordinal = buf.readU16();
      const flags = parseExportSymbolFlags(buf.readU16());
      const name = buf.readCString();
      return { kind: "Export", ordinal, flags, name };
    }

    case S_LOCAL: {
      const type: TypeIndex = buf.readU32();
      const flags = parseLocalVariableFlags(buf.readU16());
      const name = buf.readCString();
      return { kind: "Local", type, flags, name };
    }

    case S_BUILDINFO: {
      const id: IdIndex = buf.readU32();
      return { kind: "BuildInfo", id };
    }

    case S_INLINESITE:
    case S_INLINESITE2: {
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const inlinee: IdIndex = buf.readU32();
      let invocations: number | null = null;
      if (kind === S_INLINESITE2) {
        invocations = buf.readU32();
      }
      const annotations = buf.remainingBytes.slice();
      return { kind: "InlineSite", parent, end, inlinee, invocations, annotations };
    }

    case S_BLOCK32_ST:
    case S_BLOCK32: {
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const len = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = parseSymbolName(buf, kind);
      return { kind: "Block", parent, end, len, offset, name };
    }

    case S_LABEL32_ST:
    case S_LABEL32: {
      const offset = readSectionOffset(buf);
      const flags = parseProcedureFlags(buf.readU8());
      const name = parseSymbolName(buf, kind);
      return { kind: "Label", offset, flags, name };
    }

    case S_BPREL32_ST:
    case S_BPREL32: {
      const offset = buf.readI32();
      const type: TypeIndex = buf.readU32();
      const name = parseSymbolName(buf, kind);
      return { kind: "BasePointerRelative", offset, type, name };
    }

    case S_REGREL32_ST:
    case S_REGREL32: {
      const offset = buf.readI32();
      const type: TypeIndex = buf.readU32();
      const register: Register = buf.readU16();
      const name = parseSymbolName(buf, kind);
      return { kind: "RegisterRelative", offset, type, register, name };
    }

    case S_FRAMEPROC: {
      const frameSize = buf.readU32();
      const paddingSize = buf.readU32();
      const paddingOffset = buf.readU32();
      const savedRegistersSize = buf.readU32();
      const exceptionHandlerOffset = buf.readU32();
      const exceptionHandlerSection = buf.readU16();
      buf.readU16(); // padding
      const flags = buf.readU32();
      return {
        kind: "FrameProcedure",
        frameSize,
        paddingSize,
        paddingOffset,
        savedRegistersSize,
        exceptionHandlerOffset,
        exceptionHandlerSection,
        flags,
      };
    }

    case S_COMPILE2:
    case S_COMPILE3: {
      const rawFlags = buf.readU32();
      const language = sourceLanguageFromU8(rawFlags & 0xff);
      const flags = parseCompileFlags(rawFlags);
      const cpuType = cpuTypeFromU16(buf.readU16());
      const frontMajor = buf.readU16();
      const frontMinor = buf.readU16();
      const frontBuild = buf.readU16();
      let frontQfe: number | null = null;
      if (kind === S_COMPILE3) {
        frontQfe = buf.readU16();
      }
      const backMajor = buf.readU16();
      const backMinor = buf.readU16();
      const backBuild = buf.readU16();
      let backQfe: number | null = null;
      if (kind === S_COMPILE3) {
        backQfe = buf.readU16();
      }
      const compilerVersion = buf.readCString();
      return {
        kind: "CompileFlags",
        language,
        flags,
        cpuType,
        frontendVersion: { major: frontMajor, minor: frontMinor, build: frontBuild, qfe: frontQfe },
        backendVersion: { major: backMajor, minor: backMinor, build: backBuild, qfe: backQfe },
        compilerVersion,
      };
    }

    case S_CALLSITEINFO: {
      const offset = readSectionOffset(buf);
      buf.readU16(); // padding
      const type: TypeIndex = buf.readU32();
      return { kind: "CallSiteInfo", offset, type };
    }

    case S_ENVBLOCK: {
      buf.readU8(); // flags (reserved)
      const entries: string[] = [];
      while (!buf.isEmpty) {
        const s = buf.readCString();
        if (s.length === 0) break;
        entries.push(s);
      }
      return { kind: "EnvironmentBlock", entries };
    }

    case S_SECTION: {
      const sectionNumber = buf.readU16();
      const alignment = buf.readU8();
      buf.readU8(); // reserved
      const rva = buf.readU32();
      const length = buf.readU32();
      const characteristics = buf.readU32();
      const name = buf.readCString();
      return { kind: "Section", sectionNumber, alignment, rva, length, characteristics, name };
    }

    case S_COFFGROUP: {
      const length = buf.readU32();
      const characteristics = buf.readU32();
      const offset = readSectionOffset(buf);
      const name = buf.readCString();
      return { kind: "CoffGroup", length, characteristics, offset, name };
    }

    case S_DEFRANGE: {
      const program = buf.readU32();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return { kind: "DefRange", program, range, gaps };
    }

    case S_DEFRANGE_SUBFIELD: {
      const program = buf.readU32();
      const offsetInParent = buf.readU32();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return { kind: "DefRangeSubfield", program, offsetInParent, range, gaps };
    }

    case S_DEFRANGE_REGISTER: {
      const register: Register = buf.readU16();
      const attr = buf.readU16();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return {
        kind: "DefRangeRegister",
        register,
        mayHaveNoName: (attr & 0x01) !== 0,
        range,
        gaps,
      };
    }

    case S_DEFRANGE_FRAMEPOINTER_REL: {
      const offset = buf.readI32();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return { kind: "DefRangeFramePointerRel", offset, range, gaps };
    }

    case S_DEFRANGE_SUBFIELD_REGISTER: {
      const register: Register = buf.readU16();
      const attr = buf.readU16();
      const offsetInParent = buf.readU32();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return {
        kind: "DefRangeSubfieldRegister",
        register,
        mayHaveNoName: (attr & 0x01) !== 0,
        offsetInParent,
        range,
        gaps,
      };
    }

    case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE: {
      const offset = buf.readI32();
      return { kind: "DefRangeFramePointerRelFullScope", offset };
    }

    case S_DEFRANGE_REGISTER_REL: {
      const baseRegister: Register = buf.readU16();
      const flagsVal = buf.readU16();
      const basePointerOffset = buf.readI32();
      const range = readLocalAddressRange(buf);
      const gaps = readGaps(buf);
      return {
        kind: "DefRangeRegisterRel",
        baseRegister,
        spilledUdtMember: (flagsVal & 0x01) !== 0,
        offsetInParent: (flagsVal >>> 4) & 0xfff,
        basePointerOffset,
        range,
        gaps,
      };
    }

    case S_CALLEES:
    case S_CALLERS: {
      const count = buf.readU32();
      const functions: TypeIndex[] = [];
      for (let i = 0; i < count; i++) {
        functions.push(buf.readU32());
      }
      let invocations: number[] | null = null;
      if (buf.remaining >= count * 4) {
        invocations = [];
        for (let i = 0; i < count; i++) {
          invocations.push(buf.readU32());
        }
      }
      return {
        kind: kind === S_CALLEES ? "Callees" : "Callers",
        functions,
        invocations,
      };
    }

    case S_INLINEES: {
      const count = buf.readU32();
      const inlinees: IdIndex[] = [];
      for (let i = 0; i < count; i++) {
        inlinees.push(buf.readU32());
      }
      return { kind: "Inlinees", inlinees };
    }

    default:
      throw PdbError.unimplementedSymbolKind(kind);
  }
}

/** Read a section offset (offset u32 + section u16). */
function readSectionOffset(buf: ParseBuffer): PdbInternalSectionOffset {
  const offset = buf.readU32();
  const section = buf.readU16();
  return { offset, section };
}

/** Read a symbol index, treating 0 as null. */
function readOptionalSymbolIndex(buf: ParseBuffer): SymbolIndex | null {
  const val = buf.readU32();
  return val === 0 ? null : val;
}

/** Parse a symbol name based on the symbol kind. */
function parseSymbolName(buf: ParseBuffer, kind: number): string {
  if (kind < S_ST_MAX) {
    return buf.readU8PascalString();
  }
  return buf.readCString();
}

/** Parse an optional symbol name. */
function parseOptionalName(buf: ParseBuffer, kind: number): string | null {
  if (kind < S_ST_MAX) {
    return null;
  }
  if (buf.isEmpty) return null;
  return buf.readCString();
}

/** Read a CV_LVAR_ADDR_RANGE: offStart(u32) + isectStart(u16) + cbRange(u16). */
function readLocalAddressRange(buf: ParseBuffer): LocalAddressRange {
  const offsetStart = buf.readU32();
  const sectionStart = buf.readU16();
  const length = buf.readU16();
  return { offsetStart, sectionStart, length };
}

/** Read remaining bytes as CV_LVAR_ADDR_GAP entries (4 bytes each). */
function readGaps(buf: ParseBuffer): LocalAddressGap[] {
  const gaps: LocalAddressGap[] = [];
  while (buf.remaining >= 4) {
    const startOffset = buf.readU16();
    const length = buf.readU16();
    gaps.push({ startOffset, length });
  }
  return gaps;
}

/** Returns true if the given symbol kind starts a new scope. */
export function symbolStartsScope(kind: number): boolean {
  switch (kind) {
    case S_GPROC32:
    case S_LPROC32:
    case S_GPROC32_ID:
    case S_LPROC32_ID:
    case S_LPROC32_DPC:
    case S_LPROC32_DPC_ID:
    case S_GPROC32_ST:
    case S_LPROC32_ST:
    case S_THUNK32:
    case S_THUNK32_ST:
    case S_BLOCK32:
    case S_BLOCK32_ST:
    case S_SEPCODE:
    case S_INLINESITE:
    case S_INLINESITE2:
      return true;
    default:
      return false;
  }
}

/** Returns true if the given symbol kind ends a scope. */
export function symbolEndsScope(kind: number): boolean {
  switch (kind) {
    case S_END:
    case S_PROC_ID_END:
    case S_INLINESITE_END:
      return true;
    default:
      return false;
  }
}
