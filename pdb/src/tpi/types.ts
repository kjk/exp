import type { TypeIndex, IdIndex } from "../common.js";
import type { Variant } from "../buffer.js";

/** Properties bitfield for class/struct/union/enum types. */
export interface TypeProperties {
  packed: boolean;
  hasConstructor: boolean;
  hasOverloadedOperator: boolean;
  isNested: boolean;
  hasNestedTypes: boolean;
  hasOverloadedAssignment: boolean;
  hasCastingMethods: boolean;
  isForwardReference: boolean;
  isScopedDefinition: boolean;
  hasUniqueName: boolean;
  isSealed: boolean;
  isIntrinsic: boolean;
}

/** Parse a u16 properties bitfield. */
export function parseTypeProperties(raw: number): TypeProperties {
  return {
    packed: (raw & 0x0001) !== 0,
    hasConstructor: (raw & 0x0002) !== 0,
    hasOverloadedOperator: (raw & 0x0004) !== 0,
    isNested: (raw & 0x0008) !== 0,
    hasNestedTypes: (raw & 0x0010) !== 0,
    hasOverloadedAssignment: (raw & 0x0020) !== 0,
    hasCastingMethods: (raw & 0x0040) !== 0,
    isForwardReference: (raw & 0x0080) !== 0,
    isScopedDefinition: (raw & 0x0100) !== 0,
    hasUniqueName: (raw & 0x0200) !== 0,
    isSealed: (raw & 0x0400) !== 0,
    isIntrinsic: (raw & 0x2000) !== 0,
  };
}

/** Field attributes for class members. */
export interface FieldAttributes {
  access: MemberAccess;
  mprop: MethodKind;
  pseudo: boolean;
  noinherit: boolean;
  noconstruct: boolean;
  compgenx: boolean;
  isSealed: boolean;
}

export enum MemberAccess {
  None = 0,
  Private = 1,
  Protected = 2,
  Public = 3,
}

export enum MethodKind {
  Vanilla = 0,
  Virtual = 1,
  Static = 2,
  Friend = 3,
  IntroducingVirtual = 4,
  PureVirtual = 5,
  PureIntroducingVirtual = 6,
}

/** Parse a u16 field attributes value. */
export function parseFieldAttributes(raw: number): FieldAttributes {
  return {
    access: (raw & 0x03) as MemberAccess,
    mprop: ((raw >> 2) & 0x07) as MethodKind,
    pseudo: (raw & 0x0020) !== 0,
    noinherit: (raw & 0x0040) !== 0,
    noconstruct: (raw & 0x0080) !== 0,
    compgenx: (raw & 0x0100) !== 0,
    isSealed: (raw & 0x0200) !== 0,
  };
}

/** Returns true if the method is static. */
export function isStaticMethod(attr: FieldAttributes): boolean {
  return attr.mprop === MethodKind.Static;
}

/** Returns true if the method is virtual. */
export function isVirtualMethod(attr: FieldAttributes): boolean {
  return attr.mprop === MethodKind.Virtual;
}

/** Returns true if the method is pure virtual (includes pure introducing virtual). */
export function isPureVirtualMethod(attr: FieldAttributes): boolean {
  return attr.mprop === MethodKind.PureVirtual ||
    attr.mprop === MethodKind.PureIntroducingVirtual;
}

/** Returns true if the method is an introducing virtual (includes pure introducing virtual). */
export function isIntroVirtualMethod(attr: FieldAttributes): boolean {
  return attr.mprop === MethodKind.IntroducingVirtual ||
    attr.mprop === MethodKind.PureIntroducingVirtual;
}

/** The kind of a PointerType. */
export enum PointerKind {
  Near16 = 0x00,
  Far16 = 0x01,
  Huge16 = 0x02,
  BaseSeg = 0x03,
  BaseVal = 0x04,
  BaseSegVal = 0x05,
  BaseAddr = 0x06,
  BaseSegAddr = 0x07,
  BaseType = 0x08,
  BaseSelf = 0x09,
  Near32 = 0x0a,
  Far32 = 0x0b,
  Ptr64 = 0x0c,
}

/** The mode of a PointerType. */
export enum PointerMode {
  Pointer = 0x00,
  LValueReference = 0x01,
  Member = 0x02,
  MemberFunction = 0x03,
  RValueReference = 0x04,
}

/** Pointer attributes. */
export interface PointerAttributes {
  pointerKind: PointerKind;
  pointerMode: PointerMode;
  isFlat32: boolean;
  isVolatile: boolean;
  isConst: boolean;
  isUnaligned: boolean;
  isRestrict: boolean;
  size: number;
  isMocom: boolean;
  isLValueRef: boolean;
  isRValueRef: boolean;
}

/** Parse pointer attributes from a u32. */
export function parsePointerAttributes(raw: number): PointerAttributes {
  const pointerKind = (raw & 0x1f) as PointerKind;
  const pointerMode = ((raw >> 5) & 0x07) as PointerMode;
  let size = (raw >> 13) & 0x3f;
  if (size === 0) {
    if (pointerKind === PointerKind.Near32 || pointerKind === PointerKind.Far32) {
      size = 4;
    } else if (pointerKind === PointerKind.Ptr64) {
      size = 8;
    }
  }
  return {
    pointerKind,
    pointerMode,
    isFlat32: (raw & 0x0100) !== 0,
    isVolatile: (raw & 0x0200) !== 0,
    isConst: (raw & 0x0400) !== 0,
    isUnaligned: (raw & 0x0800) !== 0,
    isRestrict: (raw & 0x1000) !== 0,
    size,
    isMocom: (raw & 0x00080000) !== 0,
    isLValueRef: (raw & 0x00100000) !== 0,
    isRValueRef: (raw & 0x00200000) !== 0,
  };
}

/** Modifier flags. */
export interface ModifierFlags {
  isConst: boolean;
  isVolatile: boolean;
  isUnaligned: boolean;
}

/** Function attributes (CV_funcattr_t combined with calling convention). */
export interface FunctionAttributes {
  callingConvention: CallingConvention;
  cxxReturnUdt: boolean;
  isConstructor: boolean;
  isConstructorWithVirtualBases: boolean;
}

/** Parse function attributes from two consecutive u8 values (calling convention + funcattr). */
export function parseFunctionAttributes(callConv: number, funcAttr: number): FunctionAttributes {
  return {
    callingConvention: callConv as CallingConvention,
    cxxReturnUdt: (funcAttr & 0x01) !== 0,
    isConstructor: (funcAttr & 0x02) !== 0,
    isConstructorWithVirtualBases: (funcAttr & 0x04) !== 0,
  };
}

/** Calling convention for procedures. */
export enum CallingConvention {
  NearC = 0x00,
  FarC = 0x01,
  NearPascal = 0x02,
  FarPascal = 0x03,
  NearFast = 0x04,
  FarFast = 0x05,
  NearStd = 0x06,
  FarStd = 0x07,
  NearSys = 0x08,
  FarSys = 0x09,
  ThisCall = 0x0a,
  MipsCall = 0x0b,
  Generic = 0x0c,
  AlphaCall = 0x0d,
  PpcCall = 0x0e,
  SHCall = 0x0f,
  ArmCall = 0x10,
  AM33Call = 0x11,
  TriCall = 0x12,
  SH5Call = 0x13,
  M32RCall = 0x14,
  ClrCall = 0x15,
  NearVector = 0x18,
}

// ─── Type Data Variants ───

export interface ClassType {
  kind: "Class" | "Structure" | "Interface";
  count: number;
  properties: TypeProperties;
  fields: TypeIndex | null;
  derivedFrom: TypeIndex | null;
  vtableShape: TypeIndex | null;
  size: number | bigint;
  name: string;
  uniqueName: string | null;
}

export interface UnionType {
  kind: "Union";
  count: number;
  properties: TypeProperties;
  fields: TypeIndex | null;
  size: number | bigint;
  name: string;
  uniqueName: string | null;
}

export interface EnumerationType {
  kind: "Enumeration";
  count: number;
  properties: TypeProperties;
  underlyingType: TypeIndex;
  fields: TypeIndex | null;
  name: string;
  uniqueName: string | null;
}

export interface ProcedureType {
  kind: "Procedure";
  returnType: TypeIndex | null;
  attributes: FunctionAttributes;
  parameterCount: number;
  argumentList: TypeIndex;
}

export interface MemberFunctionType {
  kind: "MemberFunction";
  returnType: TypeIndex;
  classType: TypeIndex;
  thisPointerType: TypeIndex | null;
  attributes: FunctionAttributes;
  parameterCount: number;
  argumentList: TypeIndex;
  thisAdjustment: number;
}

export interface PointerType {
  kind: "Pointer";
  underlyingType: TypeIndex;
  attributes: PointerAttributes;
  containingClass: TypeIndex | null;
}

export interface ModifierType {
  kind: "Modifier";
  underlyingType: TypeIndex;
  flags: ModifierFlags;
}

export interface ArrayType {
  kind: "Array";
  elementType: TypeIndex;
  indexingType: TypeIndex;
  stride: number | null;
  dimensions: (number | bigint)[];
  name: string;
}

export interface AliasType {
  kind: "Alias";
  underlyingType: TypeIndex;
  name: string;
}

export interface BitfieldType {
  kind: "Bitfield";
  underlyingType: TypeIndex;
  length: number;
  position: number;
}

export interface FieldListType {
  kind: "FieldList";
  fields: TypeData[];
  continuation: TypeIndex | null;
}

export interface ArgumentListType {
  kind: "ArgumentList";
  arguments: TypeIndex[];
}

export interface MethodListEntry {
  attributes: FieldAttributes;
  methodType: TypeIndex;
  vtableOffset: number | null;
}

export interface MethodListType {
  kind: "MethodList";
  methods: MethodListEntry[];
}

// ─── Field List Members ───

export interface MemberType {
  kind: "Member";
  attributes: FieldAttributes;
  fieldType: TypeIndex;
  offset: number | bigint;
  name: string;
}

export interface StaticMemberType {
  kind: "StaticMember";
  attributes: FieldAttributes;
  fieldType: TypeIndex;
  name: string;
}

export interface NestedType {
  kind: "NestedType";
  attributes: FieldAttributes;
  nestedType: TypeIndex;
  name: string;
}

export interface BaseClassType {
  kind: "BaseClass";
  attributes: FieldAttributes;
  baseType: TypeIndex;
  offset: number | bigint;
}

export interface VirtualBaseClassType {
  kind: "VirtualBaseClass";
  isDirect: boolean;
  attributes: FieldAttributes;
  baseType: TypeIndex;
  basePointerType: TypeIndex;
  basePointerOffset: number | bigint;
  virtualBaseOffset: number | bigint;
}

export interface VirtualFunctionTablePointerType {
  kind: "VirtualFunctionTablePointer";
  tableType: TypeIndex;
}

export interface OverloadedMethodType {
  kind: "OverloadedMethod";
  count: number;
  methodList: TypeIndex;
  name: string;
}

export interface MethodType {
  kind: "Method";
  attributes: FieldAttributes;
  methodType: TypeIndex;
  vtableOffset: number | null;
  name: string;
}

export interface EnumerateType {
  kind: "Enumerate";
  attributes: FieldAttributes;
  value: Variant;
  name: string;
}

export interface VirtualFunctionTableType {
  kind: "VirtualFunctionTable";
  completeClass: TypeIndex;
  overriddenVft: TypeIndex;
  vftEntries: number;
  names: string[];
}

// ─── Union Type ───

export type TypeData =
  | ClassType
  | UnionType
  | EnumerationType
  | ProcedureType
  | MemberFunctionType
  | PointerType
  | ModifierType
  | ArrayType
  | AliasType
  | BitfieldType
  | FieldListType
  | ArgumentListType
  | MethodListType
  | MemberType
  | StaticMemberType
  | NestedType
  | BaseClassType
  | VirtualBaseClassType
  | VirtualFunctionTablePointerType
  | OverloadedMethodType
  | MethodType
  | EnumerateType
  | VirtualFunctionTableType;

// ─── Id Data Types ───

export interface FuncId {
  kind: "FuncId";
  scopeId: IdIndex;
  functionType: TypeIndex;
  name: string;
}

export interface MemberFuncId {
  kind: "MemberFuncId";
  parentType: TypeIndex;
  functionType: TypeIndex;
  name: string;
}

export interface BuildInfo {
  kind: "BuildInfo";
  arguments: IdIndex[];
}

export interface StringId {
  kind: "StringId";
  subStrings: IdIndex;
  name: string;
}

export interface UdtSourceLine {
  kind: "UdtSourceLine";
  udtType: TypeIndex;
  sourceFile: IdIndex;
  line: number;
}

export interface UdtModuleSourceLine {
  kind: "UdtModuleSourceLine";
  udtType: TypeIndex;
  sourceFile: IdIndex;
  line: number;
  module: number;
}

export type IdData =
  | FuncId
  | MemberFuncId
  | BuildInfo
  | StringId
  | UdtSourceLine
  | UdtModuleSourceLine;
