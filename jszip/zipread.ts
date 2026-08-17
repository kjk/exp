// zipread.ts - Minimal read-only ZIP parser for Bun, extracted from JSZip
// Supports STORE and DEFLATE compression, ZIP64, Unicode extra fields

// ── Signatures ──────────────────────────────────────────────────────────────

const SIG_LOCAL = [0x50, 0x4b, 0x03, 0x04];
const SIG_CENTRAL = [0x50, 0x4b, 0x01, 0x02];
const SIG_EOCD = [0x50, 0x4b, 0x05, 0x06];
const SIG_ZIP64_EOCD = [0x50, 0x4b, 0x06, 0x06];
const SIG_ZIP64_LOCATOR = [0x50, 0x4b, 0x06, 0x07];

const MAX_VALUE_16 = 0xffff;
const MAX_VALUE_32 = 0xffffffff;

// ── CRC32 ───────────────────────────────────────────────────────────────────

const crcTable: Uint32Array = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[n] = c;
  }
  return table;
})();

function crc32(buf: Uint8Array): number {
  let crc = 0xffffffff;
  for (let i = 0; i < buf.length; i++) {
    crc = (crc >>> 8) ^ crcTable[(crc ^ buf[i]) & 0xff];
  }
  return (crc ^ 0xffffffff) >>> 0;
}

// ── UTF-8 decode ────────────────────────────────────────────────────────────

const textDecoder = new TextDecoder("utf-8");

function utf8decode(buf: Uint8Array): string {
  return textDecoder.decode(buf);
}

// ── Path sanitization ───────────────────────────────────────────────────────

function resolvePath(path: string): string {
  const parts = path.split("/");
  const result: string[] = [];
  for (let i = 0; i < parts.length; i++) {
    const part = parts[i];
    if (part === "." || (part === "" && i !== 0 && i !== parts.length - 1)) {
      continue;
    } else if (part === "..") {
      result.pop();
    } else {
      result.push(part);
    }
  }
  return result.join("/");
}

// ── DataReader ──────────────────────────────────────────────────────────────

class DataReader {
  data: Uint8Array;
  index: number = 0;

  constructor(data: Uint8Array) {
    this.data = data;
  }

  get length(): number {
    return this.data.length;
  }

  checkOffset(size: number): void {
    if (this.index + size > this.data.length) {
      throw new Error(
        `End of data reached (want ${size} bytes at ${this.index}, have ${this.data.length})`
      );
    }
  }

  setIndex(i: number): void {
    if (i < 0 || i > this.data.length) {
      throw new Error(`Invalid index: ${i} (length: ${this.data.length})`);
    }
    this.index = i;
  }

  skip(n: number): void {
    this.setIndex(this.index + n);
  }

  readInt(size: number): number {
    this.checkOffset(size);
    let result = 0;
    for (let i = this.index + size - 1; i >= this.index; i--) {
      result = result * 256 + this.data[i]; // avoid << for 8-byte reads
    }
    this.index += size;
    return result;
  }

  readData(size: number): Uint8Array {
    this.checkOffset(size);
    if (size === 0) return new Uint8Array(0);
    const result = this.data.subarray(this.index, this.index + size);
    this.index += size;
    return result;
  }

  readDate(): Date {
    const dostime = this.readInt(4);
    return new Date(
      Date.UTC(
        ((dostime >> 25) & 0x7f) + 1980,
        ((dostime >> 21) & 0x0f) - 1,
        (dostime >> 16) & 0x1f,
        (dostime >> 11) & 0x1f,
        (dostime >> 5) & 0x3f,
        (dostime & 0x1f) << 1
      )
    );
  }

  lastIndexOfSignature(sig: number[]): number {
    const [s0, s1, s2, s3] = sig;
    for (let i = this.data.length - 4; i >= 0; i--) {
      if (
        this.data[i] === s0 &&
        this.data[i + 1] === s1 &&
        this.data[i + 2] === s2 &&
        this.data[i + 3] === s3
      ) {
        return i;
      }
    }
    return -1;
  }

  readAndCheckSignature(sig: number[]): boolean {
    this.checkOffset(4);
    const ok =
      this.data[this.index] === sig[0] &&
      this.data[this.index + 1] === sig[1] &&
      this.data[this.index + 2] === sig[2] &&
      this.data[this.index + 3] === sig[3];
    this.index += 4;
    return ok;
  }

  checkSignature(sig: number[]): void {
    if (!this.readAndCheckSignature(sig)) {
      throw new Error("Invalid ZIP signature");
    }
  }
}

// ── Types ───────────────────────────────────────────────────────────────────

export interface ZipEntry {
  name: string;
  unsafeName: string;
  isDir: boolean;
  date: Date;
  comment: string;
  unixPermissions: number | null;
  dosPermissions: number | null;
  compressedSize: number;
  uncompressedSize: number;
  crc32: number;
  compressedData: Uint8Array;
  compressionMethod: number;
}

export interface ZipFile {
  entries: ZipEntry[];
  comment: string;
}

// ── Extra fields parsing ────────────────────────────────────────────────────

interface ExtraField {
  id: number;
  length: number;
  value: Uint8Array;
}

function readExtraFields(
  reader: DataReader,
  extraFieldsLength: number
): Map<number, ExtraField> {
  const end = reader.index + extraFieldsLength;
  const fields = new Map<number, ExtraField>();
  while (reader.index + 4 <= end) {
    const id = reader.readInt(2);
    const length = reader.readInt(2);
    const value = reader.readData(length);
    fields.set(id, { id, length, value });
  }
  reader.setIndex(end);
  return fields;
}

function findUnicodePath(
  fields: Map<number, ExtraField>,
  fileName: Uint8Array
): string | null {
  const upath = fields.get(0x7075);
  if (!upath) return null;
  const r = new DataReader(upath.value);
  if (r.readInt(1) !== 1) return null;
  if (crc32(fileName) !== (r.readInt(4) >>> 0)) return null;
  return utf8decode(r.readData(upath.length - 5));
}

function findUnicodeComment(
  fields: Map<number, ExtraField>,
  fileComment: Uint8Array
): string | null {
  const ucomment = fields.get(0x6375);
  if (!ucomment) return null;
  const r = new DataReader(ucomment.value);
  if (r.readInt(1) !== 1) return null;
  if (crc32(fileComment) !== (r.readInt(4) >>> 0)) return null;
  return utf8decode(r.readData(ucomment.length - 5));
}

// ── Internal entry structure used during parsing ────────────────────────────

interface RawEntry {
  versionMadeBy: number;
  bitFlag: number;
  compressionMethod: number;
  date: Date;
  crc32: number;
  compressedSize: number;
  uncompressedSize: number;
  fileNameLength: number;
  extraFieldsLength: number;
  fileCommentLength: number;
  diskNumberStart: number;
  internalFileAttributes: number;
  externalFileAttributes: number;
  localHeaderOffset: number;
  fileName: Uint8Array;
  fileComment: Uint8Array;
  extraFields: Map<number, ExtraField>;
}

function readCentralEntry(reader: DataReader, isZip64: boolean): RawEntry {
  const versionMadeBy = reader.readInt(2);
  reader.skip(2); // version needed
  const bitFlag = reader.readInt(2);
  const compressionMethod = reader.readInt(2);
  const date = reader.readDate();
  const entryCrc32 = reader.readInt(4);
  let compressedSize = reader.readInt(4);
  let uncompressedSize = reader.readInt(4);
  const fileNameLength = reader.readInt(2);
  const extraFieldsLength = reader.readInt(2);
  const fileCommentLength = reader.readInt(2);
  let diskNumberStart = reader.readInt(2);
  const internalFileAttributes = reader.readInt(2);
  const externalFileAttributes = reader.readInt(4);
  let localHeaderOffset = reader.readInt(4);

  if (bitFlag & 0x0001) {
    throw new Error("Encrypted zip entries are not supported");
  }

  const fileName = reader.readData(fileNameLength);
  const extraFields = readExtraFields(reader, extraFieldsLength);
  const fileComment = reader.readData(fileCommentLength);

  // ZIP64 extra field (0x0001)
  const zip64extra = extraFields.get(0x0001);
  if (zip64extra) {
    const z = new DataReader(zip64extra.value);
    if (uncompressedSize === MAX_VALUE_32) uncompressedSize = z.readInt(8);
    if (compressedSize === MAX_VALUE_32) compressedSize = z.readInt(8);
    if (localHeaderOffset === MAX_VALUE_32) localHeaderOffset = z.readInt(8);
    if (diskNumberStart === MAX_VALUE_16) diskNumberStart = z.readInt(4);
  }

  return {
    versionMadeBy,
    bitFlag,
    compressionMethod,
    date,
    crc32: entryCrc32,
    compressedSize,
    uncompressedSize,
    fileNameLength,
    extraFieldsLength,
    fileCommentLength,
    diskNumberStart,
    internalFileAttributes,
    externalFileAttributes,
    localHeaderOffset,
    fileName,
    fileComment,
    extraFields,
  };
}

function readLocalEntry(reader: DataReader, raw: RawEntry): Uint8Array {
  // Skip: version(2) + bitflag(2) + method(2) + time(4) + crc(4) + compressed(4) + uncompressed(4) = 22
  reader.skip(22);
  const fileNameLength = reader.readInt(2);
  const localExtraLength = reader.readInt(2);
  reader.skip(fileNameLength + localExtraLength);
  return reader.readData(raw.compressedSize);
}

function processEntry(raw: RawEntry, compressedData: Uint8Array): ZipEntry {
  const isUtf8 = (raw.bitFlag & 0x0800) !== 0;
  const madeBy = raw.versionMadeBy >> 8;

  let fileNameStr: string;
  if (isUtf8) {
    fileNameStr = utf8decode(raw.fileName);
  } else {
    const upath = findUnicodePath(raw.extraFields, raw.fileName);
    fileNameStr = upath !== null ? upath : utf8decode(raw.fileName);
  }

  let commentStr: string;
  if (isUtf8) {
    commentStr = utf8decode(raw.fileComment);
  } else {
    const ucomment = findUnicodeComment(raw.extraFields, raw.fileComment);
    commentStr = ucomment !== null ? ucomment : utf8decode(raw.fileComment);
  }

  // Attributes
  let isDir = (raw.externalFileAttributes & 0x0010) !== 0;
  let unixPermissions: number | null = null;
  let dosPermissions: number | null = null;

  if (madeBy === 0x00) {
    // DOS
    dosPermissions = raw.externalFileAttributes & 0x3f;
  }
  if (madeBy === 0x03) {
    // Unix
    unixPermissions = (raw.externalFileAttributes >> 16) & 0xffff;
  }

  if (!isDir && fileNameStr.endsWith("/")) {
    isDir = true;
  }

  const unsafeName = fileNameStr;
  const name = resolvePath(fileNameStr);

  return {
    name,
    unsafeName,
    isDir,
    date: raw.date,
    comment: commentStr,
    unixPermissions,
    dosPermissions,
    compressedSize: raw.compressedSize,
    uncompressedSize: raw.uncompressedSize,
    crc32: raw.crc32,
    compressedData,
    compressionMethod: raw.compressionMethod,
  };
}

// ── Main parser ─────────────────────────────────────────────────────────────

export function readZip(data: Uint8Array): ZipFile {
  const reader = new DataReader(data);

  // 1. Find End of Central Directory
  const eocdOffset = reader.lastIndexOfSignature(SIG_EOCD);
  if (eocdOffset < 0) {
    throw new Error("Can't find end of central directory");
  }

  reader.setIndex(eocdOffset);
  reader.checkSignature(SIG_EOCD);

  let diskNumber = reader.readInt(2);
  let diskWithCentralDir = reader.readInt(2);
  let centralDirRecordsOnDisk = reader.readInt(2);
  let centralDirRecords = reader.readInt(2);
  let centralDirSize = reader.readInt(4);
  let centralDirOffset = reader.readInt(4);
  const zipCommentLength = reader.readInt(2);
  const zipComment = utf8decode(reader.readData(zipCommentLength));

  // 2. Check for ZIP64
  let isZip64 = false;
  if (
    diskNumber === MAX_VALUE_16 ||
    diskWithCentralDir === MAX_VALUE_16 ||
    centralDirRecordsOnDisk === MAX_VALUE_16 ||
    centralDirRecords === MAX_VALUE_16 ||
    centralDirSize === MAX_VALUE_32 ||
    centralDirOffset === MAX_VALUE_32
  ) {
    isZip64 = true;

    const zip64LocatorOffset = reader.lastIndexOfSignature(SIG_ZIP64_LOCATOR);
    if (zip64LocatorOffset < 0) {
      throw new Error("Can't find ZIP64 end of central directory locator");
    }
    reader.setIndex(zip64LocatorOffset);
    reader.checkSignature(SIG_ZIP64_LOCATOR);

    reader.skip(4); // disk with zip64 EOCD
    const zip64EocdOffset = reader.readInt(8);
    reader.skip(4); // total disks

    reader.setIndex(zip64EocdOffset);
    reader.checkSignature(SIG_ZIP64_EOCD);

    reader.skip(8); // zip64 EOCD size
    reader.skip(4); // version made by + version needed
    diskNumber = reader.readInt(4);
    diskWithCentralDir = reader.readInt(4);
    centralDirRecordsOnDisk = reader.readInt(8);
    centralDirRecords = reader.readInt(8);
    centralDirSize = reader.readInt(8);
    centralDirOffset = reader.readInt(8);
  }

  // 3. Read Central Directory entries
  reader.setIndex(centralDirOffset);
  const rawEntries: RawEntry[] = [];
  for (let i = 0; i < centralDirRecords; i++) {
    if (!reader.readAndCheckSignature(SIG_CENTRAL)) {
      // Also try: maybe we read all that's there
      break;
    }
    rawEntries.push(readCentralEntry(reader, isZip64));
  }

  // 4. Read Local File Headers and extract data
  const entries: ZipEntry[] = [];
  for (const raw of rawEntries) {
    reader.setIndex(raw.localHeaderOffset);
    reader.checkSignature(SIG_LOCAL);
    const compressedData = readLocalEntry(reader, raw);
    entries.push(processEntry(raw, compressedData));
  }

  return { entries, comment: zipComment };
}

// ── Decompression ───────────────────────────────────────────────────────────

export function decompressEntry(entry: ZipEntry): Uint8Array {
  let data: Uint8Array;

  if (entry.compressionMethod === 0) {
    // STORE
    data = entry.compressedData;
  } else if (entry.compressionMethod === 8) {
    // DEFLATE
    data = Bun.inflateSync(entry.compressedData);
  } else {
    throw new Error(
      `Unsupported compression method: ${entry.compressionMethod}`
    );
  }

  // CRC32 verification (skip for directories / empty files)
  if (data.length > 0) {
    const computed = crc32(data);
    const expected = entry.crc32 >>> 0;
    if (computed !== expected) {
      throw new Error(
        `CRC32 mismatch for "${entry.name}": expected ${expected.toString(16)}, got ${computed.toString(16)}`
      );
    }
  }

  return data;
}
