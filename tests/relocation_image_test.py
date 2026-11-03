#!/usr/bin/env python3
"""The game module's pointer tables after loading: RELA against RELR.

    python3 tests/relocation_image_test.py RELA_MODULE.so RELR_MODULE.so

Builds the module twice (one linked as before, one with -z pack-relative-relocs),
then loads both as the host does (dlopen) and checks that the loader filled in
every pointer the same way:

  - the two builds relocate the same set of slots;
  - each relocated slot, read from memory after loading, points at the same
    place in both: the same section, the same offset within it;
  - every byte that isn't a relocated slot is identical.

Pointers are compared as (section, offset) because RELR only changes the
relocation sections: that moves the sections after them, but not their
contents, so a correct pair agrees everywhere. Linux and x86-64 ELF only.
"""
import bisect
import ctypes
import mmap
import os
import struct
import sys

SHT_RELA, SHT_NOBITS, SHT_RELR = 4, 8, 19
SHF_WRITE, SHF_ALLOC = 0x1, 0x2
R_X86_64_RELATIVE = 8
RTLD_DI_LINKMAP = 2
# Sections a RELR link is expected to change; everything else must match. .relro_padding
# is lld's empty padding to the page after the read-only-after-relocation data: it holds
# no bytes, and its size follows wherever the sections before it end.
CHANGED_BY_RELR = {".rela.dyn", ".relr.dyn", ".dynamic", ".dynstr", ".gnu.version_r", ".gnu.version", ".dynsym",
                   ".relro_padding"}


class Module:
    def __init__(self, path):
        self.path = os.path.realpath(path)
        with open(self.path, "rb") as f:
            self.file = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        if self.file[:4] != b"\x7fELF" or self.file[4] != 2 or self.file[5] != 1:
            raise SystemExit(f"{path}: not a 64-bit little-endian ELF file")
        shoff, = struct.unpack_from("<Q", self.file, 0x28)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", self.file, 0x3A)
        raw = [struct.unpack_from("<IIQQQQIIQQ", self.file, shoff + i * shentsize) for i in range(shnum)]
        names_off = raw[shstrndx][4]
        def name(at):
            end = self.file.find(b"\0", names_off + at)
            return self.file[names_off + at:end].decode()
        self.sections = {}
        for (nm, kind, flags, addr, off, size, *_rest) in raw:
            if nm or kind:
                self.sections[name(nm)] = (kind, flags, addr, off, size)
        alloc = sorted((s[2], n) for n, s in self.sections.items() if s[1] & SHF_ALLOC and s[4])
        self._starts = [a for a, _ in alloc]
        self._names = [n for _, n in alloc]
        self.slots = self._relative_slots()
        self.handle = ctypes.CDLL(self.path, mode=os.RTLD_NOW | os.RTLD_LOCAL)
        self.base = self._load_base()

    def _relative_slots(self):
        """Every address the loader adds the load base to, from RELA and RELR."""
        slots = set()
        for kind, _flags, _addr, off, size in self.sections.values():
            if kind == SHT_RELA:
                for i in range(0, size, 24):
                    r_offset, r_info = struct.unpack_from("<QQ", self.file, off + i)
                    if r_info & 0xFFFFFFFF == R_X86_64_RELATIVE:
                        slots.add(r_offset)
            elif kind == SHT_RELR:
                where = 0
                for i in range(0, size, 8):
                    entry, = struct.unpack_from("<Q", self.file, off + i)
                    if entry & 1 == 0:          # an address: relocate it, and the bitmap counts on from the next word
                        slots.add(entry)
                        where = entry + 8
                    else:                       # a bitmap of the 63 words from `where`
                        bits, n = entry >> 1, 0
                        while bits:
                            if bits & 1:
                                slots.add(where + n * 8)
                            bits >>= 1
                            n += 1
                        where += 63 * 8
        return slots

    def _load_base(self):
        libc = ctypes.CDLL(None)
        link_map = ctypes.c_void_p()
        if libc.dlinfo(ctypes.c_void_p(self.handle._handle), RTLD_DI_LINKMAP, ctypes.byref(link_map)) != 0:
            raise SystemExit(f"{self.path}: dlinfo failed")
        return ctypes.c_size_t.from_address(link_map.value).value   # link_map's first field, l_addr

    def place(self, address):
        """A link-time address as (section, offset within it)."""
        i = bisect.bisect_right(self._starts, address) - 1
        if i < 0:
            return ("<before sections>", address)
        n = self._names[i]
        return (n, address - self._starts[i])

    def address(self, section, offset):
        return self.sections[section][2] + offset

    def loaded(self, section):
        _kind, _flags, addr, _off, size = self.sections[section]
        return ctypes.string_at(self.base + addr, size)


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    a, b = Module(argv[1]), Module(argv[2])
    print(f"{argv[1]}: {len(a.slots)} relative slots, loaded at {a.base:#x}")
    print(f"{argv[2]}: {len(b.slots)} relative slots, loaded at {b.base:#x}")
    failures = 0

    # The same sections, the same sizes, apart from those RELR is meant to change.
    for n in sorted(set(a.sections) | set(b.sections)):
        if n in CHANGED_BY_RELR or n.startswith(".debug") or n in (".symtab", ".strtab", ".shstrtab", ".comment"):
            continue
        sa, sb = a.sections.get(n), b.sections.get(n)
        if sa is None or sb is None or sa[4] != sb[4]:
            print(f"FAIL section {n}: {sa and sa[4]} bytes against {sb and sb[4]}")
            failures += 1

    # The same slots.
    places_a = {a.place(s) for s in a.slots}
    places_b = {b.place(s) for s in b.slots}
    if places_a != places_b:
        print(f"FAIL slots: {len(places_a - places_b)} only in the first, {len(places_b - places_a)} only in the second")
        failures += 1

    # Each slot points at the same place, and everything else in those sections matches.
    by_section = {}
    for sec, off in places_a & places_b:
        by_section.setdefault(sec, []).append(off)
    checked = 0
    for sec, offsets in sorted(by_section.items()):
        ma, mb = bytearray(a.loaded(sec)), bytearray(b.loaded(sec))
        wrong = 0
        for off in offsets:
            va, = struct.unpack_from("<Q", ma, off)
            vb, = struct.unpack_from("<Q", mb, off)
            if a.place(va - a.base) != b.place(vb - b.base):
                if wrong < 3:
                    print(f"FAIL {sec}+{off:#x}: {a.place(va - a.base)} against {b.place(vb - b.base)}")
                wrong += 1
            ma[off:off + 8] = mb[off:off + 8] = b"\0" * 8
        checked += len(offsets)
        if wrong:
            print(f"FAIL {sec}: {wrong} of {len(offsets)} slots point elsewhere")
            failures += 1
        if ma != mb:
            first = next(i for i in range(len(ma)) if ma[i] != mb[i])
            print(f"FAIL {sec}: unrelocated bytes differ, first at +{first:#x}")
            failures += 1
        print(f"  {sec}: {len(offsets)} slots")

    if failures:
        print(f"FAILED: {failures} problem(s)")
        return 1
    print(f"PASS: {checked} relocated pointers point at the same places in both, and every other byte matches")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
