import { ParseBuffer } from "./buffer.js";
import type { MsfStream } from "./msf.js";
import type { StringRef } from "./common.js";

/**
 * String table for resolving StringRef offsets to actual strings.
 *
 * PDB files deduplicate strings into a global string table stored in the
 * "/names" stream. StringRef values are byte offsets into this table.
 */
export class StringTable {
  private data: Uint8Array;

  constructor(data: Uint8Array) {
    this.data = data;
  }

  /** Look up a string by its StringRef (offset). */
  getString(ref: StringRef): string | null {
    if (ref >= this.data.length) return null;
    const buf = new ParseBuffer(this.data, ref);
    try {
      return buf.readCString();
    } catch {
      return null;
    }
  }
}

/** Parse the string table from the /names stream. */
export function parseStringTable(stream: MsfStream): StringTable {
  const buf = new ParseBuffer(stream.data);

  // The string table stream has a header:
  // u32 signature (0xeffeeffe)
  // u32 hash_version (1 or 2)
  // u32 byte_size (size of the string data)
  // [byte_size bytes] string data
  // ... hash table follows but we don't need it

  const signature = buf.readU32();
  const hashVersion = buf.readU32();
  const byteSize = buf.readU32();

  // The string data starts after the header
  const stringData = buf.take(byteSize);
  return new StringTable(stringData);
}
