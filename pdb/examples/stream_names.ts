/**
 * stream_names - List named streams in a PDB file.
 *
 * Usage: bun run examples/stream_names.ts <input.pdb>
 *
 * Prints the index, name, and size of each named stream.
 *
 * Ported from getsentry/pdb examples/stream_names.rs
 */

import { PDB } from "../src/index.js";

function dumpStreamNames(filename: string): void {
  const data = require("fs").readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(data));

  const names = pdb.streamNames;

  console.log("index, name, size");
  for (const entry of names) {
    let size: number;
    try {
      const stream = pdb.getStream(entry.streamId);
      size = stream.data.length;
    } catch {
      size = 0;
    }
    console.log(`${String(entry.streamId).padStart(5)}, ${entry.name} ${size} bytes`);
  }
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/stream_names.ts <input.pdb>");
  process.exit(1);
}

try {
  dumpStreamNames(filename);
} catch (e) {
  console.error(`error dumping PDB: ${e}`);
  process.exit(1);
}
