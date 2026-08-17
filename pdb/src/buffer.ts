import { PdbError } from "./error.js";
import { LF_NUMERIC, LF_CHAR, LF_SHORT, LF_LONG, LF_QUADWORD, LF_USHORT, LF_ULONG, LF_UQUADWORD } from "./tpi/constants.js";

/** Numeric variant value from PDB type records. */
export type Variant =
  | { kind: "u8"; value: number }
  | { kind: "u16"; value: number }
  | { kind: "u32"; value: number }
  | { kind: "u64"; value: bigint }
  | { kind: "i8"; value: number }
  | { kind: "i16"; value: number }
  | { kind: "i32"; value: number }
  | { kind: "i64"; value: bigint };

/** Get the numeric value of a Variant as a number (loses precision for 64-bit). */
export function variantValue(v: Variant): number | bigint {
  return v.value;
}

/**
 * A little-endian binary buffer reader for parsing PDB structures.
 * Operates on a Uint8Array without performing any I/O.
 */
export class ParseBuffer {
  private data: Uint8Array;
  private view: DataView;
  private offset: number;
  private end: number;

  constructor(data: Uint8Array, offset = 0, end?: number) {
    this.data = data;
    this.view = new DataView(data.buffer, data.byteOffset, data.byteLength);
    this.offset = offset;
    this.end = end ?? data.length;
  }

  /** Create a new ParseBuffer from a Uint8Array. */
  static from(data: Uint8Array): ParseBuffer {
    return new ParseBuffer(data);
  }

  /** Remaining bytes available to read. */
  get remaining(): number {
    return this.end - this.offset;
  }

  /** Whether all data has been consumed. */
  get isEmpty(): boolean {
    return this.remaining <= 0;
  }

  /** Current read position within the buffer. */
  get pos(): number {
    return this.offset;
  }

  /** Seek to an absolute position within the buffer. */
  seek(pos: number): void {
    this.offset = Math.min(pos, this.end);
  }

  /** Truncate the buffer to only allow reading up to `len` bytes from the start. */
  truncate(len: number): void {
    if (len > this.data.length) {
      throw PdbError.unexpectedEof();
    }
    this.end = len;
  }

  /** Align the current position to the next multiple of `alignment` bytes. */
  align(alignment: number): void {
    const diff = this.offset % alignment;
    if (diff > 0) {
      const needed = alignment - diff;
      if (this.remaining < needed) {
        throw PdbError.unexpectedEof();
      }
      this.offset += needed;
    }
  }

  /** Read a single unsigned byte. */
  readU8(): number {
    if (this.remaining < 1) throw PdbError.unexpectedEof();
    const value = this.view.getUint8(this.offset);
    this.offset += 1;
    return value;
  }

  /** Read a little-endian unsigned 16-bit integer. */
  readU16(): number {
    if (this.remaining < 2) throw PdbError.unexpectedEof();
    const value = this.view.getUint16(this.offset, true);
    this.offset += 2;
    return value;
  }

  /** Read a little-endian signed 16-bit integer. */
  readI16(): number {
    if (this.remaining < 2) throw PdbError.unexpectedEof();
    const value = this.view.getInt16(this.offset, true);
    this.offset += 2;
    return value;
  }

  /** Read a little-endian unsigned 32-bit integer. */
  readU32(): number {
    if (this.remaining < 4) throw PdbError.unexpectedEof();
    const value = this.view.getUint32(this.offset, true);
    this.offset += 4;
    return value;
  }

  /** Read a little-endian signed 32-bit integer. */
  readI32(): number {
    if (this.remaining < 4) throw PdbError.unexpectedEof();
    const value = this.view.getInt32(this.offset, true);
    this.offset += 4;
    return value;
  }

  /** Read a little-endian unsigned 64-bit integer as bigint. */
  readU64(): bigint {
    if (this.remaining < 8) throw PdbError.unexpectedEof();
    const value = this.view.getBigUint64(this.offset, true);
    this.offset += 8;
    return value;
  }

  /** Read a little-endian signed 64-bit integer as bigint. */
  readI64(): bigint {
    if (this.remaining < 8) throw PdbError.unexpectedEof();
    const value = this.view.getBigInt64(this.offset, true);
    this.offset += 8;
    return value;
  }

  /** Peek at the next unsigned byte without advancing. */
  peekU8(): number {
    if (this.remaining < 1) throw PdbError.unexpectedEof();
    return this.view.getUint8(this.offset);
  }

  /** Peek at the next little-endian unsigned 16-bit integer without advancing. */
  peekU16(): number {
    if (this.remaining < 2) throw PdbError.unexpectedEof();
    return this.view.getUint16(this.offset, true);
  }

  /** Peek at the next little-endian unsigned 32-bit integer without advancing. */
  peekU32(): number {
    if (this.remaining < 4) throw PdbError.unexpectedEof();
    return this.view.getUint32(this.offset, true);
  }

  /** Take `n` bytes from the buffer and advance the position. */
  take(n: number): Uint8Array {
    if (this.remaining < n) throw PdbError.unexpectedEof();
    const slice = this.data.subarray(this.offset, this.offset + n);
    this.offset += n;
    return slice;
  }

  /** Parse a NUL-terminated C string. Returns the string without the NUL byte. */
  readCString(): string {
    const start = this.offset;
    while (this.offset < this.end) {
      if (this.data[this.offset] === 0) {
        const bytes = this.data.subarray(start, this.offset);
        this.offset += 1; // skip the NUL
        return decodeUtf8Lossy(bytes);
      }
      this.offset++;
    }
    throw PdbError.unexpectedEof();
  }

  /** Parse a length-prefixed (u8 length) Pascal string. */
  readU8PascalString(): string {
    const length = this.readU8();
    const bytes = this.take(length);
    return decodeUtf8Lossy(bytes);
  }

  /** Parse a PDB variable-length numeric value (Variant). */
  readVariant(): Variant {
    const prefix = this.readU16();
    if (prefix < LF_NUMERIC) {
      return { kind: "u16", value: prefix };
    }
    switch (prefix) {
      case LF_CHAR:
        return { kind: "u8", value: this.readU8() };
      case LF_SHORT:
        return { kind: "i16", value: this.readI16() };
      case LF_LONG:
        return { kind: "i32", value: this.readI32() };
      case LF_QUADWORD:
        return { kind: "i64", value: this.readI64() };
      case LF_USHORT:
        return { kind: "u16", value: this.readU16() };
      case LF_ULONG:
        return { kind: "u32", value: this.readU32() };
      case LF_UQUADWORD:
        return { kind: "u64", value: this.readU64() };
      default:
        throw PdbError.unexpectedNumericPrefix(prefix);
    }
  }

  /** Create a sub-buffer from the current position for a given length. */
  slice(length: number): ParseBuffer {
    const slice = this.take(length);
    return new ParseBuffer(slice);
  }

  /** Clone this buffer (shares underlying data, separate position). */
  clone(): ParseBuffer {
    return new ParseBuffer(this.data, this.offset, this.end);
  }

  /** Get the underlying Uint8Array from current position. */
  get remainingBytes(): Uint8Array {
    return this.data.subarray(this.offset, this.end);
  }

  /** Get the full underlying buffer. */
  get fullBuffer(): Uint8Array {
    return this.data.subarray(0, this.end);
  }
}

const textDecoder = new TextDecoder("utf-8", { fatal: false });

/** Decode bytes as UTF-8, replacing invalid sequences. */
function decodeUtf8Lossy(bytes: Uint8Array): string {
  return textDecoder.decode(bytes);
}
