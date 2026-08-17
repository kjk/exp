/**
 * func_sizes_with_llvm_pdbutil - Extract function sizes using llvm-pdbutil.
 *
 * Usage: bun run examples/func_sizes_with_llvm_pdbutil.ts [-rva] <input.pdb>
 *
 * Runs llvm-pdbutil to get module symbols, parses function sizes and names,
 * and prints them sorted by size (largest last).
 */

import { execSync } from "child_process";
import * as fs from "fs";
import * as path from "path";

interface FuncEntry {
  name: string;
  size: number;
  rva: number;
}

function escapeCsv(s: string): string {
  if (s.includes(",") || s.includes('"') || s.includes("\n")) {
    return '"' + s.replace(/"/g, '""') + '"';
  }
  return s;
}

function extractFuncSizeNameRva(line: string): { size: number; name: string; rva: number } | null {
  const trimmed = line.trim();
  if (!trimmed.startsWith("func [")) return null;

  // RVA: first 0x hex value after "["
  const rvaMatch = trimmed.match(/\[0x([0-9A-Fa-f]+)/);
  if (!rvaMatch) return null;
  const rva = parseInt(rvaMatch[1], 16);

  // Size: value after "sizeof="
  const sizeMatch = trimmed.match(/sizeof=\s*(\d+)/);
  if (!sizeMatch) return null;
  const size = parseInt(sizeMatch[1], 10);

  // Name: after "]", skip the first parenthesized group (frame type / section:offset)
  const bracketEnd = trimmed.indexOf("]");
  if (bracketEnd < 0) return null;
  const afterBracket = trimmed.substring(bracketEnd + 1).trim();
  // Skip one (...) group
  const nameMatch = afterBracket.match(/^\([^)]*\)\s*(.*)/);
  const name = nameMatch ? nameMatch[1].trim() : afterBracket.trim();
  if (!name) return null;

  return { size, name, rva };
}

function run(filename: string, writeRvaCsv: boolean): void {
  const output = execSync(
    `llvm-pdbutil pretty -module-syms -sym-types=funcs "${filename}"`,
    { encoding: "utf-8", maxBuffer: 256 * 1024 * 1024 }
  );

  const entries: FuncEntry[] = [];

  for (const line of output.split("\n")) {
    const parsed = extractFuncSizeNameRva(line);
    if (parsed) {
      entries.push(parsed);
    }
  }

  entries.sort((a, b) => a.size - b.size);

  if (writeRvaCsv) {
    const baseName = path.basename(filename, path.extname(filename));
    const csvPath = path.join(process.cwd(), `${baseName}_rva.csv`);
    const csvLines: string[] = [];
    csvLines.push("rva,function name");
    for (const e of entries) {
      csvLines.push(`0x${e.rva.toString(16)},${escapeCsv(e.name)}`);
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
