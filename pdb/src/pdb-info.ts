import { ParseBuffer } from "./buffer.js";
import type { MsfStream } from "./msf.js";
import type { PdbGuid, StreamIndex } from "./common.js";
import { headerVersionFromU32 } from "./common.js";

/** Information from the PDB Information stream (stream 1). */
export interface PdbInformation {
  /** The version of the PDB format. */
  version: number;
  /** A 32-bit timestamp signature. */
  signature: number;
  /** Number of times this PDB has been written. */
  age: number;
  /** GUID uniquely identifying this PDB. */
  guid: PdbGuid;
}

/** A named stream entry in the PDB. */
export interface StreamName {
  name: string;
  streamId: StreamIndex;
}

/** Parse PDB information from stream 1. */
export function parsePdbInformation(stream: MsfStream): PdbInformation {
  const buf = new ParseBuffer(stream.data);

  const version = headerVersionFromU32(buf.readU32());
  const signature = buf.readU32();
  const age = buf.readU32();

  // Parse GUID (mixed-endian format)
  const data1 = buf.readU32();
  const data2 = buf.readU16();
  const data3 = buf.readU16();
  const data4 = new Uint8Array(buf.take(8));

  const guid: PdbGuid = { data1, data2, data3, data4 };

  return { version, signature, age, guid };
}

/** Parse stream names from the PDB Information stream. */
export function parseStreamNames(stream: MsfStream): StreamName[] {
  const buf = new ParseBuffer(stream.data);

  // Skip the fixed header (version + signature + age + guid)
  buf.take(4 + 4 + 4 + 16); // 28 bytes

  const namesSize = buf.readU32();
  const namesOffset = buf.pos;

  // Save names data for later lookup
  const namesData = stream.data.subarray(namesOffset, namesOffset + namesSize);

  // Skip past the names block
  buf.take(namesSize);

  // Parse the name map
  const count = buf.readU32();
  const _entriesSize = buf.readU32();

  // Read "ok" bit array (present entries)
  const okWords = buf.readU32();
  buf.take(okWords * 4); // skip ok bits

  // Read "deleted" bit array
  const deletedWords = buf.readU32();
  buf.take(deletedWords * 4); // skip deleted bits

  const names: StreamName[] = [];
  for (let i = 0; i < count; i++) {
    const nameOffset = buf.readU32();
    const streamId = buf.readU32() & 0xffff;

    // Read name from the names block
    const nameBuf = new ParseBuffer(namesData.subarray(nameOffset));
    const name = nameBuf.readCString();

    names.push({ name, streamId });
  }

  return names;
}
