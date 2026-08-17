import { ParseBuffer } from "./buffer.js";
import { PdbError } from "./error.js";
import type { MsfStream } from "./msf.js";
import type { DbiModule } from "./dbi.js";
import { type RawSymbol, iterateSymbols } from "./symbol/index.js";
import type { PdbInternalSectionOffset, FileIndex, StringRef } from "./common.js";

const CV_SIGNATURE_C13 = 4;

/** Debug subsection types. */
const DEBUG_S_LINES = 0xf2;
const DEBUG_S_FILECHKSMS = 0xf4;
const DEBUG_S_INLINEELINES = 0xf6;

/** A line record from C13 line info. */
export interface LineInfo {
  /** Offset within the section. */
  offset: PdbInternalSectionOffset;
  /** Length of the code this line covers. */
  length: number | null;
  /** File index within the module. */
  fileIndex: FileIndex;
  /** Starting line number. */
  lineStart: number;
  /** Ending line number. */
  lineEnd: number;
  /** Starting column (0 if not present). */
  columnStart: number;
  /** Ending column (0 if not present). */
  columnEnd: number;
  /** Whether this is a statement. */
  isStatement: boolean;
}

/** File checksum information. */
export interface FileChecksum {
  /** Offset into the string table for the file name. */
  nameRef: StringRef;
  /** Checksum kind (0=none, 1=MD5, 2=SHA1, 3=SHA256). */
  checksumKind: number;
  /** Checksum bytes. */
  checksum: Uint8Array;
}

/** Parsed module information (symbols + line info). */
export interface ModuleInfo {
  /** The C13 signature. */
  signature: number;
  /** Symbol records in this module. */
  symbols: RawSymbol[];
  /** Line information records. */
  lines: LineInfo[];
  /** File checksum records. */
  fileChecksums: FileChecksum[];
}

/** Parse a module's debug info stream. */
export function parseModuleInfo(
  stream: MsfStream,
  module: DbiModule,
): ModuleInfo {
  const buf = new ParseBuffer(stream.data);

  // Read signature
  const signature = buf.readU32();

  // Parse symbol records
  const symbolsEnd = module.symbolsSize;
  const symbolsData = stream.data.subarray(4, symbolsEnd);
  const symbols = [...iterateSymbols(symbolsData)];

  // Parse C13 line information
  const lines: LineInfo[] = [];
  const fileChecksums: FileChecksum[] = [];

  if (module.c13LinesSize > 0) {
    const c13Start = symbolsEnd;
    const c13End = c13Start + module.c13LinesSize;
    const c13Data = stream.data.subarray(c13Start, c13End);
    parseC13Lines(c13Data, lines, fileChecksums);
  }

  return { signature, symbols, lines, fileChecksums };
}

/** Parse C13-format debug subsections. */
function parseC13Lines(
  data: Uint8Array,
  lines: LineInfo[],
  fileChecksums: FileChecksum[],
): void {
  const buf = new ParseBuffer(data);

  while (!buf.isEmpty) {
    const subsectionType = buf.readU32();
    const subsectionSize = buf.readU32();
    const subsectionEnd = buf.pos + subsectionSize;

    switch (subsectionType & 0xff) {
      case DEBUG_S_LINES & 0xff:
        parseLinesSubsection(buf, subsectionSize, lines);
        break;

      case DEBUG_S_FILECHKSMS & 0xff:
        parseFileChecksumsSubsection(buf, subsectionSize, fileChecksums);
        break;

      default:
        // Skip unknown subsections
        buf.seek(subsectionEnd);
        break;
    }

    // Align to 4 bytes after each subsection
    if (buf.pos < subsectionEnd) {
      buf.seek(subsectionEnd);
    }
    if (!buf.isEmpty) {
      buf.align(4);
    }
  }
}

/** Parse a DEBUG_S_LINES subsection. */
function parseLinesSubsection(
  buf: ParseBuffer,
  size: number,
  lines: LineInfo[],
): void {
  const end = buf.pos + size;

  // Header
  const sectionOffset = buf.readU32();
  const sectionIndex = buf.readU16();
  const flags = buf.readU16();
  const codeSize = buf.readU32();

  const hasColumns = (flags & 0x01) !== 0;

  while (buf.pos < end) {
    // File block header
    const fileIndex: FileIndex = buf.readU32();
    const numLines = buf.readU32();
    const blockSize = buf.readU32();

    // Line entries
    const lineEntries: { offset: number; lineStart: number; deltaEnd: number; isStatement: boolean }[] = [];
    for (let i = 0; i < numLines; i++) {
      const offset = buf.readU32();
      const lineFlags = buf.readU32();
      const lineStart = lineFlags & 0x00ffffff;
      const deltaEnd = (lineFlags >> 24) & 0x7f;
      const isStatement = (lineFlags & 0x80000000) !== 0;
      lineEntries.push({ offset, lineStart, deltaEnd, isStatement });
    }

    // Column entries (if present)
    let columnStarts: number[] | null = null;
    let columnEnds: number[] | null = null;
    if (hasColumns) {
      columnStarts = [];
      columnEnds = [];
      for (let i = 0; i < numLines; i++) {
        columnStarts.push(buf.readU16());
        columnEnds.push(buf.readU16());
      }
    }

    // Build line info records
    for (let i = 0; i < numLines; i++) {
      const entry = lineEntries[i];
      const nextOffset = i + 1 < numLines ? lineEntries[i + 1].offset : codeSize;
      const length = nextOffset - entry.offset;

      lines.push({
        offset: { offset: sectionOffset + entry.offset, section: sectionIndex },
        length: length > 0 ? length : null,
        fileIndex,
        lineStart: entry.lineStart,
        lineEnd: entry.lineStart + entry.deltaEnd,
        columnStart: columnStarts ? columnStarts[i] : 0,
        columnEnd: columnEnds ? columnEnds[i] : 0,
        isStatement: entry.isStatement,
      });
    }
  }
}

/** Parse a DEBUG_S_FILECHKSMS subsection. */
function parseFileChecksumsSubsection(
  buf: ParseBuffer,
  size: number,
  fileChecksums: FileChecksum[],
): void {
  const end = buf.pos + size;

  while (buf.pos < end) {
    const nameRef: StringRef = buf.readU32();
    const checksumSize = buf.readU8();
    const checksumKind = buf.readU8();
    const checksum = new Uint8Array(buf.take(checksumSize));

    fileChecksums.push({ nameRef, checksumKind, checksum });

    // Align to 4 bytes
    buf.align(4);
  }
}
