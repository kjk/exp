/** Relative Virtual Address as it appears in a PE file. */
export type Rva = number & { readonly __brand: "Rva" };

/** Creates an Rva value. */
export function rva(value: number): Rva {
  return value as Rva;
}

/** A Relative Virtual Address in the PDB internal (unoptimized) address space. */
export type PdbInternalRva = number & { readonly __brand: "PdbInternalRva" };

/** Creates a PdbInternalRva value. */
export function pdbInternalRva(value: number): PdbInternalRva {
  return value as PdbInternalRva;
}

/** An offset relative to a PE section. */
export interface SectionOffset {
  /** Memory offset relative to the start of the section. */
  offset: number;
  /** Section index (1-based, 0 = invalid). */
  section: number;
}

/** An offset relative to a PE section in the PDB internal address space. */
export interface PdbInternalSectionOffset {
  /** Memory offset relative to the start of the section. */
  offset: number;
  /** Section index (1-based, 0 = invalid). */
  section: number;
}

/** Index of a PDB stream. 0xffff means "no stream". */
export type StreamIndex = number;

/** The null stream index value. */
export const STREAM_INDEX_NONE = 0xffff;

/** Check whether a stream index refers to an actual stream. */
export function streamIndexIsValid(index: StreamIndex): boolean {
  return index !== STREAM_INDEX_NONE;
}

/** Index of TypeData in the TypeInformation stream. */
export type TypeIndex = number;

/** Index of an Id in the IdInformation stream. */
export type IdIndex = number;

/** A reference to a string in the string table. */
export type StringRef = number;

/** Index of a file entry in the module. */
export type FileIndex = number;

/** A reference into the symbol table of a module. */
export type SymbolIndex = number;

/** A register referred to by its number. */
export type Register = number;

/** The minimum type index for non-primitive types. */
export const MIN_TYPE_INDEX = 0x1000;

/** Check if an item index is a cross-module reference. */
export function isCrossModuleRef(index: number): boolean {
  return (index & 0x80000000) !== 0;
}

/** PE image section header. */
export interface ImageSectionHeader {
  name: string;
  virtualSize: number;
  virtualAddress: number;
  sizeOfRawData: number;
  pointerToRawData: number;
  pointerToRelocations: number;
  pointerToLinenumbers: number;
  numberOfRelocations: number;
  numberOfLinenumbers: number;
  characteristics: number;
}

/** The target machine architecture. */
export enum MachineType {
  Unknown = 0x0,
  Am33 = 0x13,
  Amd64 = 0x8664,
  Arm = 0x1c0,
  Arm64 = 0xaa64,
  ArmNT = 0x1c4,
  Ebc = 0xebc,
  X86 = 0x14c,
  Ia64 = 0x200,
  M32R = 0x9041,
  Mips16 = 0x266,
  MipsFpu = 0x366,
  MipsFpu16 = 0x466,
  PowerPC = 0x1f0,
  PowerPCFP = 0x1f1,
  R4000 = 0x166,
  RiscV32 = 0x5032,
  RiscV64 = 0x5064,
  RiscV128 = 0x5128,
  SH3 = 0x1a2,
  SH3DSP = 0x1a3,
  SH4 = 0x1a6,
  SH5 = 0x1a8,
  Thumb = 0x1c2,
  WceMipsV2 = 0x169,
  Invalid = 0xffff,
}

/** Convert a raw u16 to a MachineType. */
export function machineTypeFromU16(value: number): MachineType {
  const known: number[] = Object.values(MachineType).filter(
    (v) => typeof v === "number",
  ) as number[];
  if (known.includes(value)) {
    return value as MachineType;
  }
  return MachineType.Unknown;
}

/** GUID as stored in PDB files (mixed-endian format). */
export interface PdbGuid {
  data1: number; // u32 LE
  data2: number; // u16 LE
  data3: number; // u16 LE
  data4: Uint8Array; // 8 bytes, big-endian
}

/** Format a PDB GUID as a standard string (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx). */
export function formatGuid(guid: PdbGuid): string {
  const d1 = guid.data1.toString(16).padStart(8, "0");
  const d2 = guid.data2.toString(16).padStart(4, "0");
  const d3 = guid.data3.toString(16).padStart(4, "0");
  const d4a = Array.from(guid.data4.subarray(0, 2))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
  const d4b = Array.from(guid.data4.subarray(2))
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");
  return `${d1}-${d2}-${d3}-${d4a}-${d4b}`;
}

/** Section characteristics flags (bitfield). */
export type SectionCharacteristics = number;

/** PDB header version. */
export enum HeaderVersion {
  V41 = 930803,
  V50 = 19960307,
  V60 = 19970606,
  V70 = 19990903,
  V110 = 20091201,
}

/** Convert a raw u32 to a HeaderVersion. */
export function headerVersionFromU32(value: number): HeaderVersion | number {
  const known: number[] = Object.values(HeaderVersion).filter(
    (v) => typeof v === "number",
  ) as number[];
  if (known.includes(value)) {
    return value as HeaderVersion;
  }
  return value;
}
