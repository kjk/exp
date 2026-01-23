/** Error codes for PDB parsing failures. */
export enum PdbErrorCode {
  UnrecognizedFileFormat = "UnrecognizedFileFormat",
  InvalidPageSize = "InvalidPageSize",
  PageReferenceOutOfRange = "PageReferenceOutOfRange",
  StreamNotFound = "StreamNotFound",
  StreamNameNotFound = "StreamNameNotFound",
  InvalidStreamLength = "InvalidStreamLength",
  UnexpectedEof = "UnexpectedEof",
  UnimplementedFeature = "UnimplementedFeature",
  GlobalSymbolsNotFound = "GlobalSymbolsNotFound",
  SymbolTooShort = "SymbolTooShort",
  UnimplementedSymbolKind = "UnimplementedSymbolKind",
  InvalidTypeInformationHeader = "InvalidTypeInformationHeader",
  TypeTooShort = "TypeTooShort",
  TypeNotFound = "TypeNotFound",
  TypeNotIndexed = "TypeNotIndexed",
  UnimplementedTypeKind = "UnimplementedTypeKind",
  UnexpectedNumericPrefix = "UnexpectedNumericPrefix",
  AddressMapNotFound = "AddressMapNotFound",
  UnimplementedDebugSubsection = "UnimplementedDebugSubsection",
  UnimplementedFileChecksumKind = "UnimplementedFileChecksumKind",
  InvalidFileChecksumOffset = "InvalidFileChecksumOffset",
  LinesNotFound = "LinesNotFound",
  InvalidCompressedAnnotation = "InvalidCompressedAnnotation",
  UnknownBinaryAnnotation = "UnknownBinaryAnnotation",
}

/** An error that occurred while reading or parsing the PDB. */
export class PdbError extends Error {
  constructor(
    public readonly code: PdbErrorCode,
    message?: string,
    public readonly detail?: number | string,
  ) {
    super(message ?? code);
    this.name = "PdbError";
  }

  static unrecognizedFileFormat(): PdbError {
    return new PdbError(
      PdbErrorCode.UnrecognizedFileFormat,
      "The input data was not recognized as a MSF (PDB) file",
    );
  }

  static invalidPageSize(size: number): PdbError {
    return new PdbError(
      PdbErrorCode.InvalidPageSize,
      `The MSF header specifies an invalid page size (${size} bytes)`,
      size,
    );
  }

  static pageReferenceOutOfRange(page: number): PdbError {
    return new PdbError(
      PdbErrorCode.PageReferenceOutOfRange,
      `MSF referred to page number (${page}) out of range`,
      page,
    );
  }

  static streamNotFound(stream: number): PdbError {
    return new PdbError(
      PdbErrorCode.StreamNotFound,
      `The requested stream (${stream}) is not stored in this file`,
      stream,
    );
  }

  static streamNameNotFound(): PdbError {
    return new PdbError(
      PdbErrorCode.StreamNameNotFound,
      "A stream requested by name was not found",
    );
  }

  static invalidStreamLength(context: string): PdbError {
    return new PdbError(
      PdbErrorCode.InvalidStreamLength,
      `${context} stream has an invalid length or alignment for its records`,
      context,
    );
  }

  static unexpectedEof(): PdbError {
    return new PdbError(
      PdbErrorCode.UnexpectedEof,
      "Unexpectedly reached end of input",
    );
  }

  static unimplementedFeature(feature: string): PdbError {
    return new PdbError(
      PdbErrorCode.UnimplementedFeature,
      `Unimplemented PDB feature: ${feature}`,
      feature,
    );
  }

  static globalSymbolsNotFound(): PdbError {
    return new PdbError(
      PdbErrorCode.GlobalSymbolsNotFound,
      "The global shared symbol table is missing",
    );
  }

  static symbolTooShort(): PdbError {
    return new PdbError(
      PdbErrorCode.SymbolTooShort,
      "A symbol record's length value was impossibly small",
    );
  }

  static unimplementedSymbolKind(kind: number): PdbError {
    return new PdbError(
      PdbErrorCode.UnimplementedSymbolKind,
      `Support for symbols of kind 0x${kind.toString(16).padStart(4, "0")} is not implemented`,
      kind,
    );
  }

  static invalidTypeInformationHeader(reason: string): PdbError {
    return new PdbError(
      PdbErrorCode.InvalidTypeInformationHeader,
      `The type information header was invalid: ${reason}`,
      reason,
    );
  }

  static typeTooShort(): PdbError {
    return new PdbError(
      PdbErrorCode.TypeTooShort,
      "A type record's length value was impossibly small",
    );
  }

  static typeNotFound(index: number): PdbError {
    return new PdbError(
      PdbErrorCode.TypeNotFound,
      `Type ${index} not found`,
      index,
    );
  }

  static typeNotIndexed(index: number, maxIndexed: number): PdbError {
    return new PdbError(
      PdbErrorCode.TypeNotIndexed,
      `Type ${index} not indexed (index covers ${maxIndexed})`,
      index,
    );
  }

  static unimplementedTypeKind(kind: number): PdbError {
    return new PdbError(
      PdbErrorCode.UnimplementedTypeKind,
      `Support for types of kind 0x${kind.toString(16).padStart(4, "0")} is not implemented`,
      kind,
    );
  }

  static unexpectedNumericPrefix(prefix: number): PdbError {
    return new PdbError(
      PdbErrorCode.UnexpectedNumericPrefix,
      `Variable-length numeric parsing encountered an unexpected prefix (0x${prefix.toString(16).padStart(4, "0")})`,
      prefix,
    );
  }

  static addressMapNotFound(): PdbError {
    return new PdbError(
      PdbErrorCode.AddressMapNotFound,
      "Required mapping for virtual addresses (OMAP) was not found",
    );
  }

  static unimplementedDebugSubsection(kind: number): PdbError {
    return new PdbError(
      PdbErrorCode.UnimplementedDebugSubsection,
      `Debug module subsection of kind 0x${kind.toString(16).padStart(4, "0")} is not implemented`,
      kind,
    );
  }

  static linesNotFound(): PdbError {
    return new PdbError(
      PdbErrorCode.LinesNotFound,
      "The lines table is missing",
    );
  }

  static invalidCompressedAnnotation(): PdbError {
    return new PdbError(
      PdbErrorCode.InvalidCompressedAnnotation,
      "A binary annotation was compressed incorrectly",
    );
  }
}
