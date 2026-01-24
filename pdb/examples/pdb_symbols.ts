/**
 * pdb_symbols - Dump global and module-private symbols from a PDB file.
 *
 * Usage: bun run examples/pdb_symbols.ts <input.pdb>
 *
 * Ported from getsentry/pdb examples/pdb_symbols.rs
 */

import { PDB, type PdbInternalSectionOffset, type RawSymbol, type SymbolData } from "../src/index.js";

function printRow(offset: PdbInternalSectionOffset, kind: string, name: string): void {
  console.log(`${offset.section.toString(16)}\t${offset.offset.toString(16)}\t${kind}\t${name}`);
}

function isPrintableSymbol(pdb: PDB, symbol: RawSymbol): boolean {
  try {
    const data = pdb.parseSymbol(symbol);
    return data.kind === "Public" || data.kind === "Data" || data.kind === "Procedure";
  } catch {
    return false;
  }
}

function printSymbol(pdb: PDB, symbol: RawSymbol): void {
  let data: SymbolData;
  try {
    data = pdb.parseSymbol(symbol);
  } catch (e) {
    console.error(`error parsing symbol: ${e}`);
    return;
  }

  switch (data.kind) {
    case "Public":
      printRow(data.offset, "function", data.name);
      break;
    case "Data":
      printRow(data.offset, "data", data.name);
      break;
    case "Procedure":
      printRow(data.offset, "function", data.name);
      break;
    default:
      // ignore everything else
      break;
  }
}

function walkSymbols(pdb: PDB, symbols: RawSymbol[]): void {
  console.log("segment\toffset\tkind\tname");
  for (const symbol of symbols) {
    printSymbol(pdb, symbol);
  }
}

function dumpPdb(filename: string): void {
  const data = require("fs").readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(data));

  console.log("Global symbols:");
  walkSymbols(pdb, pdb.globalSymbols);

  console.log("\nModule private symbols:");
  for (const module of pdb.modules) {
    const info = pdb.getModuleInfo(module);
    if (!info) continue;
    if (!info.symbols.some(s => isPrintableSymbol(pdb, s))) continue;
    console.log(`\nModule: ${module.objectFileName}`);
    walkSymbols(pdb, info.symbols);
  }
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/pdb_symbols.ts <input.pdb>");
  process.exit(1);
}

try {
  dumpPdb(filename);
} catch (e) {
  console.error(`error dumping PDB: ${e}`);
  process.exit(1);
}
