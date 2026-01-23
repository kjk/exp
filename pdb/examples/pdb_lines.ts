/**
 * pdb_lines - Dump line information for procedures in a PDB file.
 *
 * Usage: bun run examples/pdb_lines.ts <input.pdb>
 *
 * For each module, prints procedures and their source line mappings,
 * resolving file names through the string table.
 *
 * Ported from getsentry/pdb examples/pdb_lines.rs
 */

import { PDB, type SymbolData } from "../src/index.js";

function dumpPdb(filename: string): void {
  const fileData = require("fs").readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(fileData));

  const addressMap = pdb.addressMap;
  const stringTable = pdb.stringTable;

  console.log("Module private symbols:");

  for (const module of pdb.modules) {
    console.log();
    console.log(`Module: ${module.moduleName}`);

    const info = pdb.getModuleInfo(module);
    if (!info) {
      console.log("  no module info");
      continue;
    }

    for (const rawSymbol of info.symbols) {
      let data: SymbolData;
      try {
        data = pdb.parseSymbol(rawSymbol);
      } catch {
        continue;
      }

      if (data.kind !== "Procedure") continue;

      const sign = data.global ? "+" : "-";
      console.log(`${sign} ${data.name}`);

      // Find lines for this procedure by matching its section offset range
      const procSection = data.offset.section;
      const procOffset = data.offset.offset;
      const procEnd = procOffset + data.len;

      for (const line of info.lines) {
        // Match lines in the same section and within the procedure's offset range
        if (line.offset.section !== procSection) continue;
        if (line.offset.offset < procOffset || line.offset.offset >= procEnd) continue;

        // Resolve RVA
        const rvaValue = addressMap.sectionOffsetToRva(line.offset);
        const rvaStr = rvaValue !== null ? `0x${rvaValue.toString(16)}` : "??";

        // Resolve file name
        let fileName = "??";
        if (stringTable && info.fileChecksums.length > 0) {
          // The file index is an offset into the file checksums substream
          // We need to find the matching checksum entry
          const checksumIndex = Math.floor(line.fileIndex / 8);
          if (checksumIndex < info.fileChecksums.length) {
            const checksum = info.fileChecksums[checksumIndex];
            const resolved = stringTable.getString(checksum.nameRef);
            if (resolved) fileName = resolved;
          } else {
            // Try direct lookup by offset
            for (const checksum of info.fileChecksums) {
              const resolved = stringTable.getString(checksum.nameRef);
              if (resolved) {
                fileName = resolved;
                break;
              }
            }
          }
        }

        console.log(`  ${rvaStr} ${fileName}:${line.lineStart}`);
      }
    }
  }
}

// Main
const filename = process.argv[2];
if (!filename) {
  console.error("Usage: bun run examples/pdb_lines.ts <input.pdb>");
  process.exit(1);
}

try {
  dumpPdb(filename);
} catch (e) {
  console.error(`error dumping PDB: ${e}`);
  process.exit(1);
}
