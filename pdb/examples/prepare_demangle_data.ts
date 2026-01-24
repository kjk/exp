/**
 * prepare_demangle_data - Build a CSV of mangled/demangled name pairs.
 *
 * Reads SumatraPDF_rva.csv (rva, demangled name) and
 * SumatraPDF_size_info.csv (rva, mangled name), then writes
 * mangle_test_data.csv with entries where both files have an RVA
 * but the names differ.
 */

import * as fs from "fs";

function parseCsvLine(line: string): string[] {
  const fields: string[] = [];
  let i = 0;
  while (i < line.length) {
    if (line[i] === '"') {
      // Quoted field
      i++;
      let field = "";
      while (i < line.length) {
        if (line[i] === '"') {
          if (i + 1 < line.length && line[i + 1] === '"') {
            field += '"';
            i += 2;
          } else {
            i++; // closing quote
            break;
          }
        } else {
          field += line[i];
          i++;
        }
      }
      fields.push(field);
      if (i < line.length && line[i] === ',') i++; // skip comma
    } else {
      // Unquoted field
      const next = line.indexOf(',', i);
      if (next < 0) {
        fields.push(line.substring(i));
        break;
      } else {
        fields.push(line.substring(i, next));
        i = next + 1;
      }
    }
  }
  return fields;
}

function escapeCsv(s: string): string {
  if (s.includes(",") || s.includes('"') || s.includes("\n")) {
    return '"' + s.replace(/"/g, '""') + '"';
  }
  return s;
}

// Read SumatraPDF_rva.csv: rva, demangled name
const rvaLines = fs.readFileSync("SumatraPDF_rva.csv", "utf-8").split("\n");
const demangledByRva = new Map<string, string>();
for (let i = 1; i < rvaLines.length; i++) {
  const line = rvaLines[i].trim();
  if (!line) continue;
  const fields = parseCsvLine(line);
  if (fields.length >= 2) {
    demangledByRva.set(fields[0], fields[1]);
  }
}

// Read SumatraPDF_size_info.csv: rva, mangled name
const sizeLines = fs.readFileSync("SumatraPDF_size_info.csv", "utf-8").split("\n");
const mangledByRva = new Map<string, string>();
for (let i = 1; i < sizeLines.length; i++) {
  const line = sizeLines[i].trim();
  if (!line) continue;
  const fields = parseCsvLine(line);
  if (fields.length >= 2) {
    mangledByRva.set(fields[0], fields[1]);
  }
}

// Write mangle_test_data.csv for RVAs present in both with different names
const out: string[] = [];
out.push("rva,name,demangled name");
for (const [rva, mangled] of mangledByRva) {
  const demangled = demangledByRva.get(rva);
  if (demangled && demangled !== mangled) {
    out.push(`${rva},${escapeCsv(mangled)},${escapeCsv(demangled)}`);
  }
}

fs.writeFileSync("mangle_test_data.csv", out.join("\n") + "\n");
console.log(`Wrote ${out.length - 1} entries to mangle_test_data.csv`);
