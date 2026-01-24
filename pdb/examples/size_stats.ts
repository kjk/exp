/**
 * size_stats - Calculate size information for symbols in a PDB file.
 *
 * Usage: bun run examples/size_stats.ts <input.pdb>
 *
 * Writes a CSV file (e.g. shell32_size_info.csv) with columns:
 *   module name, segment name, symbol kind, symbol name, rva, symbol size
 *
 * Also prints the 100 largest symbols to the console (smallest first).
 */

import { PDB, type PdbInternalSectionOffset, type SymbolData, type RawSymbol } from "../src/index.js";
import * as path from "path";
import * as fs from "fs";

interface SizeEntry {
  moduleName: string;
  segmentName: string;
  symbolKind: string;
  symbolName: string;
  rva: number;
  size: number;
  section: number;
  sectionOffset: number;
}

function getSymbolOffset(sym: SymbolData): PdbInternalSectionOffset | null {
  switch (sym.kind) {
    case "Procedure":
    case "ManagedProcedure":
    case "Data":
    case "Public":
    case "Thunk":
    case "ThreadStorage":
    case "Block":
    case "SeparatedCode":
    case "Label":
      return sym.offset;
    default:
      return null;
  }
}

function getSymbolSize(sym: SymbolData): number | null {
  switch (sym.kind) {
    case "Procedure":
    case "ManagedProcedure":
    case "Thunk":
    case "Block":
    case "SeparatedCode":
      return sym.len;
    default:
      return null;
  }
}

function getSymbolName(sym: SymbolData): string {
  switch (sym.kind) {
    case "Procedure":
    case "ManagedProcedure":
    case "Data":
    case "Public":
    case "Thunk":
    case "ThreadStorage":
    case "Block":
    case "Label":
      return sym.name ?? "";
    default:
      return "";
  }
}

function escapeCsv(s: string): string {
  if (s.includes(",") || s.includes('"') || s.includes("\n")) {
    return '"' + s.replace(/"/g, '""') + '"';
  }
  return s;
}

function dumpSizeStats(filename: string): void {
  const data = fs.readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(data));

  const sectionHeaders = pdb.sectionHeaders;
  const addressMap = pdb.addressMap;

  // Collect all symbols with offsets, grouped by section for size estimation
  const entries: SizeEntry[] = [];

  // Track raw entries that need size computation (no explicit size)
  interface RawEntry {
    moduleName: string;
    symbolKind: string;
    symbolName: string;
    offset: PdbInternalSectionOffset;
    explicitSize: number | null;
  }
  const rawEntries: RawEntry[] = [];

  function collectSymbols(moduleName: string, symbols: RawSymbol[]): void {
    for (const rawSym of symbols) {
      let sym: SymbolData;
      try {
        sym = pdb.parseSymbol(rawSym);
      } catch {
        continue;
      }

      const offset = getSymbolOffset(sym);
      if (!offset || offset.section === 0) continue;

      const name = getSymbolName(sym);
      if (!name) continue;

      const explicitSize = getSymbolSize(sym);

      rawEntries.push({
        moduleName,
        symbolKind: sym.kind,
        symbolName: name,
        offset,
        explicitSize,
      });
    }
  }

  // Collect global symbols
  collectSymbols("(global)", pdb.globalSymbols);

  // Collect module-private symbols
  for (const mod of pdb.modules) {
    const info = pdb.getModuleInfo(mod);
    if (!info) continue;
    collectSymbols(mod.moduleName, info.symbols);
  }

  // Sort by section then offset for size estimation
  rawEntries.sort((a, b) => {
    if (a.offset.section !== b.offset.section) return a.offset.section - b.offset.section;
    return a.offset.offset - b.offset.offset;
  });

  // Compute sizes: for entries without explicit size, use gap to next symbol in same section
  for (let i = 0; i < rawEntries.length; i++) {
    const entry = rawEntries[i];
    let size: number;

    if (entry.explicitSize !== null) {
      size = entry.explicitSize;
    } else {
      // Find next symbol in same section
      let nextOffset: number | null = null;
      for (let j = i + 1; j < rawEntries.length; j++) {
        if (rawEntries[j].offset.section === entry.offset.section) {
          if (rawEntries[j].offset.offset > entry.offset.offset) {
            nextOffset = rawEntries[j].offset.offset;
            break;
          }
        } else {
          break; // different section, stop looking
        }
      }

      if (nextOffset !== null) {
        size = nextOffset - entry.offset.offset;
      } else {
        // Use section end
        const sectionIdx = entry.offset.section - 1;
        if (sectionIdx < sectionHeaders.length) {
          const sectionSize = sectionHeaders[sectionIdx].virtualSize;
          size = sectionSize - entry.offset.offset;
          if (size < 0) size = 0;
        } else {
          size = 0;
        }
      }
    }

    if (size <= 0) continue;

    const sectionIdx = entry.offset.section - 1;
    const segmentName = sectionIdx < sectionHeaders.length
      ? sectionHeaders[sectionIdx].name
      : `section_${entry.offset.section}`;

    const rvaValue = addressMap.sectionOffsetToRva(entry.offset);
    if (rvaValue === null) continue;

    entries.push({
      moduleName: entry.moduleName,
      segmentName,
      symbolKind: entry.symbolKind,
      symbolName: entry.symbolName,
      rva: rvaValue as number,
      size,
      section: entry.offset.section,
      sectionOffset: entry.offset.offset,
    });
  }

  // Write CSV
  const baseName = path.basename(filename, path.extname(filename));
  const csvPath = path.join(process.cwd(), `${baseName}_size_info.csv`);

  const csvLines: string[] = [];
  csvLines.push("module name,segment name,symbol kind,symbol name,rva,symbol size");
  for (const e of entries) {
    csvLines.push([
      escapeCsv(e.moduleName),
      escapeCsv(e.segmentName),
      escapeCsv(e.symbolKind),
      escapeCsv(e.symbolName),
      `0x${e.rva.toString(16)}`,
      e.size.toString(),
    ].join(","));
  }
  fs.writeFileSync(csvPath, csvLines.join("\n") + "\n");
  console.log(`Wrote ${entries.length} entries to ${csvPath}`);

  // Print top 100 largest symbols (largest last)
  const sorted = [...entries].sort((a, b) => a.size - b.size);
  const top100 = sorted.slice(-100);

  console.log(`\nTop ${top100.length} largest symbols (largest last):`);
  console.log("---");
  for (const e of top100) {
    console.log(`${e.symbolName}  [${e.symbolKind}]  ${e.size} bytes`);
  }
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/size_stats.ts <input.pdb>");
  process.exit(1);
}

try {
  dumpSizeStats(filename);
} catch (e) {
  console.error(`error: ${e}`);
  process.exit(1);
}
