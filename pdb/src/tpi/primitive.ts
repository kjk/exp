import type { TypeIndex } from "../common.js";
import { PdbError } from "../error.js";

// References for primitive types:
//
// cvinfo.h provides an enumeration:
//   https://github.com/Microsoft/microsoft-pdb/blob/082c5290e5aff028ae84e43affa8be717aa7af73/include/cvinfo.h#L328-L750
//
// pdbparse.cpp describes them as strings:
//   https://github.com/Microsoft/microsoft-pdb/blob/082c5290e5aff028ae84e43affa8be717aa7af73/pdbdump/pdbdump.cpp#L1896-L1974

// ─── PrimitiveKind constants ───

/** Uncharacterized type (no type) */
export const PrimitiveKind_NoType = 0x00;
/** Void type */
export const PrimitiveKind_Void = 0x03;
/** Windows HRESULT error code */
export const PrimitiveKind_HRESULT = 0x08;
/** Character (byte) */
export const PrimitiveKind_Char = 0x10;
/** Unsigned character */
export const PrimitiveKind_UChar = 0x20;
/** Signed 8-bit integer */
export const PrimitiveKind_I8 = 0x68;
/** Unsigned 8-bit integer */
export const PrimitiveKind_U8 = 0x69;
/** "Really a char" */
export const PrimitiveKind_RChar = 0x70;
/** Wide characters (16 bits) */
export const PrimitiveKind_WChar = 0x71;
/** Signed 16-bit integer */
export const PrimitiveKind_I16 = 0x72;
/** Unsigned 16-bit integer */
export const PrimitiveKind_U16 = 0x73;
/** Signed 32-bit integer */
export const PrimitiveKind_I32 = 0x74;
/** Unsigned 32-bit integer */
export const PrimitiveKind_U32 = 0x75;
/** Signed 64-bit integer */
export const PrimitiveKind_I64 = 0x76;
/** Unsigned 64-bit integer */
export const PrimitiveKind_U64 = 0x77;
/** Signed 128-bit integer */
export const PrimitiveKind_I128 = 0x78;
/** Unsigned 128-bit integer */
export const PrimitiveKind_U128 = 0x79;
/** "Really a 16-bit char" (char16_t) */
export const PrimitiveKind_RChar16 = 0x7a;
/** "Really a 32-bit char" (char32_t) */
export const PrimitiveKind_RChar32 = 0x7b;
/** UTF-8 character (C++20 char8_t) */
export const PrimitiveKind_Char8 = 0x7c;
/** Signed 16-bit integer (short) */
export const PrimitiveKind_Short = 0x11;
/** Unsigned 16-bit integer (unsigned short) */
export const PrimitiveKind_UShort = 0x21;
/** Signed 32-bit integer (long) */
export const PrimitiveKind_Long = 0x12;
/** Unsigned 32-bit integer (unsigned long) */
export const PrimitiveKind_ULong = 0x22;
/** Signed 64-bit integer (quad) */
export const PrimitiveKind_Quad = 0x13;
/** Unsigned 64-bit integer (unsigned quad) */
export const PrimitiveKind_UQuad = 0x23;
/** Signed 128-bit integer (octa) */
export const PrimitiveKind_Octa = 0x14;
/** Unsigned 128-bit integer (unsigned octa) */
export const PrimitiveKind_UOcta = 0x24;
/** 32-bit floating point */
export const PrimitiveKind_F32 = 0x40;
/** 64-bit floating point */
export const PrimitiveKind_F64 = 0x41;
/** 80-bit floating point */
export const PrimitiveKind_F80 = 0x42;
/** 128-bit floating point */
export const PrimitiveKind_F128 = 0x43;
/** 48-bit floating point */
export const PrimitiveKind_F48 = 0x44;
/** 32-bit partial precision floating point */
export const PrimitiveKind_F32PP = 0x45;
/** 16-bit floating point */
export const PrimitiveKind_F16 = 0x46;
/** 32-bit complex number */
export const PrimitiveKind_Complex32 = 0x50;
/** 64-bit complex number */
export const PrimitiveKind_Complex64 = 0x51;
/** 80-bit complex number */
export const PrimitiveKind_Complex80 = 0x52;
/** 128-bit complex number */
export const PrimitiveKind_Complex128 = 0x53;
/** 8-bit boolean value */
export const PrimitiveKind_Bool8 = 0x30;
/** 16-bit boolean value */
export const PrimitiveKind_Bool16 = 0x31;
/** 32-bit boolean value */
export const PrimitiveKind_Bool32 = 0x32;
/** 64-bit boolean value */
export const PrimitiveKind_Bool64 = 0x33;

export type PrimitiveKind = number;

// ─── Indirection constants ───

/** 16-bit ("near") pointer */
export const Indirection_Near16 = 0x100;
/** 16:16 far pointer */
export const Indirection_Far16 = 0x200;
/** 16:16 huge pointer */
export const Indirection_Huge16 = 0x300;
/** 32-bit pointer */
export const Indirection_Near32 = 0x400;
/** 48-bit 16:32 pointer */
export const Indirection_Far32 = 0x500;
/** 64-bit near pointer */
export const Indirection_Near64 = 0x600;
/** 128-bit near pointer */
export const Indirection_Near128 = 0x700;

export type Indirection = number;

// ─── PrimitiveType ───

/** Represents a primitive type like `void` or `char *`. */
export interface PrimitiveType {
  kind: PrimitiveKind;
  indirection: Indirection | null;
}

/** Parse a TypeIndex into a PrimitiveType. Index must be < 0x1000. */
export function parsePrimitiveType(index: TypeIndex): PrimitiveType {
  const indirectionBits = index & 0xf00;
  let indirection: Indirection | null;
  switch (indirectionBits) {
    case 0x000: indirection = null; break;
    case 0x100: indirection = Indirection_Near16; break;
    case 0x200: indirection = Indirection_Far16; break;
    case 0x300: indirection = Indirection_Huge16; break;
    case 0x400: indirection = Indirection_Near32; break;
    case 0x500: indirection = Indirection_Far32; break;
    case 0x600: indirection = Indirection_Near64; break;
    case 0x700: indirection = Indirection_Near128; break;
    default: throw PdbError.typeNotFound(index);
  }

  const kind = index & 0xff;
  switch (kind) {
    case PrimitiveKind_NoType:
    case PrimitiveKind_Void:
    case PrimitiveKind_HRESULT:
    case PrimitiveKind_Char:
    case PrimitiveKind_UChar:
    case PrimitiveKind_I8:
    case PrimitiveKind_U8:
    case PrimitiveKind_RChar:
    case PrimitiveKind_WChar:
    case PrimitiveKind_RChar16:
    case PrimitiveKind_RChar32:
    case PrimitiveKind_Char8:
    case PrimitiveKind_Short:
    case PrimitiveKind_UShort:
    case PrimitiveKind_I16:
    case PrimitiveKind_U16:
    case PrimitiveKind_Long:
    case PrimitiveKind_ULong:
    case PrimitiveKind_I32:
    case PrimitiveKind_U32:
    case PrimitiveKind_Quad:
    case PrimitiveKind_UQuad:
    case PrimitiveKind_I64:
    case PrimitiveKind_U64:
    case PrimitiveKind_Octa:
    case PrimitiveKind_UOcta:
    case PrimitiveKind_I128:
    case PrimitiveKind_U128:
    case PrimitiveKind_F16:
    case PrimitiveKind_F32:
    case PrimitiveKind_F32PP:
    case PrimitiveKind_F48:
    case PrimitiveKind_F64:
    case PrimitiveKind_F80:
    case PrimitiveKind_F128:
    case PrimitiveKind_Complex32:
    case PrimitiveKind_Complex64:
    case PrimitiveKind_Complex80:
    case PrimitiveKind_Complex128:
    case PrimitiveKind_Bool8:
    case PrimitiveKind_Bool16:
    case PrimitiveKind_Bool32:
    case PrimitiveKind_Bool64:
      break;
    default:
      throw PdbError.typeNotFound(index);
  }

  return { kind, indirection };
}

// ─── primitiveTypeName ───

/** Get the C/C++ type name for a primitive type index. */
export function primitiveTypeName(index: TypeIndex): string {
  const baseType = index & 0xff;
  const indirection = (index >> 8) & 0x0f;

  let name: string;
  switch (baseType) {
    case PrimitiveKind_NoType: name = "..."; break;
    case PrimitiveKind_Void: name = "void"; break;
    case PrimitiveKind_HRESULT: name = "HRESULT"; break;
    case PrimitiveKind_Char: name = "char"; break;
    case PrimitiveKind_UChar: name = "unsigned char"; break;
    case PrimitiveKind_I8: name = "int8_t"; break;
    case PrimitiveKind_U8: name = "uint8_t"; break;
    case PrimitiveKind_RChar: name = "char"; break;
    case PrimitiveKind_WChar: name = "wchar_t"; break;
    case PrimitiveKind_RChar16: name = "char16_t"; break;
    case PrimitiveKind_RChar32: name = "char32_t"; break;
    case PrimitiveKind_Char8: name = "char8_t"; break;
    case PrimitiveKind_Short: name = "short"; break;
    case PrimitiveKind_UShort: name = "unsigned short"; break;
    case PrimitiveKind_I16: name = "int16_t"; break;
    case PrimitiveKind_U16: name = "uint16_t"; break;
    case PrimitiveKind_Long: name = "long"; break;
    case PrimitiveKind_ULong: name = "unsigned long"; break;
    case PrimitiveKind_I32: name = "int32_t"; break;
    case PrimitiveKind_U32: name = "uint32_t"; break;
    case PrimitiveKind_Quad: name = "int64_t"; break;
    case PrimitiveKind_UQuad: name = "uint64_t"; break;
    case PrimitiveKind_I64: name = "int64_t"; break;
    case PrimitiveKind_U64: name = "uint64_t"; break;
    case PrimitiveKind_Octa: name = "__int128"; break;
    case PrimitiveKind_UOcta: name = "unsigned __int128"; break;
    case PrimitiveKind_I128: name = "__int128"; break;
    case PrimitiveKind_U128: name = "unsigned __int128"; break;
    case PrimitiveKind_F16: name = "_Float16"; break;
    case PrimitiveKind_F32: name = "float"; break;
    case PrimitiveKind_F32PP: name = "float"; break;
    case PrimitiveKind_F48: name = "__float48"; break;
    case PrimitiveKind_F64: name = "double"; break;
    case PrimitiveKind_F80: name = "long double"; break;
    case PrimitiveKind_F128: name = "__float128"; break;
    case PrimitiveKind_Complex32: name = "_Complex float"; break;
    case PrimitiveKind_Complex64: name = "_Complex double"; break;
    case PrimitiveKind_Complex80: name = "_Complex long double"; break;
    case PrimitiveKind_Complex128: name = "_Complex __float128"; break;
    case PrimitiveKind_Bool8: name = "bool"; break;
    case PrimitiveKind_Bool16: name = "__bool16"; break;
    case PrimitiveKind_Bool32: name = "__bool32"; break;
    case PrimitiveKind_Bool64: name = "__bool64"; break;
    default: name = `primitive_0x${baseType.toString(16)}`; break;
  }

  if (indirection > 0) {
    name += " *";
  }

  return name;
}
