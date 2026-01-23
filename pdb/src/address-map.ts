import { ParseBuffer } from "./buffer.js";
import type { MsfStream } from "./msf.js";
import type {
  Rva,
  PdbInternalRva,
  PdbInternalSectionOffset,
  SectionOffset,
  ImageSectionHeader,
} from "./common.js";
import { rva, pdbInternalRva } from "./common.js";

/** An OMAP record mapping one address range to another. */
interface OmapRecord {
  sourceRva: number;
  targetRva: number;
}

/**
 * Address map for translating between PDB internal addresses and actual RVAs.
 *
 * PDB files from optimized (BBT/OMAP) binaries store addresses in an internal
 * address space. The AddressMap provides methods to translate these to real RVAs
 * suitable for debugging.
 */
export class AddressMap {
  private sectionHeaders: ImageSectionHeader[];
  private originalSectionHeaders: ImageSectionHeader[] | null;
  private omapFromSrc: OmapRecord[] | null;
  private omapToSrc: OmapRecord[] | null;

  constructor(
    sectionHeaders: ImageSectionHeader[],
    originalSectionHeaders: ImageSectionHeader[] | null,
    omapFromSrc: OmapRecord[] | null,
    omapToSrc: OmapRecord[] | null,
  ) {
    this.sectionHeaders = sectionHeaders;
    this.originalSectionHeaders = originalSectionHeaders;
    this.omapFromSrc = omapFromSrc;
    this.omapToSrc = omapToSrc;
  }

  /** Whether this PDB uses OMAP address translation. */
  get hasOmap(): boolean {
    return this.omapFromSrc !== null;
  }

  /** Convert a PDB internal section offset to an RVA. */
  sectionOffsetToRva(offset: PdbInternalSectionOffset): Rva | null {
    const internalRva = this.sectionOffsetToInternalRva(offset);
    if (internalRva === null) return null;

    if (this.omapFromSrc) {
      return this.omapLookup(this.omapFromSrc, internalRva);
    }
    return rva(internalRva);
  }

  /** Convert a PDB internal section offset to an internal RVA. */
  sectionOffsetToInternalRva(
    offset: PdbInternalSectionOffset,
  ): PdbInternalRva | null {
    const headers = this.originalSectionHeaders ?? this.sectionHeaders;
    return sectionOffsetToRvaWithHeaders(offset, headers);
  }

  /** Convert an RVA to a section offset. */
  rvaToSectionOffset(addr: Rva): SectionOffset | null {
    for (let i = 0; i < this.sectionHeaders.length; i++) {
      const section = this.sectionHeaders[i];
      const sectionStart = section.virtualAddress;
      const sectionEnd = sectionStart + Math.max(section.virtualSize, section.sizeOfRawData);
      if (addr >= sectionStart && addr < sectionEnd) {
        return {
          section: i + 1,
          offset: addr - sectionStart,
        };
      }
    }
    return null;
  }

  /** Perform OMAP lookup to translate an internal RVA to an actual RVA. */
  private omapLookup(omap: OmapRecord[], internalRva: number): Rva | null {
    if (omap.length === 0) return null;

    // Binary search for the largest source RVA <= internalRva
    let lo = 0;
    let hi = omap.length - 1;

    while (lo < hi) {
      const mid = (lo + hi + 1) >>> 1;
      if (omap[mid].sourceRva <= internalRva) {
        lo = mid;
      } else {
        hi = mid - 1;
      }
    }

    const record = omap[lo];
    if (record.sourceRva > internalRva) return null;

    if (record.targetRva === 0) {
      // This range was removed
      return null;
    }

    const offset = internalRva - record.sourceRva;
    return rva(record.targetRva + offset);
  }
}

/** Parse section headers from a stream. */
export function parseSectionHeaders(stream: MsfStream): ImageSectionHeader[] {
  const buf = new ParseBuffer(stream.data);
  const headers: ImageSectionHeader[] = [];
  const SECTION_HEADER_SIZE = 40;

  while (buf.remaining >= SECTION_HEADER_SIZE) {
    const nameBytes = buf.take(8);
    // Trim NUL bytes from name
    let nameEnd = nameBytes.indexOf(0);
    if (nameEnd === -1) nameEnd = 8;
    const name = new TextDecoder().decode(nameBytes.subarray(0, nameEnd));

    const virtualSize = buf.readU32();
    const virtualAddress = buf.readU32();
    const sizeOfRawData = buf.readU32();
    const pointerToRawData = buf.readU32();
    const pointerToRelocations = buf.readU32();
    const pointerToLinenumbers = buf.readU32();
    const numberOfRelocations = buf.readU16();
    const numberOfLinenumbers = buf.readU16();
    const characteristics = buf.readU32();

    headers.push({
      name,
      virtualSize,
      virtualAddress,
      sizeOfRawData,
      pointerToRawData,
      pointerToRelocations,
      pointerToLinenumbers,
      numberOfRelocations,
      numberOfLinenumbers,
      characteristics,
    });
  }

  return headers;
}

/** Parse OMAP records from a stream. */
export function parseOmapStream(stream: MsfStream): OmapRecord[] {
  const buf = new ParseBuffer(stream.data);
  const records: OmapRecord[] = [];

  while (buf.remaining >= 8) {
    const sourceRva = buf.readU32();
    const targetRva = buf.readU32();
    records.push({ sourceRva, targetRva });
  }

  return records;
}

/** Build an AddressMap from the available streams. */
export function buildAddressMap(
  sectionHeadersStream: MsfStream | null,
  originalSectionHeadersStream: MsfStream | null,
  omapFromSrcStream: MsfStream | null,
  omapToSrcStream: MsfStream | null,
): AddressMap {
  const sectionHeaders = sectionHeadersStream
    ? parseSectionHeaders(sectionHeadersStream)
    : [];
  const originalSectionHeaders = originalSectionHeadersStream
    ? parseSectionHeaders(originalSectionHeadersStream)
    : null;
  const omapFromSrc = omapFromSrcStream
    ? parseOmapStream(omapFromSrcStream)
    : null;
  const omapToSrc = omapToSrcStream
    ? parseOmapStream(omapToSrcStream)
    : null;

  return new AddressMap(
    sectionHeaders,
    originalSectionHeaders,
    omapFromSrc,
    omapToSrc,
  );
}

/** Convert a section offset to an RVA using section headers. */
function sectionOffsetToRvaWithHeaders(
  offset: PdbInternalSectionOffset,
  headers: ImageSectionHeader[],
): PdbInternalRva | null {
  if (offset.section === 0) return null;
  const sectionIdx = offset.section - 1;
  if (sectionIdx >= headers.length) return null;
  const section = headers[sectionIdx];
  return pdbInternalRva(section.virtualAddress + offset.offset);
}
