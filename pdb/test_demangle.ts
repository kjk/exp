import { demangleCpp } from "./src/demangle_cpp.js";
import * as fs from "fs";

function parseCsvLine(line: string): string[] {
  const fields: string[] = [];
  let i = 0;
  while (i < line.length) {
    if (line[i] === '"') {
      i++;
      let field = "";
      while (i < line.length) {
        if (line[i] === '"') {
          if (i + 1 < line.length && line[i + 1] === '"') {
            field += '"';
            i += 2;
          } else {
            i++;
            break;
          }
        } else {
          field += line[i];
          i++;
        }
      }
      fields.push(field);
      if (i < line.length && line[i] === ',') i++;
    } else {
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

const lines = fs.readFileSync("mangle_test_data.csv", "utf-8").split("\n").slice(1).filter(l => l.trim());

let total = 0;
let mangledTotal = 0;
let mangledPass = 0;
let unmangledTotal = 0;
let unmangledPass = 0;
const failures: { name: string; expected: string; got: string }[] = [];

for (const line of lines) {
  const fields = parseCsvLine(line);
  if (fields.length < 3) continue;
  const [_rva, name, expected] = fields;
  total++;

  const got = demangleCpp(name);

  if (name.startsWith("?")) {
    mangledTotal++;
    if (got === expected) {
      mangledPass++;
    } else if (failures.length < 20) {
      failures.push({ name, expected, got });
    }
  } else {
    unmangledTotal++;
    if (got === expected) {
      unmangledPass++;
    }
  }
}

console.log(`Total entries: ${total}`);
console.log(`Mangled (start with ?): ${mangledTotal}, pass: ${mangledPass} (${(100*mangledPass/mangledTotal).toFixed(1)}%)`);
console.log(`Unmangled: ${unmangledTotal}, pass: ${unmangledPass} (${(100*unmangledPass/unmangledTotal).toFixed(1)}%)`);
console.log(`\nFirst ${Math.min(failures.length, 20)} failures for mangled names:`);
for (const f of failures) {
  console.log(`  NAME: ${f.name}`);
  console.log(`  WANT: ${f.expected}`);
  console.log(`  GOT:  ${f.got}`);
  console.log();
}
