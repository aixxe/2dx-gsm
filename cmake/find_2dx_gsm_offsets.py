import mmap
import time
import struct
import pefile
import pathlib
import re


def pos():
    return mm.tell()


def find_pattern(pattern, offset=0, adjust=0):
    if "?" in pattern:
        s = pattern.split(" ")
        fixedpattern = []
        for h in s:
            if h == "?":
                h = "."
            else:
                h = f"\\x{h}"
            fixedpattern.append(h)
        pattern = "".join(fixedpattern).encode()

    # print(pattern)

    if type(pattern) == str:
        return mm.seek(mm.find(bytes.fromhex(pattern), offset) + adjust)
    elif type(pattern) == bytes:
        return mm.seek(re.search(pattern, mm[offset:], flags=re.DOTALL).start() + offset + adjust)


def dereference(offset, length):
    relative = pe.get_rva_from_offset(pos())
    mm.seek(pos() + offset)
    disp = struct.unpack("<i", mm.read(4))[0]
    return relative + disp + length


def find_pattern_backwards(pattern, adjust=0):
    pattern = pattern.replace(" ", "")
    pattern_len = len(pattern) // 2
    while mm.read(pattern_len) != bytes.fromhex(pattern):
        mm.seek(pos() - pattern_len - 1)
    if adjust != 0:
        mm.seek(pos() + adjust)

# example dll format: bm2dx-2025111900-33-010.dll
for dll in pathlib.Path(".").glob("bm2dx*.dll"):
    with open(dll, "r+b") as infile:
        mm = mmap.mmap(infile.fileno(), length=0, access=mmap.ACCESS_READ)
        pe = pefile.PE(dll, fast_load=True)

        dll_date = dll.name.split("-")[1]
        dll_type = dll.name.split("-")[3].split(".")[0]

        output_filename = f"2dx-gsm.{dll_date}-{dll_type}.cmake"
        print(f"-> {output_filename}")

        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
        output = [
            f"# Generated at {timestamp}",
            f"set(EXPECTED_BM2DX_PRODUCT_VERSION                     \"{dll_date}-{dll_type}\")\n",

            f"set(EXPECTED_BM2DX_NT_CODE_SIZE                        0x{pe.OPTIONAL_HEADER.SizeOfCode:08x})",
            f"set(EXPECTED_BM2DX_NT_ENTRYPOINT                       0x{pe.OPTIONAL_HEADER.AddressOfEntryPoint:08x})",
            f"set(EXPECTED_BM2DX_NT_IMAGE_SIZE                       0x{pe.OPTIONAL_HEADER.SizeOfImage:08x})\n",
        ]

        addresses = {}

        # RESOLVED_DEATH_DEFYING_PATCH
        try:
            find_pattern("66 83 F9 64 7D 46")
            find_pattern("48 8B C8 E8", pos(), 3)
        except ValueError:
            find_pattern("E8 ? ? ? ? EB ? B8 ? ? ? ? 66 89 03")
        addresses["RESOLVED_DEATH_DEFYING_PATCH"] = pe.get_rva_from_offset(pos())

        # RESOLVED_GAUGE_RENDER_FN_BEGIN
        find_pattern("48 8B C4 48 89 48 08 53 56 57 41 54 41 56")
        addresses["RESOLVED_GAUGE_RENDER_FN_BEGIN"] = pe.get_rva_from_offset(pos())

        # RESOLVED_GAUGE_RENDER_TEXTURE_FN
        find_pattern("48 8B C4 48 89 68 20 56 57 41 56 48 83 EC 50")
        addresses["RESOLVED_GAUGE_RENDER_TEXTURE_FN"] = pe.get_rva_from_offset(pos())

        # RESOLVED_STATE_PTR
        find_pattern("E8 ? ? ? ? 83 F8 ? 75 ? 48 B8")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)))
        addresses["RESOLVED_STATE_PTR"] = dereference(2, 6)

        # RESOLVED_GAUGE_DATA_PTR
        find_pattern("E8 ? ? ? ? 8B D7 48 8B C8 E8 ? ? ? ? 8B D3")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)))
        addresses["RESOLVED_GAUGE_DATA_PTR"] = dereference(3, 7)

        # RESOLVED_OPTION_DATA_PTR
        find_pattern("E8 ? ? ? ? 49 8B 0F 48 63 11")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)))
        addresses["RESOLVED_OPTION_DATA_PTR"] = dereference(3, 7)

        # RESOLVED_GET_GAUGE_FN
        find_pattern("E8 ? ? ? ? 44 8B E8 48 8D 85")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)))
        addresses["RESOLVED_GET_GAUGE_FN"] = pe.get_rva_from_offset(pos())

        # RESOLVED_SET_GAUGE_FN
        find_pattern("E8 ? ? ? ? 44 8B C5 41 F7 C6")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)))
        addresses["RESOLVED_SET_GAUGE_FN"] = pe.get_rva_from_offset(pos())

        # RESOLVED_IS_DAN_PRACTICE_FN
        find_pattern("E8 ? ? ? ? 84 C0 8B CB")
        addresses["RESOLVED_IS_DAN_PRACTICE_FN"] = dereference(1, 5)

        # RESOLVED_GAUGE_DATA_GHOST_OFFSET
        find_pattern("81 FA 80 01 00 00 7D 09 48 63 C2 0F BF 44 41", 0, 15)
        offset = struct.unpack("<b", mm.read(1))[0]
        addresses["RESOLVED_GAUGE_DATA_GHOST_OFFSET"] = offset

        # RESOLVED_GAUGE_DATA_COUNT_OFFSET
        find_pattern("48 8B C8 E8 ? ? ? ? 66 FF 80 ? ? 00 00", 0, 11)
        addresses["RESOLVED_GAUGE_DATA_COUNT_OFFSET"] = struct.unpack("<h", mm.read(2))[0]

        # RESOLVED_GAUGE_DATA_PLAYER_OFFSET
        find_pattern("48 8B C1 85 D2 74 07 48 8D 81", 0, 10)
        addresses["RESOLVED_GAUGE_DATA_PLAYER_OFFSET"] = struct.unpack("<i", mm.read(4))[0]

        # RESOLVED_INPUT_PTR
        find_pattern("48 89 05 ? ? ? ? 33 DB 48 89 1D")
        addresses["RESOLVED_INPUT_PTR"] = dereference(3, 7) + 0x8

        # RESOLVED_P1_GROOVE_GAUGE_PTR
        # RESOLVED_P2_GROOVE_GAUGE_PTR
        find_pattern("48 8D 05 ? ? ? ? 0F BF 14 48")
        address = dereference(3, 7)
        addresses["RESOLVED_P1_GROOVE_GAUGE_PTR"] = address
        addresses["RESOLVED_P2_GROOVE_GAUGE_PTR"] = address + 0xA

        # RESOLVED_P1_RESULT_GRAPH_PTR
        # RESOLVED_P2_RESULT_GRAPH_PTR
        find_pattern("E8 ? ? ? ? 0F 28 D6 8B D3")
        mm.seek(pe.get_offset_from_rva(dereference(1, 5)) + 3)
        address = dereference(3, 7) + 0xC8
        addresses["RESOLVED_P1_RESULT_GRAPH_PTR"] = address
        addresses["RESOLVED_P2_RESULT_GRAPH_PTR"] = address + 0x600

        # RESOLVED_P1_CHART_JUDGEMENT_PTR
        # RESOLVED_P2_CHART_JUDGEMENT_PTR
        addresses["RESOLVED_P1_CHART_JUDGEMENT_PTR"] = addresses["RESOLVED_P1_GROOVE_GAUGE_PTR"] + 0x2
        addresses["RESOLVED_P2_CHART_JUDGEMENT_PTR"] = addresses["RESOLVED_P1_GROOVE_GAUGE_PTR"] + 0xC

        # RESOLVED_P1_GAUGE_OPTION_PTR
        # RESOLVED_P2_GAUGE_OPTION_PTR
        find_pattern("48 8D 35 ? ? ? ? 44 8B 05")
        address = dereference(3, 7) + 0x64
        addresses["RESOLVED_P1_GAUGE_OPTION_PTR"] = address
        addresses["RESOLVED_P2_GAUGE_OPTION_PTR"] = address + 0x38

        # RESOLVED_P1_DEAD_MEASURE_PTR
        # RESOLVED_P2_DEAD_MEASURE_PTR
        find_pattern("41 8B F7 48 8D 59 08", 0, 9)
        dead_measure_base_offset = struct.unpack("<i", mm.read(4))[0]
        find_pattern("E8 ? ? ? ? 8B D6", pos(), 0)
        dead_measure_base = dereference(1, 5)
        mm.seek(pe.get_offset_from_rva(dead_measure_base))
        address = dereference(3, 7) + dead_measure_base_offset
        addresses["RESOLVED_P1_DEAD_MEASURE_PTR"] = address
        addresses["RESOLVED_P2_DEAD_MEASURE_PTR"] = address + 0x4

        # RESOLVED_CALCULATE_INDIVIDUAL_CHART_JUDGE_VALUE
        find_pattern("E8 ? ? ? ? 66 89 47 F6")
        addresses["RESOLVED_CALCULATE_INDIVIDUAL_CHART_JUDGE_VALUE"] = dereference(1, 5)

        # RESOLVED_TARGET_CALCULATE_CHART_JUDGE
        find_pattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B F2 8B E9")
        addresses["RESOLVED_TARGET_CALCULATE_CHART_JUDGE"] = pe.get_rva_from_offset(pos())

        # RESOLVED_TARGET_UPDATE_GROOVE_GAUGE
        find_pattern("E8 ? ? ? ? EB ? E8 ? ? ? ? 48 8B D7")
        addresses["RESOLVED_TARGET_UPDATE_GROOVE_GAUGE"] = dereference(1, 5)

        # RESOLVED_TARGET_UPDATE_GRAPH_DATA
        find_pattern("B8 80 01 00 00 66 3B D0")
        addresses["RESOLVED_TARGET_UPDATE_GRAPH_DATA"] = pe.get_rva_from_offset(pos())

        # RESOLVED_TARGET_DRAW_GRAPH_CTOR
        find_pattern("41 B8 ? ? ? ? E8 ? ? ? ? 33 C9 E8")
        find_pattern_backwards("CC CC CC")
        addresses["RESOLVED_TARGET_DRAW_GRAPH_CTOR"] = pe.get_rva_from_offset(pos())

        # RESOLVED_TARGET_RESULT_GRAPH_RENDER
        find_pattern("40 53 48 83 EC ? 83 B9 ? ? ? ? ? 48 8B D9 74 ? 33 C9")
        addresses["RESOLVED_TARGET_RESULT_GRAPH_RENDER"] = pe.get_rva_from_offset(pos())

        # RESOLVED_TARGET_RETURN_FROM_RESULT
        try:
            # 32
            find_pattern("40 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24 D9 48 81 EC E0 00 00 00")
        except ValueError:
            # 33
            find_pattern("40 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 C7 45 ? ? ? ? ? 4C 8B F9 E8")
        addresses["RESOLVED_TARGET_RETURN_FROM_RESULT"] = pe.get_rva_from_offset(pos())

        # RESOLVED_TARGET_QUICK_RETRY
        find_pattern("BD FF FF FF FF 45 33 FF", 0x800000, -44)
        addresses["RESOLVED_TARGET_QUICK_RETRY"] = pe.get_rva_from_offset(pos())

        for title, address in addresses.items():
            output.append(f"set({title:<50} 0x{address:08x})")

        with open(output_filename, mode="w", newline="\n") as outfile:
            outfile.write("\n".join(output))
