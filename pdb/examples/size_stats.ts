/**
 * size_stats - Calculate size information for symbols in a PDB file.
 *
 * Usage: bun run examples/size_stats.ts [-just-rva] <input.pdb>
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

function dumpSizeStats(filename: string, justRva: boolean): void {
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

  // Deduplicate: when the same symbol appears multiple times at the same address,
  // prefer module-private over global, and Data/Procedure over Public
  {
    const best = new Map<string, RawEntry>();
    for (const e of rawEntries) {
      const key = `${e.offset.section}:${e.offset.offset}`;
      const prev = best.get(key);
      if (!prev ||
        (prev.moduleName === "(global)" && e.moduleName !== "(global)") ||
        (prev.symbolKind === "Public" && e.symbolKind !== "Public")) {
        best.set(key, e);
      }
    }
    rawEntries.length = 0;
    rawEntries.push(...best.values());
    rawEntries.sort((a, b) => {
      if (a.offset.section !== b.offset.section) return a.offset.section - b.offset.section;
      return a.offset.offset - b.offset.offset;
    });
  }

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
  if (justRva) {
    csvLines.push("rva,symbol name");
    for (const e of entries) {
      csvLines.push([
        `0x${e.rva.toString(16)}`,
        escapeCsv(e.symbolName),
      ].join(","));
    }
  } else {
    csvLines.push("rva,symbol size,symbol name,symbol kind,module name,segment name");
    for (const e of entries) {
      csvLines.push([
        `0x${e.rva.toString(16)}`,
        `0x${e.size.toString(16)}`,
        escapeCsv(e.symbolName),
        escapeCsv(e.symbolKind),
        escapeCsv(e.moduleName),
        escapeCsv(e.segmentName),
      ].join(","));
    }
  }
  fs.writeFileSync(csvPath, csvLines.join("\n") + "\n");
  console.log(`Wrote ${entries.length} entries to ${csvPath}`);

  if (justRva) return;

  // Print top 100 largest symbols (largest last)
  const sorted = [...entries].sort((a, b) => a.size - b.size);
  const top100 = sorted.slice(-100);

  console.log(`\nTop ${top100.length} largest symbols (largest last):`);
  console.log("---");
  const maxSizeWidth = top100.length > 0
    ? top100[top100.length - 1].size.toString().length
    : 0;
  for (const e of top100) {
    const sizeStr = e.size.toString().padStart(maxSizeWidth);
    console.log(`${sizeStr}  ${e.symbolName}  [${e.symbolKind}]`);
  }

  // Check for overlaps and duplicates based on rva/size
  const byRva = [...entries].sort((a, b) => a.rva - b.rva);
  const overlaps: { a: SizeEntry; b: SizeEntry }[] = [];
  for (let i = 0; i < byRva.length - 1; i++) {
    const a = byRva[i];
    const b = byRva[i + 1];
    if (a.rva + a.size > b.rva) {
      overlaps.push({ a, b });
    }
  }

  if (overlaps.length > 0) {
    console.log(`\nOverlaps/duplicates found: ${overlaps.length}`);
    console.log("---");
    for (const { a, b } of overlaps) {
      const aEnd = a.rva + a.size;
      const overlapSize = aEnd - b.rva;
      if (a.rva === b.rva && a.size === b.size) {
        console.log(`DUPLICATE at 0x${a.rva.toString(16)} size=0x${a.size.toString(16)}:`);
      } else {
        console.log(`OVERLAP of 0x${overlapSize.toString(16)} bytes:`);
      }
      console.log(`  ${a.symbolName}  [${a.symbolKind}]  rva=0x${a.rva.toString(16)} size=0x${a.size.toString(16)}`);
      console.log(`  ${b.symbolName}  [${b.symbolKind}]  rva=0x${b.rva.toString(16)} size=0x${b.size.toString(16)}`);
    }
  } else {
    console.log("\nNo overlaps or duplicates found.");
  }
}

// Main
const args = process.argv.slice(2);
const justRva = args.includes("-just-rva");
const filename = args.find(a => !a.startsWith("-"));
if (!filename) {
  console.error("Usage: bun run examples/size_stats.ts [-just-rva] <input.pdb>");
  process.exit(1);
}

try {
  dumpSizeStats(filename, justRva);
} catch (e) {
  console.error(`error: ${e}`);
  process.exit(1);
}
