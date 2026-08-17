/**
 * pdb2hpp - Generate C++ header declarations from PDB type information.
 *
 * Usage: bun run examples/pdb2hpp.ts <input.pdb> <ClassName|*>
 *
 * Finds the named class/struct in the PDB type information and generates
 * a C++ header file with its definition, including base classes, fields,
 * and methods. Recursively resolves all dependent types.
 * Use * as ClassName to dump all types.
 *
 * Ported from getsentry/pdb examples/pdb2hpp.rs
 */

import {
  PDB,
  type TypeData,
  type IdData,
  type TypeIndex,
  type RawItem,
  type FieldAttributes,
  type Variant,
  ItemFinder,
  MethodKind,
  MIN_TYPE_INDEX,
  primitiveTypeName,
} from "../src/index.js";

type TypeSet = Set<TypeIndex>;

interface SourceInfo {
  sourceFile: string;
  moduleName: string;
  line: number;
}

/** Resolve a type index to a human-readable C++ type name. */
function typeName(
  finder: ItemFinder,
  pdb: PDB,
  typeIndex: TypeIndex,
  neededTypes: TypeSet,
): string {
  // Handle primitive types (index < 0x1000)
  if (typeIndex < MIN_TYPE_INDEX) {
    return primitiveTypeName(typeIndex);
  }

  const item = finder.find(typeIndex);
  if (!item) return `Type${typeIndex}`;

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return `Type${typeIndex} /* parse error */`;
  }

  switch (data.kind) {
    case "Class":
    case "Structure":
    case "Interface":
      neededTypes.add(typeIndex);
      return data.name;

    case "Enumeration":
      neededTypes.add(typeIndex);
      return data.name;

    case "Union":
      neededTypes.add(typeIndex);
      return data.name;

    case "Pointer":
      return `${typeName(finder, pdb, data.underlyingType, neededTypes)}*`;

    case "Modifier":
      if (data.flags.isConst) {
        return `const ${typeName(finder, pdb, data.underlyingType, neededTypes)}`;
      } else if (data.flags.isVolatile) {
        return `volatile ${typeName(finder, pdb, data.underlyingType, neededTypes)}`;
      }
      return typeName(finder, pdb, data.underlyingType, neededTypes);

    case "Array": {
      let name = typeName(finder, pdb, data.elementType, neededTypes);
      for (const dim of data.dimensions) {
        name = `${name}[${dim}]`;
      }
      return name;
    }

    default:
      return `Type${typeIndex} /* unhandled */`;
  }
}

// ─── Data structures for output ───

interface BaseClass {
  typeName: string;
  offset: number;
}

interface Field {
  typeName: string;
  name: string;
  offset: number;
}

interface Method {
  name: string;
  returnTypeName: string;
  arguments: string[];
  isVirtual: boolean;
}

interface ClassDef {
  classKind: "class" | "struct" | "interface";
  name: string;
  sourceInfo?: SourceInfo;
  baseClasses: BaseClass[];
  fields: Field[];
  instanceMethods: Method[];
  staticMethods: Method[];
}

interface EnumValue {
  name: string;
  value: Variant;
}

interface EnumDef {
  name: string;
  sourceInfo?: SourceInfo;
  underlyingTypeName: string;
  values: EnumValue[];
}

interface ForwardRef {
  classKind: "class" | "struct" | "interface";
  name: string;
}

interface OutputData {
  forwardReferences: ForwardRef[];
  classes: ClassDef[];
  enums: EnumDef[];
}

/** Resolve a method's return type and argument list. */
function resolveMethod(
  name: string,
  attributes: FieldAttributes,
  finder: ItemFinder,
  pdb: PDB,
  methodTypeIndex: TypeIndex,
  neededTypes: TypeSet,
): Method | null {
  const item = finder.find(methodTypeIndex);
  if (!item) return null;

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return null;
  }

  if (data.kind !== "MemberFunction") return null;

  const returnTypeName = typeName(finder, pdb, data.returnType, neededTypes);
  const args = resolveArgumentList(finder, pdb, data.argumentList, neededTypes);
  const isVirtual = attributes.mprop === MethodKind.Virtual ||
    attributes.mprop === MethodKind.IntroducingVirtual ||
    attributes.mprop === MethodKind.PureVirtual ||
    attributes.mprop === MethodKind.PureIntroducingVirtual;

  return { name, returnTypeName, arguments: args, isVirtual };
}

/** Resolve an argument list type to a list of type name strings. */
function resolveArgumentList(
  finder: ItemFinder,
  pdb: PDB,
  argListIndex: TypeIndex,
  neededTypes: TypeSet,
): string[] {
  const item = finder.find(argListIndex);
  if (!item) return [];

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return [];
  }

  if (data.kind !== "ArgumentList") return [];
  return data.arguments.map((ti) => typeName(finder, pdb, ti, neededTypes));
}

/** Add fields from a FieldList type to the class definition. */
function addFields(
  classDef: ClassDef,
  finder: ItemFinder,
  pdb: PDB,
  fieldListIndex: TypeIndex,
  neededTypes: TypeSet,
): void {
  const item = finder.find(fieldListIndex);
  if (!item) return;

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return;
  }

  if (data.kind !== "FieldList") return;

  for (const field of data.fields) {
    addField(classDef, finder, pdb, field, neededTypes);
  }

  if (data.continuation) {
    addFields(classDef, finder, pdb, data.continuation, neededTypes);
  }
}

/** Add a single field to the class definition. */
function addField(
  classDef: ClassDef,
  finder: ItemFinder,
  pdb: PDB,
  field: TypeData,
  neededTypes: TypeSet,
): void {
  switch (field.kind) {
    case "Member":
      classDef.fields.push({
        typeName: typeName(finder, pdb, field.fieldType, neededTypes),
        name: field.name,
        offset: Number(field.offset),
      });
      break;

    case "Method": {
      const method = resolveMethod(
        field.name,
        field.attributes,
        finder,
        pdb,
        field.methodType,
        neededTypes,
      );
      if (method) {
        if (field.attributes.mprop === MethodKind.Static) {
          classDef.staticMethods.push(method);
        } else {
          classDef.instanceMethods.push(method);
        }
      }
      break;
    }

    case "OverloadedMethod": {
      // Find the method list and add each overload
      const mlItem = finder.find(field.methodList);
      if (!mlItem) break;

      let mlData: TypeData;
      try {
        mlData = pdb.parseType(mlItem);
      } catch {
        break;
      }

      if (mlData.kind !== "MethodList") break;
      for (const entry of mlData.methods) {
        const method = resolveMethod(
          field.name,
          entry.attributes,
          finder,
          pdb,
          entry.methodType,
          neededTypes,
        );
        if (method) {
          if (entry.attributes.mprop === MethodKind.Static) {
            classDef.staticMethods.push(method);
          } else {
            classDef.instanceMethods.push(method);
          }
        }
      }
      break;
    }

    case "BaseClass":
      classDef.baseClasses.push({
        typeName: typeName(finder, pdb, field.baseType, neededTypes),
        offset: Number(field.offset),
      });
      break;

    case "VirtualBaseClass":
      classDef.baseClasses.push({
        typeName: typeName(finder, pdb, field.baseType, neededTypes),
        offset: Number(field.basePointerOffset),
      });
      break;

    default:
      // Ignore other field types
      break;
  }
}

/** Add enum values from a FieldList to the enum definition. */
function addEnumFields(
  enumDef: EnumDef,
  finder: ItemFinder,
  pdb: PDB,
  fieldListIndex: TypeIndex,
): void {
  const item = finder.find(fieldListIndex);
  if (!item) return;

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return;
  }

  if (data.kind !== "FieldList") return;

  for (const field of data.fields) {
    if (field.kind === "Enumerate") {
      enumDef.values.push({ name: field.name, value: field.value });
    }
  }

  if (data.continuation) {
    addEnumFields(enumDef, finder, pdb, data.continuation);
  }
}

/** Add a type to the output data. */
function addType(
  output: OutputData,
  finder: ItemFinder,
  pdb: PDB,
  typeIndex: TypeIndex,
  neededTypes: TypeSet,
  sourceMap?: Map<TypeIndex, SourceInfo>,
): void {
  const item = finder.find(typeIndex);
  if (!item) return;

  let data: TypeData;
  try {
    data = pdb.parseType(item);
  } catch {
    return;
  }

  switch (data.kind) {
    case "Class":
    case "Structure":
    case "Interface": {
      if (data.properties.isForwardReference) {
        const classKind = data.kind === "Class" ? "class"
          : data.kind === "Interface" ? "interface" : "struct";
        output.forwardReferences.push({ classKind, name: data.name });
        return;
      }

      const classKind = data.kind === "Class" ? "class"
        : data.kind === "Interface" ? "interface" : "struct";
      const classDef: ClassDef = {
        classKind,
        name: data.name,
        sourceInfo: sourceMap?.get(typeIndex),
        baseClasses: [],
        fields: [],
        instanceMethods: [],
        staticMethods: [],
      };

      if (data.fields) {
        addFields(classDef, finder, pdb, data.fields, neededTypes);
      }

      output.classes.unshift(classDef);
      break;
    }

    case "Enumeration": {
      const enumDef: EnumDef = {
        name: data.name,
        sourceInfo: sourceMap?.get(typeIndex),
        underlyingTypeName: typeName(finder, pdb, data.underlyingType, neededTypes),
        values: [],
      };

      if (data.fields) {
        addEnumFields(enumDef, finder, pdb, data.fields);
      }

      output.enums.unshift(enumDef);
      break;
    }

    default:
      console.error(`warning: don't know how to add type kind "${data.kind}"`);
      break;
  }
}

/** Format a Variant value for display. */
function formatVariant(v: Variant): string {
  const isSigned = v.kind.startsWith("i");
  if (typeof v.value === "bigint") {
    if (isSigned) {
      return v.value.toString();
    }
    return `0x${v.value.toString(16)}`;
  }
  if (isSigned) {
    return v.value.toString();
  }
  if (v.value <= 0xff) return `0x${v.value.toString(16).padStart(2, "0")}`;
  if (v.value <= 0xffff) return `0x${v.value.toString(16).padStart(4, "0")}`;
  return `0x${v.value.toString(16).padStart(8, "0")}`;
}

/** Format the output data as C++ header. */
function formatOutput(output: OutputData): string {
  const lines: string[] = [];
  lines.push("// automatically generated by pdb2hpp");
  lines.push("// do not edit");

  if (output.forwardReferences.length > 0) {
    lines.push("");
    for (const ref of output.forwardReferences) {
      lines.push(`${ref.classKind} ${ref.name};`);
    }
  }

  for (const enumDef of output.enums) {
    lines.push("");
    if (enumDef.sourceInfo) {
      lines.push(`// ${enumDef.sourceInfo.sourceFile || enumDef.sourceInfo.moduleName}:${enumDef.sourceInfo.line}`);
    }
    lines.push(`enum ${enumDef.name} /* stored as ${enumDef.underlyingTypeName} */ {`);
    for (const val of enumDef.values) {
      lines.push(`\t${val.name} = ${formatVariant(val.value)},`);
    }
    lines.push("}");
  }

  for (const classDef of output.classes) {
    lines.push("");
    if (classDef.sourceInfo) {
      lines.push(`// ${classDef.sourceInfo.sourceFile || classDef.sourceInfo.moduleName}:${classDef.sourceInfo.line}`);
    }
    let header = `${classDef.classKind} ${classDef.name} `;
    if (classDef.baseClasses.length > 0) {
      for (let i = 0; i < classDef.baseClasses.length; i++) {
        header += i === 0 ? ": " : ", ";
        header += classDef.baseClasses[i].typeName;
      }
    }
    header += " {";
    lines.push(header);

    for (const base of classDef.baseClasses) {
      lines.push(`\t/* offset ${String(base.offset).padStart(3)} */ /* fields for ${base.typeName} */`);
    }

    for (const field of classDef.fields) {
      lines.push(`\t/* offset ${String(field.offset).padStart(3)} */ ${field.typeName} ${field.name};`);
    }

    if (classDef.instanceMethods.length > 0) {
      lines.push("\t");
      for (const method of classDef.instanceMethods) {
        const virt = method.isVirtual ? "virtual " : "";
        lines.push(`\t${virt}${method.returnTypeName} ${method.name}(${method.arguments.join(", ")});`);
      }
    }

    if (classDef.staticMethods.length > 0) {
      lines.push("\t");
      for (const method of classDef.staticMethods) {
        const virt = method.isVirtual ? "virtual " : "";
        lines.push(`\t${virt}static ${method.returnTypeName} ${method.name}(${method.arguments.join(", ")});`);
      }
    }

    lines.push("}");
  }

  return lines.join("\n") + "\n";
}

/** Build a map from TypeIndex to source file/module info using the IPI stream. */
function buildSourceMap(pdb: PDB): Map<TypeIndex, SourceInfo> {
  const map = new Map<TypeIndex, SourceInfo>();
  const idInfo = pdb.idInformation;
  if (!idInfo) return map;

  const idFinder = new ItemFinder(idInfo);
  const modules = pdb.modules;

  for (const item of idInfo.items) {
    let data: IdData;
    try {
      data = pdb.parseId(item);
    } catch {
      continue;
    }

    if (data.kind !== "UdtModuleSourceLine" && data.kind !== "UdtSourceLine") {
      continue;
    }

    // Resolve the source file name from the IdIndex
    let sourceFile = "";
    const sfItem = idFinder.find(data.sourceFile);
    if (sfItem) {
      try {
        const sfData = pdb.parseId(sfItem);
        if (sfData.kind === "StringId") {
          sourceFile = sfData.name;
        }
      } catch { /* ignore */ }
    }

    let moduleName = "";
    if (data.kind === "UdtModuleSourceLine" && data.module < modules.length) {
      moduleName = modules[data.module].moduleName;
    }

    map.set(data.udtType, { sourceFile, moduleName, line: data.line });
  }

  return map;
}

function writeClass(filename: string, className: string): void {
  const data = require("fs").readFileSync(filename);
  const pdb = PDB.open(new Uint8Array(data));

  const typeInfo = pdb.typeInformation;
  const finder = new ItemFinder(typeInfo);
  const sourceMap = buildSourceMap(pdb);

  const neededTypes: TypeSet = new Set();
  const output: OutputData = {
    forwardReferences: [],
    classes: [],
    enums: [],
  };

  // Find the target class(es)
  let found = false;
  const matchAll = className === "*";
  for (const item of typeInfo.items) {
    let typeData: TypeData;
    try {
      typeData = pdb.parseType(item);
    } catch {
      continue;
    }

    if (matchAll) {
      if (
        (typeData.kind === "Class" || typeData.kind === "Structure" ||
          typeData.kind === "Enumeration" || typeData.kind === "Union" ||
          typeData.kind === "Interface") &&
        !typeData.properties.isForwardReference
      ) {
        addType(output, finder, pdb, item.index, neededTypes, sourceMap);
        found = true;
      }
    } else if (
      (typeData.kind === "Class" || typeData.kind === "Structure") &&
      typeData.name === className &&
      !typeData.properties.isForwardReference
    ) {
      addType(output, finder, pdb, item.index, neededTypes, sourceMap);
      found = true;
      break;
    }
  }

  // Resolve all needed types iteratively (skip when dumping all, since everything is already added)
  if (!matchAll) {
    const processed = new Set<TypeIndex>();
    while (neededTypes.size > 0) {
      const next = [...neededTypes].pop()!;
      neededTypes.delete(next);
      if (processed.has(next)) continue;
      processed.add(next);
      addType(output, finder, pdb, next, neededTypes, sourceMap);
    }
  }

  if (!found) {
    console.error(matchAll ? "no types found in PDB" : `sorry, class ${className} was not found`);
  } else {
    process.stdout.write(formatOutput(output));
  }
}

// Main
const filename = process.argv[2];
const className = process.argv[3];

if (!filename || !className) {
  console.error("Usage: bun run examples/pdb2hpp.ts <input.pdb> <ClassName|*>");
  process.exit(1);
}

try {
  writeClass(filename, className);
} catch (e) {
  console.error(`error dumping PDB: ${e}`);
  process.exit(1);
}
