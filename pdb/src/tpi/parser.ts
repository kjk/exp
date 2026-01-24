import { ParseBuffer } from "../buffer.js";
import { PdbError } from "../error.js";
import type { MsfStream } from "../msf.js";
import type { TypeIndex, IdIndex } from "../common.js";
import { MIN_TYPE_INDEX } from "../common.js";
import * as C from "./constants.js";
import type {
  TypeData,
  IdData,
  FieldAttributes,
  MethodListEntry,
} from "./types.js";
import {
  parseTypeProperties,
  parseFieldAttributes,
  parsePointerAttributes,
  parseFunctionAttributes,
  PointerMode,
  MethodKind,
} from "./types.js";

/** Header of the TPI/IPI stream. */
interface TpiHeader {
  version: number;
  headerSize: number;
  minimumIndex: number;
  maximumIndex: number;
}

/** A raw type/id item from the stream. */
export interface RawItem {
  index: number;
  kind: number;
  data: Uint8Array;
}

/** Parsed type information stream. */
export interface TypeInformation {
  header: TpiHeader;
  items: RawItem[];
}

/** Parse a TPI or IPI stream header. */
function parseTpiHeader(buf: ParseBuffer): TpiHeader {
  const version = buf.readU32();
  const headerSize = buf.readU32();
  const minimumIndex = buf.readU32();
  const maximumIndex = buf.readU32();
  // Skip remaining header fields (byte count of type records follows, plus hash info)
  const remaining = headerSize - buf.pos;
  if (remaining > 0) {
    buf.take(remaining);
  }
  return { version, headerSize, minimumIndex, maximumIndex };
}

/** Parse the Type Information (TPI) or Id Information (IPI) stream. */
export function parseItemInformation(stream: MsfStream): TypeInformation {
  const buf = new ParseBuffer(stream.data);
  const header = parseTpiHeader(buf);
  const items: RawItem[] = [];

  let currentIndex = header.minimumIndex;
  while (!buf.isEmpty) {
    const length = buf.readU16();
    if (length < 2) {
      throw PdbError.typeTooShort();
    }
    const itemData = buf.take(length);
    const kind = itemData[0] | (itemData[1] << 8);
    items.push({
      index: currentIndex,
      kind,
      data: itemData,
    });
    currentIndex++;
  }

  return { header, items };
}

/** Finder for efficient random access to types by index. */
export class ItemFinder {
  private items: RawItem[];
  private minimumIndex: number;

  constructor(info: TypeInformation) {
    this.items = info.items;
    this.minimumIndex = info.header.minimumIndex;
  }

  /** Find an item by its index. */
  find(index: number): RawItem | null {
    if (index < this.minimumIndex) return null;
    const arrayIndex = index - this.minimumIndex;
    if (arrayIndex >= this.items.length) return null;
    return this.items[arrayIndex];
  }

  /** Get the total number of items. */
  get length(): number {
    return this.items.length;
  }
}

/** Parse a type record from its raw bytes. */
export function parseTypeData(item: RawItem): TypeData {
  const buf = new ParseBuffer(item.data);
  const kind = buf.readU16(); // skip kind which we already have
  return parseTypeRecord(buf, kind);
}

/** Parse an id record from its raw bytes. */
export function parseIdData(item: RawItem): IdData {
  const buf = new ParseBuffer(item.data);
  const kind = buf.readU16();
  return parseIdRecord(buf, kind);
}

/** Internal: parse a type record body. */
function parseTypeRecord(buf: ParseBuffer, kind: number): TypeData {
  switch (kind) {
    case C.LF_CLASS:
    case C.LF_CLASS_ST:
    case C.LF_STRUCTURE:
    case C.LF_STRUCTURE_ST:
    case C.LF_INTERFACE: {
      const count = buf.readU16();
      const properties = parseTypeProperties(buf.readU16());
      const fields = readTypeIndexOrNull(buf);
      const derivedFrom = readTypeIndexOrNull(buf);
      const vtableShape = readTypeIndexOrNull(buf);
      const size = buf.readVariant().value;
      const name = readTypeName(buf, kind);
      const uniqueName = properties.hasUniqueName ? readTypeName(buf, kind) : null;
      const typeKind = kind === C.LF_INTERFACE
        ? "Interface" as const
        : (kind === C.LF_CLASS || kind === C.LF_CLASS_ST)
          ? "Class" as const
          : "Structure" as const;
      return {
        kind: typeKind,
        count,
        properties,
        fields: fields || null,
        derivedFrom: derivedFrom || null,
        vtableShape: vtableShape || null,
        size,
        name,
        uniqueName,
      };
    }

    // LF_STRUCTURE19 has a different field order from the regular LF_STRUCTURE
    case C.LF_STRUCTURE19: {
      const properties = parseTypeProperties(buf.readU32() & 0xffff);
      const fields = readTypeIndexOrNull(buf);
      const derivedFrom = readTypeIndexOrNull(buf);
      const vtableShape = readTypeIndexOrNull(buf);
      const count = buf.readU16();
      const size = buf.readVariant().value;
      const name = readTypeName(buf, kind);
      const uniqueName = properties.hasUniqueName ? readTypeName(buf, kind) : null;
      return {
        kind: "Structure" as const,
        count,
        properties,
        fields: fields || null,
        derivedFrom: derivedFrom || null,
        vtableShape: vtableShape || null,
        size,
        name,
        uniqueName,
      };
    }

    case C.LF_UNION:
    case C.LF_UNION_ST: {
      const count = buf.readU16();
      const properties = parseTypeProperties(buf.readU16());
      const fields = readTypeIndexOrNull(buf);
      const sizeVariant = buf.readVariant();
      const size = sizeVariant.value;
      const name = readTypeName(buf, kind);
      const uniqueName = properties.hasUniqueName ? readTypeName(buf, kind) : null;
      return {
        kind: "Union",
        count,
        properties,
        fields: fields || null,
        size,
        name,
        uniqueName,
      };
    }

    case C.LF_ENUM:
    case C.LF_ENUM_ST: {
      const count = buf.readU16();
      const properties = parseTypeProperties(buf.readU16());
      const underlyingType: TypeIndex = buf.readU32();
      const fields = readTypeIndexOrNull(buf);
      const name = readTypeName(buf, kind);
      const uniqueName = properties.hasUniqueName ? readTypeName(buf, kind) : null;
      return {
        kind: "Enumeration",
        count,
        properties,
        underlyingType,
        fields: fields || null,
        name,
        uniqueName,
      };
    }

    case C.LF_PROCEDURE: {
      const returnTypeRaw: TypeIndex = buf.readU32();
      const returnType = (returnTypeRaw === 0 || returnTypeRaw === 0xffff) ? null : returnTypeRaw;
      const callConv = buf.readU8();
      const funcAttr = buf.readU8();
      const attributes = parseFunctionAttributes(callConv, funcAttr);
      const parameterCount = buf.readU16();
      const argumentList: TypeIndex = buf.readU32();
      return {
        kind: "Procedure",
        returnType,
        attributes,
        parameterCount,
        argumentList,
      };
    }

    case C.LF_MFUNCTION: {
      const returnType: TypeIndex = buf.readU32();
      const classType: TypeIndex = buf.readU32();
      const thisPointerTypeRaw: TypeIndex = buf.readU32();
      const thisPointerType = (thisPointerTypeRaw === 0 || thisPointerTypeRaw === 0xffff) ? null : thisPointerTypeRaw;
      const callConv = buf.readU8();
      const funcAttr = buf.readU8();
      const attributes = parseFunctionAttributes(callConv, funcAttr);
      const parameterCount = buf.readU16();
      const argumentList: TypeIndex = buf.readU32();
      const thisAdjustment = buf.readI32();
      return {
        kind: "MemberFunction",
        returnType,
        classType,
        thisPointerType,
        attributes,
        parameterCount,
        argumentList,
        thisAdjustment,
      };
    }

    case C.LF_POINTER: {
      const underlyingType: TypeIndex = buf.readU32();
      const attributes = parsePointerAttributes(buf.readU32());
      let containingClass: TypeIndex | null = null;
      if (attributes.pointerMode === PointerMode.Member ||
          attributes.pointerMode === PointerMode.MemberFunction) {
        if (buf.remaining >= 4) {
          containingClass = buf.readU32();
        }
      }
      return {
        kind: "Pointer",
        underlyingType,
        attributes,
        containingClass,
      };
    }

    case C.LF_MODIFIER: {
      const underlyingType: TypeIndex = buf.readU32();
      const modFlags = buf.readU16();
      return {
        kind: "Modifier",
        underlyingType,
        flags: {
          isConst: (modFlags & 0x01) !== 0,
          isVolatile: (modFlags & 0x02) !== 0,
          isUnaligned: (modFlags & 0x04) !== 0,
        },
      };
    }

    case C.LF_ARRAY:
    case C.LF_ARRAY_ST:
    case C.LF_STRIDED_ARRAY: {
      const elementType: TypeIndex = buf.readU32();
      const indexingType: TypeIndex = buf.readU32();
      const stride: number | null = kind === C.LF_STRIDED_ARRAY ? buf.readU32() : null;
      const sizeVariant = buf.readVariant();
      const name = readTypeName(buf, kind);
      return {
        kind: "Array",
        elementType,
        indexingType,
        stride,
        dimensions: [sizeVariant.value],
        name,
      };
    }

    case C.LF_ALIAS:
    case C.LF_ALIAS_ST: {
      const underlyingType: TypeIndex = buf.readU32();
      const name = readTypeName(buf, kind);
      return { kind: "Alias", underlyingType, name };
    }

    case C.LF_BITFIELD: {
      const underlyingType: TypeIndex = buf.readU32();
      const length = buf.readU8();
      const position = buf.readU8();
      return { kind: "Bitfield", underlyingType, length, position };
    }

    case C.LF_FIELDLIST: {
      const fields: TypeData[] = [];
      let continuation: TypeIndex | null = null;

      while (!buf.isEmpty) {
        // Skip padding bytes
        while (!buf.isEmpty && buf.peekU8() >= 0xf0) {
          buf.readU8();
        }
        if (buf.isEmpty) break;

        const fieldKind = buf.readU16();
        if (fieldKind === C.LF_INDEX) {
          // Continuation
          buf.readU16(); // padding
          continuation = buf.readU32();
          break;
        }

        const field = parseFieldRecord(buf, fieldKind);
        if (field) fields.push(field);
      }

      return { kind: "FieldList", fields, continuation };
    }

    case C.LF_ARGLIST: {
      const count = buf.readU32();
      const args: TypeIndex[] = [];
      for (let i = 0; i < count; i++) {
        args.push(buf.readU32());
      }
      return { kind: "ArgumentList", arguments: args };
    }

    case C.LF_METHODLIST: {
      const methods: MethodListEntry[] = [];
      while (!buf.isEmpty) {
        const attrRaw = buf.readU16();
        buf.readU16(); // padding
        const attributes = parseFieldAttributes(attrRaw);
        const methodType: TypeIndex = buf.readU32();
        let vtableOffset: number | null = null;
        if (
          attributes.mprop === MethodKind.IntroducingVirtual ||
          attributes.mprop === MethodKind.PureIntroducingVirtual
        ) {
          vtableOffset = buf.readU32();
        }
        methods.push({ attributes, methodType, vtableOffset });
      }
      return { kind: "MethodList", methods };
    }

    default:
      throw PdbError.unimplementedTypeKind(kind);
  }
}

/** Parse a field list member record. */
function parseFieldRecord(buf: ParseBuffer, kind: number): TypeData | null {
  switch (kind) {
    case C.LF_MEMBER:
    case C.LF_MEMBER_ST: {
      const attributes = parseFieldAttributes(buf.readU16());
      const fieldType: TypeIndex = buf.readU32();
      const offset = buf.readVariant();
      const name = readTypeName(buf, kind);
      return {
        kind: "Member",
        attributes,
        fieldType,
        offset: offset.value,
        name,
      };
    }

    case C.LF_STMEMBER:
    case C.LF_STMEMBER_ST: {
      const attributes = parseFieldAttributes(buf.readU16());
      const fieldType: TypeIndex = buf.readU32();
      const name = readTypeName(buf, kind);
      return { kind: "StaticMember", attributes, fieldType, name };
    }

    case C.LF_NESTTYPE:
    case C.LF_NESTTYPE_ST: {
      buf.readU16(); // padding
      const attributes = parseFieldAttributes(0);
      const nestedType: TypeIndex = buf.readU32();
      const name = readTypeName(buf, kind);
      return { kind: "NestedType", attributes, nestedType, name };
    }

    case C.LF_NESTTYPEEX: {
      const attributes = parseFieldAttributes(buf.readU16());
      const nestedType: TypeIndex = buf.readU32();
      const name = readTypeName(buf, kind);
      return { kind: "NestedType", attributes, nestedType, name };
    }

    case C.LF_BCLASS:
    case C.LF_BINTERFACE: {
      const attributes = parseFieldAttributes(buf.readU16());
      const baseType: TypeIndex = buf.readU32();
      const offset = buf.readVariant();
      return { kind: "BaseClass", attributes, baseType, offset: offset.value };
    }

    case C.LF_VBCLASS:
    case C.LF_IVBCLASS: {
      const attributes = parseFieldAttributes(buf.readU16());
      const baseType: TypeIndex = buf.readU32();
      const basePointerType: TypeIndex = buf.readU32();
      const basePointerOffset = buf.readVariant();
      const virtualBaseOffset = buf.readVariant();
      return {
        kind: "VirtualBaseClass",
        isDirect: kind === C.LF_VBCLASS,
        attributes,
        baseType,
        basePointerType,
        basePointerOffset: basePointerOffset.value,
        virtualBaseOffset: virtualBaseOffset.value,
      };
    }

    case C.LF_VFUNCTAB: {
      buf.readU16(); // padding
      const tableType: TypeIndex = buf.readU32();
      return { kind: "VirtualFunctionTablePointer", tableType };
    }

    case C.LF_METHOD:
    case C.LF_METHOD_ST: {
      const count = buf.readU16();
      const methodList: TypeIndex = buf.readU32();
      const name = readTypeName(buf, kind);
      return { kind: "OverloadedMethod", count, methodList, name };
    }

    case C.LF_ONEMETHOD:
    case C.LF_ONEMETHOD_ST: {
      const attrRaw = buf.readU16();
      const attributes = parseFieldAttributes(attrRaw);
      const methodType: TypeIndex = buf.readU32();
      let vtableOffset: number | null = null;
      if (
        attributes.mprop === MethodKind.IntroducingVirtual ||
        attributes.mprop === MethodKind.PureIntroducingVirtual
      ) {
        vtableOffset = buf.readU32();
      }
      const name = readTypeName(buf, kind);
      return { kind: "Method", attributes, methodType, vtableOffset, name };
    }

    case C.LF_ENUMERATE: {
      const attributes = parseFieldAttributes(buf.readU16());
      const value = buf.readVariant();
      const name = buf.readCString();
      return { kind: "Enumerate", attributes, value, name };
    }

    case C.LF_VFUNCOFF: {
      buf.readU16(); // padding
      const _type: TypeIndex = buf.readU32();
      const _offset = buf.readU32();
      // Not commonly needed, skip
      return null;
    }

    default:
      // Skip unknown field records
      throw PdbError.unimplementedTypeKind(kind);
  }
}

/** Parse an ID record body. */
function parseIdRecord(buf: ParseBuffer, kind: number): IdData {
  switch (kind) {
    case C.LF_FUNC_ID: {
      const scopeId: IdIndex = buf.readU32();
      const functionType: TypeIndex = buf.readU32();
      const name = buf.readCString();
      return { kind: "FuncId", scopeId, functionType, name };
    }

    case C.LF_MFUNC_ID: {
      const parentType: TypeIndex = buf.readU32();
      const functionType: TypeIndex = buf.readU32();
      const name = buf.readCString();
      return { kind: "MemberFuncId", parentType, functionType, name };
    }

    case C.LF_BUILDINFO: {
      const count = buf.readU16();
      const args: IdIndex[] = [];
      for (let i = 0; i < count; i++) {
        args.push(buf.readU32());
      }
      return { kind: "BuildInfo", arguments: args };
    }

    case C.LF_STRING_ID: {
      const subStrings: IdIndex = buf.readU32();
      const name = buf.readCString();
      return { kind: "StringId", subStrings, name };
    }

    case C.LF_UDT_SRC_LINE: {
      const udtType: TypeIndex = buf.readU32();
      const sourceFile: IdIndex = buf.readU32();
      const line = buf.readU32();
      return { kind: "UdtSourceLine", udtType, sourceFile, line };
    }

    case C.LF_UDT_MOD_SRC_LINE: {
      const udtType: TypeIndex = buf.readU32();
      const sourceFile: IdIndex = buf.readU32();
      const line = buf.readU32();
      const module = buf.readU16();
      return { kind: "UdtModuleSourceLine", udtType, sourceFile, line, module };
    }

    default:
      throw PdbError.unimplementedTypeKind(kind);
  }
}

/** Read a type index, returning 0 as "null". */
function readTypeIndexOrNull(buf: ParseBuffer): TypeIndex | null {
  const index = buf.readU32();
  return index === 0 ? null : index;
}

/** Read a type name - CString for SZ types, Pascal for ST types. */
function readTypeName(buf: ParseBuffer, kind: number): string {
  if (kind < C.LF_ST_MAX && kind >= C.LF_TI16_MAX) {
    // ST format (pascal-style)
    return buf.readU8PascalString();
  }
  return buf.readCString();
}
