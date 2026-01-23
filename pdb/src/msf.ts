import { ParseBuffer } from "./buffer.js";
import { PdbError } from "./error.js";

/** MSF 7.00 magic signature bytes. */
const BIG_MSF_MAGIC = new Uint8Array([
  0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x43, 0x2f,
  0x43, 0x2b, 0x2b, 0x20, 0x4d, 0x53, 0x46, 0x20, 0x37, 0x2e, 0x30, 0x30,
  0x0d, 0x0a, 0x1a, 0x44, 0x53, 0x00, 0x00, 0x00,
]);

/** MSF 2.00 (small) magic signature bytes (not supported). */
const SMALL_MSF_MAGIC_PREFIX = new Uint8Array([
  0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x43, 0x2f,
  0x43, 0x2b, 0x2b, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d,
]);

/** A stream of bytes from the MSF container. */
export interface MsfStream {
  data: Uint8Array;
}

/**
 * Multi-Stream File (MSF) container - the underlying format of PDB files.
 *
 * Parses the MSF header and stream table to provide access to individual streams.
 * Operates entirely on a Uint8Array buffer with no I/O.
 */
export class Msf {
  private pageSize: number;
  private pagesUsed: number;
  private streamSizes: number[];
  private streamPages: number[][];

  private constructor(
    private readonly buffer: Uint8Array,
    pageSize: number,
    pagesUsed: number,
    streamSizes: number[],
    streamPages: number[][],
  ) {
    this.pageSize = pageSize;
    this.pagesUsed = pagesUsed;
    this.streamSizes = streamSizes;
    this.streamPages = streamPages;
  }

  /** Parse an MSF container from a Uint8Array. */
  static open(data: Uint8Array): Msf {
    if (data.length < 4096) {
      // Check for small MSF
      if (headerMatches(data, SMALL_MSF_MAGIC_PREFIX)) {
        throw PdbError.unimplementedFeature("small MSF file format");
      }
      throw PdbError.unrecognizedFileFormat();
    }

    // Check for Big MSF magic
    if (!headerMatches(data, BIG_MSF_MAGIC)) {
      if (headerMatches(data, SMALL_MSF_MAGIC_PREFIX)) {
        throw PdbError.unimplementedFeature("small MSF file format");
      }
      throw PdbError.unrecognizedFileFormat();
    }

    // Parse the header
    const buf = new ParseBuffer(data);
    buf.take(32); // skip magic

    const pageSize = buf.readU32();
    const _freePageMap = buf.readU32();
    const pagesUsed = buf.readU32();
    const directorySize = buf.readU32();
    const _reserved = buf.readU32();

    // Validate page size: must be power of 2, >= 256, <= 128*64K
    if (
      popcount32(pageSize) !== 1 ||
      pageSize < 0x100 ||
      pageSize > 128 * 0x10000
    ) {
      throw PdbError.invalidPageSize(pageSize);
    }

    const pagesNeededForDirectory = Math.ceil(directorySize / pageSize);
    const pagesNeededForDirectoryPageList = Math.ceil(
      (pagesNeededForDirectory * 4) / pageSize,
    );

    // Read the page numbers that point to the directory page list
    const directoryPageListPages: number[] = [];
    for (let i = 0; i < pagesNeededForDirectoryPageList; i++) {
      const pageNum = buf.readU32();
      validatePageNumber(pageNum, pagesUsed);
      directoryPageListPages.push(pageNum);
    }

    // Read the directory page list (list of pages that contain the stream table)
    const directoryPageListData = readPages(
      data,
      pageSize,
      directoryPageListPages,
      pagesNeededForDirectory * 4,
    );
    const directoryPageListBuf = new ParseBuffer(directoryPageListData);
    const directoryPages: number[] = [];
    for (let i = 0; i < pagesNeededForDirectory; i++) {
      const pageNum = directoryPageListBuf.readU32();
      validatePageNumber(pageNum, pagesUsed);
      directoryPages.push(pageNum);
    }

    // Read the actual stream table (directory)
    const streamTableData = readPages(
      data,
      pageSize,
      directoryPages,
      directorySize,
    );
    const streamTableBuf = new ParseBuffer(streamTableData);

    const streamCount = streamTableBuf.readU32();

    // Read stream sizes
    const streamSizes: number[] = [];
    for (let i = 0; i < streamCount; i++) {
      streamSizes.push(streamTableBuf.readU32());
    }

    // Read stream page numbers
    const streamPages: number[][] = [];
    for (let i = 0; i < streamCount; i++) {
      const size = streamSizes[i];
      if (size === 0xffffffff) {
        // Stream doesn't exist
        streamPages.push([]);
        continue;
      }
      const numPages = Math.ceil(size / pageSize);
      const pages: number[] = [];
      for (let j = 0; j < numPages; j++) {
        const pageNum = streamTableBuf.readU32();
        validatePageNumber(pageNum, pagesUsed);
        pages.push(pageNum);
      }
      streamPages.push(pages);
    }

    return new Msf(data, pageSize, pagesUsed, streamSizes, streamPages);
  }

  /** Get the number of streams. */
  get streamCount(): number {
    return this.streamSizes.length;
  }

  /** Get a stream by its index. */
  getStream(streamNumber: number, limit?: number): MsfStream {
    if (streamNumber >= this.streamSizes.length) {
      throw PdbError.streamNotFound(streamNumber);
    }

    const size = this.streamSizes[streamNumber];
    if (size === 0xffffffff) {
      throw PdbError.streamNotFound(streamNumber);
    }

    const actualSize = limit !== undefined ? Math.min(size, limit) : size;
    const pages = this.streamPages[streamNumber];
    const streamData = readPages(this.buffer, this.pageSize, pages, actualSize);

    return { data: streamData };
  }

  /** Check if a stream exists. */
  hasStream(streamNumber: number): boolean {
    if (streamNumber >= this.streamSizes.length) return false;
    return this.streamSizes[streamNumber] !== 0xffffffff;
  }
}

/**
 * Read pages from the raw PDB data and concatenate them, truncating to the
 * specified byte length.
 */
function readPages(
  data: Uint8Array,
  pageSize: number,
  pages: number[],
  totalBytes: number,
): Uint8Array {
  if (totalBytes === 0) return new Uint8Array(0);

  const result = new Uint8Array(totalBytes);
  let outputOffset = 0;

  for (const page of pages) {
    const sourceOffset = page * pageSize;
    const remaining = totalBytes - outputOffset;
    if (remaining <= 0) break;
    const bytesToCopy = Math.min(pageSize, remaining);
    result.set(data.subarray(sourceOffset, sourceOffset + bytesToCopy), outputOffset);
    outputOffset += bytesToCopy;
  }

  return result;
}

/** Validate a page number is within range. */
function validatePageNumber(pageNumber: number, pagesUsed: number): void {
  if (pageNumber === 0 || pageNumber > pagesUsed) {
    throw PdbError.pageReferenceOutOfRange(pageNumber);
  }
}

/** Check if the data starts with the expected magic bytes. */
function headerMatches(data: Uint8Array, expected: Uint8Array): boolean {
  if (data.length < expected.length) return false;
  for (let i = 0; i < expected.length; i++) {
    if (data[i] !== expected[i]) return false;
  }
  return true;
}

/** Count the number of set bits in a 32-bit integer. */
function popcount32(n: number): number {
  n = n - ((n >> 1) & 0x55555555);
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
  return (((n + (n >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
}
