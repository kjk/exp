/**
 * pdb_framedata - Dump frame data from a PDB file.
 *
 * Usage: bun run examples/pdb_framedata.ts <input.pdb>
 *
 * Parses and displays the frame data (stack unwinding information) from the
 * DBI extra streams. Shows code address, sizes, and frame program strings.
 *
 * Ported from getsentry/pdb examples/pdb_framedata.rs
 */

import { PDB, ParseBuffer, type StringTable } from "../src/index.js";

/** A parsed frame data record. */
interface FrameData {
  codeStart: number;
  codeSize: number;
  localsSize: number;
  paramsSize: number;
  maxStackSize: number;
  prologSize: number;
  savedRegsSize: number;
  hasStructuredEh: boolean;
  hasCppEh: boolean;
  isFunctionStart: boolean;
  usesBasePointer: boolean;
  frameType: number;
  programStringOffset: number | null;
}

function dumpFrameData(filename: string): void {
  const data = require("fs").readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(data));

  const stringTable = pdb.stringTable;
  const extra = pdb.extraStreams;

  // The frame data is stored in the "framedata" extra stream
  let framedataStream;
  try {
    if (extra.framedata === 0xffff) {
      console.log("No frame data stream available.");
      return;
    }
    framedataStream = pdb.getStream(extra.framedata);
  } catch {
    console.log("No frame data stream available.");
    return;
  }

  console.log("Frame data:");
  console.log(
    "Address    Blk Size   Locals   Params   StkMax   Prolog SavedReg SEH C++EH Start  BP  Type   Program"
  );
  console.log();

  // Parse frame data records
  // The framedata stream starts with a u32 count in some formats, or
  // is just a sequence of fixed-size records.
  const buf = new ParseBuffer(framedataStream.data);

  // New FPO format: each record is 32 bytes
  // struct {
  //   u32 code_start_rva;
  //   u32 code_size;
  //   u32 locals_size;
  //   u32 params_size;
  //   u32 max_stack_size;
  //   u32 frame_program_string;  // offset into string table
  //   u16 prolog_size;
  //   u16 saved_regs_size;
  //   u32 flags;
  // }
  const RECORD_SIZE = 32;

  // Skip the initial count word if present
  if (buf.remaining >= 4) {
    // Check if first u32 looks like a count
    const possibleCount = buf.peekU32();
    const expectedRecords = (buf.remaining - 4) / RECORD_SIZE;
    if (possibleCount === Math.floor(expectedRecords)) {
      buf.readU32(); // skip count
    }
  }

  while (buf.remaining >= RECORD_SIZE) {
    const codeStart = buf.readU32();
    const codeSize = buf.readU32();
    const localsSize = buf.readU32();
    const paramsSize = buf.readU32();
    const maxStackSize = buf.readU32();
    const programOffset = buf.readU32();
    const prologSize = buf.readU16();
    const savedRegsSize = buf.readU16();
    const flags = buf.readU32();

    const hasStructuredEh = (flags & 0x01) !== 0;
    const hasCppEh = (flags & 0x02) !== 0;
    const isFunctionStart = (flags & 0x04) !== 0;
    const usesBasePointer = (flags & 0x08) !== 0;
    const frameType = (flags >> 14) & 0x03;

    // Resolve program string
    let programString = "";
    if (stringTable && programOffset !== 0) {
      const resolved = stringTable.getString(programOffset);
      if (resolved) programString = resolved;
    }

    console.log(
      `${hex8(codeStart)} ${hex8(codeSize)} ${hex8(localsSize)} ${hex8(paramsSize)} ` +
      `${hex8(maxStackSize)} ${hex8(prologSize)} ${hex8(savedRegsSize)}   ` +
      `${hasStructuredEh ? "Y" : "N"}     ${hasCppEh ? "Y" : "N"}     ` +
      `${isFunctionStart ? "Y" : "N"}   ${usesBasePointer ? "Y" : "N"} ` +
      `${String(frameType).padStart(5)}   ${programString}`
    );
  }
}

function hex8(n: number): string {
  return n.toString(16).padStart(8, " ");
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/pdb_framedata.ts <input.pdb>");
  process.exit(1);
}

try {
  dumpFrameData(filename);
} catch (e) {
  console.error(`error dumping PDB: ${e}`);
  process.exit(1);
}
