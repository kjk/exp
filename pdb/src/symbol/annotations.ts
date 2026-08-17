import { ParseBuffer } from "../buffer.js";
import type { FileIndex } from "../common.js";

/** Parsed binary annotation used by S_INLINESITE to encode line program opcodes. */
export type BinaryAnnotation =
  | { kind: "CodeOffset"; offset: number }
  | { kind: "ChangeCodeOffsetBase"; offset: number }
  | { kind: "ChangeCodeOffset"; delta: number }
  | { kind: "ChangeCodeLength"; length: number }
  | { kind: "ChangeFile"; fileIndex: FileIndex }
  | { kind: "ChangeLineOffset"; delta: number }
  | { kind: "ChangeLineEndDelta"; value: number }
  | { kind: "ChangeRangeKind"; value: number }
  | { kind: "ChangeColumnStart"; column: number }
  | { kind: "ChangeColumnEndDelta"; delta: number }
  | { kind: "ChangeCodeOffsetAndLineOffset"; codeDelta: number; lineDelta: number }
  | { kind: "ChangeCodeLengthAndCodeOffset"; length: number; offset: number }
  | { kind: "ChangeColumnEnd"; column: number };

/** Returns true if this annotation emits a line info record. */
export function annotationEmitsLineInfo(ann: BinaryAnnotation): boolean {
  return ann.kind === "ChangeCodeOffset" ||
    ann.kind === "ChangeCodeOffsetAndLineOffset" ||
    ann.kind === "ChangeCodeLengthAndCodeOffset";
}

/** Decode a signed operand (CVUncompressSignedData). */
function decodeSignedOperand(value: number): number {
  if (value & 1) {
    return -((value >>> 1) | 0);
  }
  return (value >>> 1) | 0;
}

/** Iterator over binary annotations in an inline site. */
export class BinaryAnnotationsIter {
  private buf: ParseBuffer;

  constructor(data: Uint8Array) {
    this.buf = new ParseBuffer(data);
  }

  /** Parse a compact unsigned integer (CVUncompressData). */
  private uncompressNext(): number {
    const b1 = this.buf.readU8();
    if ((b1 & 0x80) === 0x00) {
      return b1;
    }
    const b2 = this.buf.readU8();
    if ((b1 & 0xc0) === 0x80) {
      return ((b1 & 0x3f) << 8) | b2;
    }
    const b3 = this.buf.readU8();
    const b4 = this.buf.readU8();
    if ((b1 & 0xe0) === 0xc0) {
      return ((b1 & 0x1f) << 24) | (b2 << 16) | (b3 << 8) | b4;
    }
    throw new Error("Invalid compressed annotation value");
  }

  /** Get the next annotation, or null if done. */
  next(): BinaryAnnotation | null {
    if (this.buf.isEmpty) {
      return null;
    }

    const op = this.uncompressNext();
    switch (op) {
      case 0: // Eof
        return null;
      case 1: // CodeOffset
        return { kind: "CodeOffset", offset: this.uncompressNext() };
      case 2: // ChangeCodeOffsetBase
        return { kind: "ChangeCodeOffsetBase", offset: this.uncompressNext() };
      case 3: // ChangeCodeOffset
        return { kind: "ChangeCodeOffset", delta: this.uncompressNext() };
      case 4: // ChangeCodeLength
        return { kind: "ChangeCodeLength", length: this.uncompressNext() };
      case 5: // ChangeFile
        return { kind: "ChangeFile", fileIndex: this.uncompressNext() };
      case 6: // ChangeLineOffset
        return { kind: "ChangeLineOffset", delta: decodeSignedOperand(this.uncompressNext()) };
      case 7: // ChangeLineEndDelta
        return { kind: "ChangeLineEndDelta", value: this.uncompressNext() };
      case 8: // ChangeRangeKind
        return { kind: "ChangeRangeKind", value: this.uncompressNext() };
      case 9: // ChangeColumnStart
        return { kind: "ChangeColumnStart", column: this.uncompressNext() };
      case 10: // ChangeColumnEndDelta
        return { kind: "ChangeColumnEndDelta", delta: decodeSignedOperand(this.uncompressNext()) };
      case 11: { // ChangeCodeOffsetAndLineOffset
        const operand = this.uncompressNext();
        return {
          kind: "ChangeCodeOffsetAndLineOffset",
          codeDelta: operand & 0xf,
          lineDelta: decodeSignedOperand(operand >>> 4),
        };
      }
      case 12: // ChangeCodeLengthAndCodeOffset
        return {
          kind: "ChangeCodeLengthAndCodeOffset",
          length: this.uncompressNext(),
          offset: this.uncompressNext(),
        };
      case 13: // ChangeColumnEnd
        return { kind: "ChangeColumnEnd", column: this.uncompressNext() };
      default:
        throw new Error(`Unknown binary annotation opcode: ${op}`);
    }
  }

  /** Collect all annotations into an array. */
  collect(): BinaryAnnotation[] {
    const result: BinaryAnnotation[] = [];
    let ann = this.next();
    while (ann !== null) {
      result.push(ann);
      ann = this.next();
    }
    return result;
  }
}

/** Iterate over binary annotations from raw annotation bytes. */
export function iterateBinaryAnnotations(data: Uint8Array): BinaryAnnotationsIter {
  return new BinaryAnnotationsIter(data);
}
