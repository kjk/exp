import { PDB } from "./src/index.ts";

const pdbPath = `C:\\Users\\kjk\\OneDrive\\bin\\sg.pdb`;
const data = await Bun.file(pdbPath).bytes();
const pdb = PDB.open(data);

// Metadata
console.log(pdb.guidString, pdb.age, pdb.machineType);

// Types, symbols, modules, line info all accessible
for (const mod of pdb.modules) {
  const info = pdb.getModuleInfo(mod);
  console.log(info);
}
