import { ParseBuffer, type Variant } from "./buffer.js";
import { PdbError } from "./error.js";
import type { MsfStream } from "./msf.js";
import type {
  TypeIndex,
  PdbInternalSectionOffset,
  Register,
  SymbolIndex,
} from "./common.js";

// Symbol kind constants
const S_END = 0x0006;
const S_PROC_ID_END = 0x114f;
const S_INLINESITE_END = 0x114e;
const S_ST_MAX = 0x1100;

// Key symbol kinds
const S_OBJNAME_ST = 0x0009;
const S_OBJNAME = 0x1101;
const S_REGISTER_ST = 0x1001;
const S_REGISTER = 0x1106;
const S_CONSTANT_ST = 0x1002;
const S_CONSTANT = 0x1107;
const S_UDT_ST = 0x1003;
const S_UDT = 0x1108;
const S_LDATA32_ST = 0x1007;
const S_LDATA32 = 0x110c;
const S_GDATA32_ST = 0x1008;
const S_GDATA32 = 0x110d;
const S_PUB32_ST = 0x1009;
const S_PUB32 = 0x110e;
const S_LPROC32_ST = 0x100a;
const S_LPROC32 = 0x110f;
const S_GPROC32_ST = 0x100b;
const S_GPROC32 = 0x1110;
const S_LTHREAD32_ST = 0x100e;
const S_LTHREAD32 = 0x1112;
const S_GTHREAD32_ST = 0x100f;
const S_GTHREAD32 = 0x1113;
const S_LPROC32_ID = 0x1146;
const S_GPROC32_ID = 0x1147;
const S_LPROC32_DPC = 0x1155;
const S_LPROC32_DPC_ID = 0x1156;
const S_PROCREF_ST = 0x0400;
const S_PROCREF = 0x1125;
const S_LPROCREF_ST = 0x0403;
const S_LPROCREF = 0x1127;
const S_DATAREF_ST = 0x0401;
const S_DATAREF = 0x1126;
const S_ANNOTATIONREF = 0x1128;
const S_TRAMPOLINE = 0x112c;
const S_EXPORT = 0x1138;
const S_LOCAL = 0x113e;
const S_BUILDINFO = 0x114c;
const S_INLINESITE = 0x114d;
const S_INLINESITE2 = 0x115d;
const S_COMPILE2 = 0x1116;
const S_COMPILE3 = 0x113c;
const S_UNAMESPACE_ST = 0x1029;
const S_UNAMESPACE = 0x1124;
const S_BPREL32 = 0x110b;
const S_BPREL32_ST = 0x1006;
const S_REGREL32 = 0x1111;
const S_REGREL32_ST = 0x100d;
const S_BLOCK32_ST = 0x0207;
const S_BLOCK32 = 0x1103;
const S_LABEL32_ST = 0x0209;
const S_LABEL32 = 0x1105;
const S_THUNK32_ST = 0x0206;
const S_THUNK32 = 0x1102;
const S_SEPCODE = 0x1132;
const S_FRAMEPROC = 0x1012;
const S_CALLSITEINFO = 0x1139;
const S_ENVBLOCK = 0x113d;
const S_SECTION = 0x1136;
const S_COFFGROUP = 0x1137;
const S_MANYREG_ST = 0x1005;
const S_MANYREG = 0x110a;
const S_MANYREG2_ST = 0x1014;
const S_MANYREG2 = 0x1117;

/** A raw symbol record. */
export interface RawSymbol {
  index: SymbolIndex;
  kind: number;
  data: Uint8Array;
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

export interface ConstantSymbol {
  kind: "Constant";
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

export interface ProcedureFlags {
  nofpo: boolean;
  interrupt: boolean;
  farReturn: boolean;
  neverReturn: boolean;
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
  trampolineType: number;
  thunkSize: number;
  thunkOffset: PdbInternalSectionOffset;
  targetOffset: PdbInternalSectionOffset;
}

export interface ExportSymbol {
  kind: "Export";
  ordinal: number;
  flags: number;
  name: string;
}

export interface LocalSymbol {
  kind: "Local";
  type: TypeIndex;
  flags: number;
  name: string;
}

export interface BuildInfoSymbol {
  kind: "BuildInfo";
  id: number;
}

export interface InlineSiteSymbol {
  kind: "InlineSite";
  parent: SymbolIndex | null;
  end: SymbolIndex;
  inlinee: TypeIndex;
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
  flags: number;
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
  language: number;
  cpu: number;
  flags: number;
  compilerVersion: string;
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

export type SymbolData =
  | ObjNameSymbol
  | RegisterVariableSymbol
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
  | SectionSymbol
  | CoffGroupSymbol;

/** Iterator for symbol records. */
export function* iterateSymbols(data: Uint8Array): Generator<RawSymbol> {
  const buf = new ParseBuffer(data);
  let offset = 0;

  while (!buf.isEmpty) {
    const startPos = buf.pos;
    const length = buf.readU16();
    if (length < 2) {
      throw PdbError.symbolTooShort();
    }
    const recordData = buf.take(length);
    const kind = recordData[0] | (recordData[1] << 8);
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

    case S_CONSTANT_ST:
    case S_CONSTANT: {
      const type: TypeIndex = buf.readU32();
      const value = buf.readVariant();
      const name = parseSymbolName(buf, kind);
      return { kind: "Constant", type, value, name };
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
      return { kind: "Data", global, type, offset, name };
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
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const next = readOptionalSymbolIndex(buf);
      const len = buf.readU32();
      const dbgStart = buf.readU32();
      const dbgEnd = buf.readU32();
      const type: TypeIndex = buf.readU32();
      const offset = readSectionOffset(buf);
      const rawFlags = buf.readU8();
      const flags: ProcedureFlags = {
        nofpo: (rawFlags & 0x01) !== 0,
        interrupt: (rawFlags & 0x02) !== 0,
        farReturn: (rawFlags & 0x04) !== 0,
        neverReturn: (rawFlags & 0x08) !== 0,
      };
      const name = parseSymbolName(buf, kind);
      return {
        kind: "Procedure",
        global,
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
      const trampolineType = buf.readU16();
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
      const flags = buf.readU16();
      const name = buf.readCString();
      return { kind: "Export", ordinal, flags, name };
    }

    case S_LOCAL: {
      const type: TypeIndex = buf.readU32();
      const flags = buf.readU16();
      const name = buf.readCString();
      return { kind: "Local", type, flags, name };
    }

    case S_BUILDINFO: {
      const id = buf.readU32();
      return { kind: "BuildInfo", id };
    }

    case S_INLINESITE:
    case S_INLINESITE2: {
      const parent = readOptionalSymbolIndex(buf);
      const end: SymbolIndex = buf.readU32();
      const inlinee: TypeIndex = buf.readU32();
      if (kind === S_INLINESITE2) {
        buf.readU32(); // invocations count
      }
      const annotations = buf.remainingBytes.slice();
      return { kind: "InlineSite", parent, end, inlinee, annotations };
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
      const flags = buf.readU8();
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
      const flags = buf.readU32();
      const language = flags & 0xff;
      const cpu = buf.readU16();
      const frontMajor = buf.readU16();
      const frontMinor = buf.readU16();
      const frontBuild = buf.readU16();
      let frontQfe = 0;
      if (kind === S_COMPILE3) {
        frontQfe = buf.readU16();
      }
      const backMajor = buf.readU16();
      const backMinor = buf.readU16();
      const backBuild = buf.readU16();
      let backQfe = 0;
      if (kind === S_COMPILE3) {
        backQfe = buf.readU16();
      }
      const compilerVersion = buf.readCString();
      return {
        kind: "CompileFlags",
        language,
        cpu,
        flags,
        compilerVersion,
      };
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
