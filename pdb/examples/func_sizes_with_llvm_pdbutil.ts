/**
 * func_sizes_with_llvm_pdbutil - Extract function sizes using llvm-pdbutil.
 *
 * Usage: bun run examples/func_sizes_with_llvm_pdbutil.ts <input.pdb>
 *
 * Runs llvm-pdbutil to get module symbols, parses function sizes and names,
 * and prints them sorted by size (largest last).
 */

import { execSync } from "child_process";

interface FuncEntry {
  name: string;
  size: number;
}

function run(filename: string): void {
  const output = execSync(
    `llvm-pdbutil pretty -module-syms -sym-types=funcs "${filename}"`,
    { encoding: "utf-8", maxBuffer: 256 * 1024 * 1024 }
  );

  const entries: FuncEntry[] = [];
  const sizeofRe = /sizeof=(\d+)\]\s*\([^)]*\)\s*(.*)/;

  for (const line of output.split("\n")) {
    const trimmed = line.trim();
    if (!trimmed.startsWith("func [")) continue;

    const match = trimmed.match(sizeofRe);
    if (!match) continue;

    const size = parseInt(match[1], 10);
    const name = match[2].trim();
    if (name) {
      entries.push({ name, size });
    }
  }

  entries.sort((a, b) => a.size - b.size);

  const maxSizeWidth = entries.length > 0
    ? entries[entries.length - 1].size.toString().length
    : 0;

  for (const e of entries) {
    console.log(`${e.size.toString().padStart(maxSizeWidth)}  ${e.name}`);
  }

  console.log(`\n${entries.length} functions total`);
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/func_sizes_with_llvm_pdbutil.ts <input.pdb>");
  process.exit(1);
}

try {
  run(filename);
} catch (e) {
  console.error(`error: ${e}`);
  process.exit(1);
}
