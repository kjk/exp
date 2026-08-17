import { ParseBuffer } from "./buffer.js";
import { PdbError } from "./error.js";
import type { MsfStream } from "./msf.js";
import type {
  StreamIndex,
  PdbInternalSectionOffset,
  SectionCharacteristics,
} from "./common.js";
import {
  STREAM_INDEX_NONE,
  headerVersionFromU32,
  machineTypeFromU16,
  MachineType,
} from "./common.js";

/** DBI stream header. */
export interface DbiHeader {
  signature: number;
  version: number;
  age: number;
  gsSymbolsStream: StreamIndex;
  internalVersion: number;
  psSymbolsStream: StreamIndex;
  pdbDllBuildVersion: number;
  symbolRecordsStream: StreamIndex;
  pdbDllRbldVersion: number;
  moduleListSize: number;
  sectionContributionSize: number;
  sectionMapSize: number;
  fileInfoSize: number;
  typeServerMapSize: number;
  mfcTypeServerIndex: number;
  debugHeaderSize: number;
  ecSubstreamSize: number;
  flags: number;
  machineType: number;
}

/** Information about a module's contribution to a section. */
export interface DbiSectionContribution {
  offset: PdbInternalSectionOffset;
  size: number;
  characteristics: SectionCharacteristics;
  module: number;
  dataCrc: number;
  relocCrc: number;
}

/** Module information from the DBI stream. */
export interface DbiModule {
  /** Stream number for module's debug info. */
  stream: StreamIndex;
  /** Size of local symbols in the module's stream. */
  symbolsSize: number;
  /** Size of C13-style line info in the module's stream. */
  c13LinesSize: number;
  /** First section contribution. */
  sectionContribution: DbiSectionContribution;
  /** Module name (usually path to object file). */
  moduleName: string;
  /** Object file name (path to archive for library modules). */
  objectFileName: string;
}

/** Extra stream indices from the DBI debug header. */
export interface DbiExtraStreams {
  fpo: StreamIndex;
  exception: StreamIndex;
  fixup: StreamIndex;
  omapToSrc: StreamIndex;
  omapFromSrc: StreamIndex;
  sectionHeaders: StreamIndex;
  tokenRidMap: StreamIndex;
  xdata: StreamIndex;
  pdata: StreamIndex;
  framedata: StreamIndex;
  originalSectionHeaders: StreamIndex;
}

/**
 * An entry in the DBI section map (also known as OMF Segment Map).
 * See https://github.com/google/syzygy/blob/8164b24ebde9c5649c9a09e88a7fc0b0fcbd1bc5/syzygy/pdb/pdb_data.h#L172
 */
export interface DbiSectionMapItem {
  /** Flags: 0x1 read, 0x2 write, 0x4 execute, 0x8 32-bit. */
  flags: number;
  /** Section type: 0x1 = SEL, 0x2 = ABS, 0x10 = GROUP. */
  sectionType: number;
  /** Overlay number. */
  overlay: number;
  /** Group index, 0 if not relevant. */
  group: number;
  /** Section number (frame in OMF terminology). */
  sectionNumber: number;
  /** Index into name table, or 0xffff. */
  segNameIndex: number;
  /** Index into name table, or 0xffff. */
  classNameIndex: number;
  /** RVA offset of this section. */
  rvaOffset: number;
  /** Length of this section. */
  sectionLength: number;
}

/** Parsed debug information from the DBI stream. */
export interface DebugInformation {
  header: DbiHeader;
  modules: DbiModule[];
  machineType: MachineType;
  age: number | null;
  /** Whether this PDB has been stripped (no type info, line info, or per-object CV symbols). */
  isStripped: boolean;
  extraStreams: DbiExtraStreams;
  sectionContributions: DbiSectionContribution[];
  /** Section map entries (OMF Segment Map). */
  sectionMap: DbiSectionMapItem[];
}

/** Parse a section contribution record. */
function parseSectionContribution(buf: ParseBuffer): DbiSectionContribution {
  const section = buf.readU16();
  buf.readU16(); // padding
  const offset = buf.readU32();
  const size = buf.readU32();
  const characteristics = buf.readU32();
  const module = buf.readU16();
  buf.readU16(); // padding
  const dataCrc = buf.readU32();
  const relocCrc = buf.readU32();

  return {
    offset: { offset, section },
    size,
    characteristics,
    module,
    dataCrc,
    relocCrc,
  };
}

/** Parse the DBI stream. */
export function parseDebugInformation(stream: MsfStream): DebugInformation {
  const buf = new ParseBuffer(stream.data);

  // Parse DBI header
  const header: DbiHeader = {
    signature: buf.readU32(),
    version: headerVersionFromU32(buf.readU32()) as number,
    age: buf.readU32(),
    gsSymbolsStream: buf.readU16(),
    internalVersion: buf.readU16(),
    psSymbolsStream: buf.readU16(),
    pdbDllBuildVersion: buf.readU16(),
    symbolRecordsStream: buf.readU16(),
    pdbDllRbldVersion: buf.readU16(),
    moduleListSize: buf.readU32(),
    sectionContributionSize: buf.readU32(),
    sectionMapSize: buf.readU32(),
    fileInfoSize: buf.readU32(),
    typeServerMapSize: buf.readU32(),
    mfcTypeServerIndex: buf.readU32(),
    debugHeaderSize: buf.readU32(),
    ecSubstreamSize: buf.readU32(),
    flags: buf.readU16(),
    machineType: buf.readU16(),
    };

  buf.readU32(); // reserved

  if (header.signature !== 0xffffffff) {
    throw PdbError.unimplementedFeature("ancient DBI header");
  }

  const headerLen = buf.pos;

  // Parse modules
  const modules: DbiModule[] = [];
  const modulesEnd = headerLen + header.moduleListSize;
  while (buf.pos < modulesEnd) {
    const opened = buf.readU32();
    const sectionContribution = parseSectionContribution(buf);
    const flags = buf.readU16();
    const moduleStream: StreamIndex = buf.readU16();
    const symbolsSize = buf.readU32();
    const linesSize = buf.readU32();
    const c13LinesSize = buf.readU32();
    const files = buf.readU16();
    const _padding = buf.readU16();
    const filenameOffsets = buf.readU32();
    const source = buf.readU32();
    const compiler = buf.readU32();

    const moduleName = buf.readCString();
    const objectFileName = buf.readCString();
    buf.align(4);

    modules.push({
      stream: moduleStream,
      symbolsSize,
      c13LinesSize,
      sectionContribution,
      moduleName,
      objectFileName,
    });
  }

  // Parse section contributions
  const sectionContributions: DbiSectionContribution[] = [];
  const scEnd = modulesEnd + header.sectionContributionSize;
  if (buf.pos < scEnd) {
    const scVersion = buf.readU32();
    const isV2 = scVersion === (0xeffe0000 + 20140516);
    while (buf.pos < scEnd) {
      sectionContributions.push(parseSectionContribution(buf));
      if (isV2) {
        buf.readU32(); // extra field in V2
      }
    }
  }

  // Parse section map
  const sectionMapStart = modulesEnd + header.sectionContributionSize;
  buf.seek(sectionMapStart);
  const sectionMap = parseSectionMap(buf, header.sectionMapSize);

  // Skip to debug header
  const debugHeaderOffset =
    headerLen +
    header.moduleListSize +
    header.sectionContributionSize +
    header.sectionMapSize +
    header.fileInfoSize +
    header.typeServerMapSize +
    header.ecSubstreamSize;

  buf.seek(debugHeaderOffset);

  // Parse extra streams
  const extraStreams = parseExtraStreams(buf, header.debugHeaderSize);

  const machineType = machineTypeFromU16(header.machineType);
  const age = header.age === 0 ? null : header.age;

  return {
    header,
    modules,
    machineType,
    age,
    isStripped: (header.flags & 0x2) !== 0,
    extraStreams,
    sectionContributions,
    sectionMap,
  };
}

/** Parse the DBI extra streams (DbgDataHdr). */
function parseExtraStreams(buf: ParseBuffer, size: number): DbiExtraStreams {
  if (size % 2 !== 0) {
    throw PdbError.invalidStreamLength("DbgDataHdr");
  }

  function nextIndex(): StreamIndex {
    if (buf.remaining < 2) return STREAM_INDEX_NONE;
    return buf.readU16();
  }

  const end = buf.pos + size;
  const result: DbiExtraStreams = {
    fpo: nextIndex(),
    exception: nextIndex(),
    fixup: nextIndex(),
    omapToSrc: nextIndex(),
    omapFromSrc: nextIndex(),
    sectionHeaders: nextIndex(),
    tokenRidMap: nextIndex(),
    xdata: nextIndex(),
    pdata: nextIndex(),
    framedata: nextIndex(),
    originalSectionHeaders: nextIndex(),
  };

  // Skip any remaining bytes
  if (buf.pos < end) {
    buf.take(end - buf.pos);
  }

  return result;
}

/** Parse the DBI section map (OMF Segment Map). */
function parseSectionMap(buf: ParseBuffer, size: number): DbiSectionMapItem[] {
  if (size === 0) return [];

  const end = buf.pos + size;

  // Section map header: sec_count (u16) + sec_count_log (u16)
  const secCount = buf.readU16();
  const _secCountLog = buf.readU16();

  const items: DbiSectionMapItem[] = [];
  while (buf.pos < end && items.length < secCount) {
    items.push({
      flags: buf.readU8(),
      sectionType: buf.readU8(),
      overlay: buf.readU16(),
      group: buf.readU16(),
      sectionNumber: buf.readU16(),
      segNameIndex: buf.readU16(),
      classNameIndex: buf.readU16(),
      rvaOffset: buf.readU32(),
      sectionLength: buf.readU32(),
    });
  }

  // Skip remaining bytes
  if (buf.pos < end) {
    buf.take(end - buf.pos);
  }

  return items;
}
