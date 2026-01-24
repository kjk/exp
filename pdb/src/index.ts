/**
 * PDB (Program Database) file format parser for TypeScript/Bun.
 *
 * This is a zero-copy parser that operates entirely on a Uint8Array buffer.
 * No I/O is performed - the caller is responsible for reading the file into memory.
 *
 * @example
 * ```ts
 * import { PDB } from "./index.js";
 *
 * const data = await Bun.file("example.pdb").bytes();
 * const pdb = PDB.open(data);
 *
 * console.log("GUID:", pdb.guid);
 * console.log("Age:", pdb.age);
 * console.log("Machine:", pdb.machineType);
 *
 * // Iterate modules
 * for (const mod of pdb.modules) {
 *   console.log("Module:", mod.moduleName);
 * }
 *
 * // Get type information
 * const types = pdb.typeInformation;
 * for (const item of types.items) {
 *   const parsed = pdb.parseType(item);
 *   if (parsed.kind === "Structure") {
 *     console.log("Struct:", parsed.name);
 *   }
 * }
 *
 * // Get global symbols
 * for (const sym of pdb.globalSymbols) {
 *   const parsed = pdb.parseSymbol(sym);
 *   if (parsed.kind === "Public") {
 *     console.log("Public:", parsed.name);
 *   }
 * }
 * ```
 */

// Re-export core types
export * from "./common.js";
export * from "./error.js";
export { ParseBuffer, type Variant } from "./buffer.js";
export { Msf, type MsfStream } from "./msf.js";
export type { PdbInformation, StreamName } from "./pdb-info.js";
export type {
  DbiHeader,
  DbiModule,
  DbiSectionContribution,
  DbiSectionMapItem,
  DbiExtraStreams,
  DebugInformation,
} from "./dbi.js";
export type {
  TypeData,
  IdData,
  ClassType,
  UnionType,
  EnumerationType,
  ProcedureType,
  MemberFunctionType,
  PointerType,
  ModifierType,
  ArrayType,
  AliasType,
  BitfieldType,
  FieldListType,
  ArgumentListType,
  MethodListType,
  MemberType,
  StaticMemberType,
  NestedType,
  BaseClassType,
  VirtualBaseClassType,
  VirtualFunctionTablePointerType,
  VirtualFunctionTableType,
  VirtualTableShapeType,
  OverloadedMethodType,
  MethodType,
  EnumerateType,
  TypeProperties,
  FieldAttributes,
  PointerAttributes,
  FunctionAttributes,
  ModifierFlags,
  FuncId,
  MemberFuncId,
  BuildInfo,
  StringId,
  UdtSourceLine,
  UdtModuleSourceLine,
} from "./tpi/types.js";
export {
  MemberAccess,
  MethodKind,
  CallingConvention,
  PointerKind,
  PointerMode,
  VirtualTableShapeDescriptor,
  isStaticMethod,
  isVirtualMethod,
  isPureVirtualMethod,
  isIntroVirtualMethod,
} from "./tpi/types.js";
export type { RawItem, TypeInformation } from "./tpi/parser.js";
export { ItemFinder } from "./tpi/parser.js";
export type { PrimitiveType, PrimitiveKind, Indirection } from "./tpi/primitive.js";
export { parsePrimitiveType, primitiveTypeName } from "./tpi/primitive.js";
export {
  PrimitiveKind_NoType, PrimitiveKind_Void, PrimitiveKind_HRESULT,
  PrimitiveKind_Char, PrimitiveKind_UChar, PrimitiveKind_I8, PrimitiveKind_U8,
  PrimitiveKind_RChar, PrimitiveKind_WChar, PrimitiveKind_RChar16,
  PrimitiveKind_RChar32, PrimitiveKind_Char8,
  PrimitiveKind_Short, PrimitiveKind_UShort, PrimitiveKind_I16, PrimitiveKind_U16,
  PrimitiveKind_Long, PrimitiveKind_ULong, PrimitiveKind_I32, PrimitiveKind_U32,
  PrimitiveKind_Quad, PrimitiveKind_UQuad, PrimitiveKind_I64, PrimitiveKind_U64,
  PrimitiveKind_Octa, PrimitiveKind_UOcta, PrimitiveKind_I128, PrimitiveKind_U128,
  PrimitiveKind_F16, PrimitiveKind_F32, PrimitiveKind_F32PP, PrimitiveKind_F48,
  PrimitiveKind_F64, PrimitiveKind_F80, PrimitiveKind_F128,
  PrimitiveKind_Complex32, PrimitiveKind_Complex64, PrimitiveKind_Complex80, PrimitiveKind_Complex128,
  PrimitiveKind_Bool8, PrimitiveKind_Bool16, PrimitiveKind_Bool32, PrimitiveKind_Bool64,
  Indirection_Near16, Indirection_Far16, Indirection_Huge16,
  Indirection_Near32, Indirection_Far32, Indirection_Near64, Indirection_Near128,
} from "./tpi/primitive.js";
export type {
  RawSymbol,
  SymbolData,
  ObjNameSymbol,
  RegisterVariableSymbol,
  MultiRegisterVariableSymbol,
  ConstantSymbol,
  UserDefinedTypeSymbol,
  DataSymbol,
  PublicSymbol,
  ProcedureSymbol,
  ProcedureFlags,
  ThreadStorageSymbol,
  UsingNamespaceSymbol,
  ProcedureReferenceSymbol,
  DataReferenceSymbol,
  AnnotationReferenceSymbol,
  TrampolineSymbol,
  ExportSymbol,
  ExportSymbolFlags,
  LocalSymbol,
  LocalVariableFlags,
  BuildInfoSymbol,
  InlineSiteSymbol,
  BlockSymbol,
  LabelSymbol,
  ScopeEndSymbol,
  BasePointerRelativeSymbol,
  RegisterRelativeSymbol,
  FrameProcedureSymbol,
  CompileFlagsSymbol,
  CompileFlags,
  CompilerVersion,
  ThunkSymbol,
  SeparatedCodeSymbol,
  SeparatedCodeFlags,
  CallSiteInfoSymbol,
  EnvironmentBlockSymbol,
  SectionSymbol,
  CoffGroupSymbol,
  BinaryAnnotation,
} from "./symbol/index.js";
export {
  CPUType,
  SourceLanguage,
  ThunkKind,
  TrampolineType,
  symbolStartsScope,
  symbolEndsScope,
  iterateBinaryAnnotations,
  BinaryAnnotationsIter,
  annotationEmitsLineInfo,
} from "./symbol/index.js";
export type {
  LineInfo,
  FileChecksum,
  ModuleInfo,
} from "./modi.js";
export { AddressMap } from "./address-map.js";
export { StringTable } from "./string-table.js";

import { Msf, type MsfStream } from "./msf.js";
import { parsePdbInformation, parseStreamNames, type PdbInformation, type StreamName } from "./pdb-info.js";
import { parseDebugInformation, type DebugInformation, type DbiModule, type DbiExtraStreams } from "./dbi.js";
import { parseItemInformation, parseTypeData, parseIdData, ItemFinder, type TypeInformation, type RawItem } from "./tpi/parser.js";
import type { TypeData, IdData } from "./tpi/types.js";
import { parseSymbolTable, parseSymbolData, type RawSymbol, type SymbolData } from "./symbol/index.js";
import { parseModuleInfo, type ModuleInfo } from "./modi.js";
import { buildAddressMap, parseSectionHeaders, AddressMap } from "./address-map.js";
import { parseStringTable, StringTable } from "./string-table.js";
import type { PdbGuid, ImageSectionHeader, StreamIndex } from "./common.js";
import { formatGuid, streamIndexIsValid, MachineType, STREAM_INDEX_NONE } from "./common.js";
import { PdbError } from "./error.js";

/** Fixed stream numbers in the MSF. */
const PDB_STREAM = 1;
const TPI_STREAM = 2;
const DBI_STREAM = 3;
const IPI_STREAM = 4;

/**
 * Main PDB parser class.
 *
 * Provides high-level access to all PDB data structures. Operates on a
 * Uint8Array buffer with no I/O operations.
 */
export class PDB {
  private msf: Msf;
  private _pdbInfo: PdbInformation | null = null;
  private _streamNames: StreamName[] | null = null;
  private _debugInfo: DebugInformation | null = null;
  private _typeInfo: TypeInformation | null = null;
  private _idInfo: TypeInformation | null = null;
  private _globalSymbols: RawSymbol[] | null = null;
  private _addressMap: AddressMap | null = null;
  private _stringTable: StringTable | null = null;
  private _sectionHeaders: ImageSectionHeader[] | null = null;

  private constructor(msf: Msf) {
    this.msf = msf;
  }

  /** Open a PDB file from a Uint8Array buffer. */
  static open(data: Uint8Array): PDB {
    const msf = Msf.open(data);
    return new PDB(msf);
  }

  // ─── PDB Information ───

  /** Get PDB metadata (GUID, age, signature). */
  get pdbInformation(): PdbInformation {
    if (!this._pdbInfo) {
      const stream = this.msf.getStream(PDB_STREAM);
      this._pdbInfo = parsePdbInformation(stream);
    }
    return this._pdbInfo;
  }

  /** The PDB's unique GUID. */
  get guid(): PdbGuid {
    return this.pdbInformation.guid;
  }

  /** The PDB's GUID formatted as a string. */
  get guidString(): string {
    return formatGuid(this.guid);
  }

  /** The PDB age (number of times written). */
  get age(): number {
    return this.pdbInformation.age;
  }

  /** Get named streams in the PDB. */
  get streamNames(): StreamName[] {
    if (!this._streamNames) {
      const stream = this.msf.getStream(PDB_STREAM);
      this._streamNames = parseStreamNames(stream);
    }
    return this._streamNames;
  }

  // ─── Debug Information (DBI) ───

  /** Get parsed debug information. */
  get debugInformation(): DebugInformation {
    if (!this._debugInfo) {
      const stream = this.msf.getStream(DBI_STREAM);
      this._debugInfo = parseDebugInformation(stream);
    }
    return this._debugInfo;
  }

  /** The target machine architecture. */
  get machineType(): MachineType {
    return this.debugInformation.machineType;
  }

  /** List of modules (object files) contributing to this PDB. */
  get modules(): DbiModule[] {
    return this.debugInformation.modules;
  }

  /** Extra stream references from the DBI header. */
  get extraStreams(): DbiExtraStreams {
    return this.debugInformation.extraStreams;
  }

  // ─── Type Information (TPI) ───

  /** Get the type information stream (TPI). */
  get typeInformation(): TypeInformation {
    if (!this._typeInfo) {
      const stream = this.msf.getStream(TPI_STREAM);
      this._typeInfo = parseItemInformation(stream);
    }
    return this._typeInfo;
  }

  /** Create an ItemFinder for efficient type lookup. */
  get typeFinder(): ItemFinder {
    return new ItemFinder(this.typeInformation);
  }

  /** Parse a raw type item into structured TypeData. */
  parseType(item: RawItem): TypeData {
    return parseTypeData(item);
  }

  // ─── Id Information (IPI) ───

  /** Get the id information stream (IPI), if present. */
  get idInformation(): TypeInformation | null {
    if (!this._idInfo) {
      if (!this.msf.hasStream(IPI_STREAM)) return null;
      const stream = this.msf.getStream(IPI_STREAM);
      this._idInfo = parseItemInformation(stream);
    }
    return this._idInfo;
  }

  /** Parse a raw id item into structured IdData. */
  parseId(item: RawItem): IdData {
    return parseIdData(item);
  }

  // ─── Symbols ───

  /** Get the global symbol table. */
  get globalSymbols(): RawSymbol[] {
    if (!this._globalSymbols) {
      const dbi = this.debugInformation;
      const streamIndex = dbi.header.symbolRecordsStream;
      if (!streamIndexIsValid(streamIndex)) {
        throw PdbError.globalSymbolsNotFound();
      }
      const stream = this.msf.getStream(streamIndex);
      this._globalSymbols = parseSymbolTable(stream);
    }
    return this._globalSymbols;
  }

  /** Parse a raw symbol into structured SymbolData. */
  parseSymbol(symbol: RawSymbol): SymbolData {
    return parseSymbolData(symbol);
  }

  // ─── Module Info ───

  /** Get module-specific debug info (symbols, lines) for a module. */
  getModuleInfo(module: DbiModule): ModuleInfo | null {
    if (!streamIndexIsValid(module.stream)) return null;
    const stream = this.msf.getStream(module.stream);
    return parseModuleInfo(stream, module);
  }

  // ─── Address Map ───

  /** Get the address map for RVA translation. */
  get addressMap(): AddressMap {
    if (!this._addressMap) {
      const extra = this.extraStreams;
      const sectionHeadersStream = this.getOptionalStream(extra.sectionHeaders);
      const originalSectionHeadersStream = this.getOptionalStream(extra.originalSectionHeaders);
      const omapFromSrcStream = this.getOptionalStream(extra.omapFromSrc);
      const omapToSrcStream = this.getOptionalStream(extra.omapToSrc);

      this._addressMap = buildAddressMap(
        sectionHeadersStream,
        originalSectionHeadersStream,
        omapFromSrcStream,
        omapToSrcStream,
      );
    }
    return this._addressMap;
  }

  // ─── Section Headers ───

  /** Get the PE section headers. */
  get sectionHeaders(): ImageSectionHeader[] {
    if (!this._sectionHeaders) {
      const extra = this.extraStreams;
      const stream = this.getOptionalStream(extra.sectionHeaders);
      this._sectionHeaders = stream
        ? parseSectionHeaders(stream)
        : this.synthesizeSections();
    }
    return this._sectionHeaders;
  }

  /**
   * Synthesize section headers from the DBI section map when no explicit
   * section header stream exists. This handles NGEN-generated PDB files
   * (.ni.pdb from Crossgen2).
   */
  private synthesizeSections(): ImageSectionHeader[] {
    // If we have OMAP From data, the RVAs won't map correctly.
    const extra = this.extraStreams;
    const omapFromSrcStream = this.getOptionalStream(extra.omapFromSrc);
    if (omapFromSrcStream) return [];

    const dbi = this.debugInformation;
    const sectionMap = dbi.sectionMap;
    if (sectionMap.length === 0) return [];

    let rva = 0x1000; // in the absence of explicit section data, this starts at 0x1000
    const sections: ImageSectionHeader[] = [];

    for (const sm of sectionMap) {
      // Filter: only SEL sections (sectionType == 1), skip ABS/GROUP.
      // Also skip sections with bogus length.
      if (sm.sectionType !== 1 || sm.sectionLength === 0xffffffff) continue;

      let characteristics = 0;
      if (sm.flags & 0x1) { // R
        characteristics |= 0x40000000; // IMAGE_SCN_MEM_READ
      }
      if (sm.flags & 0x2) { // W
        characteristics |= 0x80000000; // IMAGE_SCN_MEM_WRITE
      }
      if (sm.flags & 0x4) { // X
        characteristics |= 0x20000000; // IMAGE_SCN_MEM_EXECUTE
        characteristics |= 0x20; // IMAGE_SCN_CNT_CODE
      }

      const thisRva = rva + sm.rvaOffset;
      rva = thisRva + sm.sectionLength;

      sections.push({
        name: "",
        virtualSize: sm.sectionLength,
        virtualAddress: thisRva,
        sizeOfRawData: sm.sectionLength,
        pointerToRawData: 0,
        pointerToRelocations: 0,
        pointerToLinenumbers: 0,
        numberOfRelocations: 0,
        numberOfLinenumbers: 0,
        characteristics,
      });
    }

    return sections;
  }

  // ─── String Table ───

  /** Get the global string table (/names stream). */
  get stringTable(): StringTable | null {
    if (!this._stringTable) {
      const namesStream = this.streamNames.find((s) => s.name === "/names");
      if (!namesStream) return null;
      const stream = this.msf.getStream(namesStream.streamId);
      this._stringTable = parseStringTable(stream);
    }
    return this._stringTable;
  }

  // ─── Raw Stream Access ───

  /** Access a raw stream by its index. */
  getStream(index: StreamIndex): MsfStream {
    return this.msf.getStream(index);
  }

  /** Get a named stream by looking it up in the stream names. */
  getNamedStream(name: string): MsfStream | null {
    const entry = this.streamNames.find((s) => s.name === name);
    if (!entry) return null;
    return this.msf.getStream(entry.streamId);
  }

  /** Get an optional stream (returns null if stream index is invalid). */
  private getOptionalStream(index: StreamIndex): MsfStream | null {
    if (!streamIndexIsValid(index)) return null;
    if (!this.msf.hasStream(index)) return null;
    return this.msf.getStream(index);
  }
}
