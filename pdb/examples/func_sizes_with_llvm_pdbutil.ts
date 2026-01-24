/**
 * func_sizes_with_llvm_pdbutil - Extract function sizes using llvm-pdbutil.
 *
 * Usage: bun run examples/func_sizes_with_llvm_pdbutil.ts [-rva] <input.pdb>
 *
 * Runs llvm-pdbutil to get module symbols, parses function sizes and names,
 * and prints them sorted by size (largest last).
 */

import { execSync } from "child_process";
import { PDB } from "../src/index.js";
import * as fs from "fs";
import * as path from "path";

interface FuncEntry {
  name: string;
  size: number;
  section: number;
  offset: number;
}

function escapeCsv(s: string): string {
  if (s.includes(",") || s.includes('"') || s.includes("\n")) {
    return '"' + s.replace(/"/g, '""') + '"';
  }
  return s;
}

function run(filename: string, writeRvaCsv: boolean): void {
  const output = execSync(
    `llvm-pdbutil pretty -module-syms -sym-types=funcs "${filename}"`,
    { encoding: "utf-8", maxBuffer: 256 * 1024 * 1024 }
  );

  const entries: FuncEntry[] = [];
  const sizeofRe = /sizeof=(\d+)\]\s*\(([0-9A-Fa-f]+):([0-9A-Fa-f]+)\)\s*(.*)/;

  for (const line of output.split("\n")) {
    const trimmed = line.trim();
    if (!trimmed.startsWith("func [")) continue;

    const match = trimmed.match(sizeofRe);
    if (!match) continue;

    const size = parseInt(match[1], 10);
    const section = parseInt(match[2], 16);
    const offset = parseInt(match[3], 16);
    const name = match[4].trim();
    if (name) {
      entries.push({ name, size, section, offset });
    }
  }

  entries.sort((a, b) => a.size - b.size);

  if (writeRvaCsv) {
    const data = fs.readFileSync(filename);
    const pdb = PDB.open(new Uint8Array(data));
    const addressMap = pdb.addressMap;

    const baseName = path.basename(filename, path.extname(filename));
    const csvPath = path.join(process.cwd(), `${baseName}_rva.csv`);
    const csvLines: string[] = [];
    csvLines.push("rva,function name");
    for (const e of entries) {
      const rva = addressMap.sectionOffsetToRva({ section: e.section, offset: e.offset });
      if (rva === null) continue;
      csvLines.push(`0x${(rva as number).toString(16)},${escapeCsv(e.name)}`);
    }
    fs.writeFileSync(csvPath, csvLines.join("\n") + "\n");
    console.log(`Wrote ${csvLines.length - 1} entries to ${csvPath}`);
    return;
  }

  const maxSizeWidth = entries.length > 0
    ? entries[entries.length - 1].size.toString().length
    : 0;

  for (const e of entries) {
    console.log(`${e.size.toString().padStart(maxSizeWidth)}  ${e.name}`);
  }

  console.log(`\n${entries.length} functions total`);
}

// Main
const args = process.argv.slice(2);
const writeRvaCsv = args.includes("-rva");
const filename = args.find(a => !a.startsWith("-"));
if (!filename) {
  console.error("Usage: bun run examples/func_sizes_with_llvm_pdbutil.ts [-rva] <input.pdb>");
  process.exit(1);
}

try {
  run(filename, writeRvaCsv);
} catch (e) {
  console.error(`error: ${e}`);
  process.exit(1);
}
