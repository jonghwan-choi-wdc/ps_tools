import ctypes
import sys

# Define the constants
VBA_FTL_FLAVOR1_NUM_BITS_FMU_IN_MB = 17
VBA_FTL_FLAVOR1_NUM_BITS_MBID = 15

VBA_FTL_FLAVOR2_NUM_BITS_FMU_IN_MB = 17
VBA_FTL_FLAVOR2_NUM_BITS_MB = 11
VBA_FTL_FLAVOR2_NUM_BITS_MD = 4

VBA_PS_FLAVOR3_NUM_BITS_FMU = 2
VBA_PS_FLAVOR3_NUM_BITS_PLANE = 22
VBA_PS_FLAVOR3_NUM_BITS_FIM = 1
VBA_PS_FLAVOR3_NUM_BITS_DIE_PAGE_IN_BLOCK = 12
VBA_PS_FLAVOR3_NUM_BITS_META_BLOCK = 11
VBA_PS_FLAVOR3_NUM_BITS_META_DIE = 4

VBA_PS_FLAVOR3_NUM_BITS_FMU = 2
VBA_PS_FLAVOR3_NUM_BITS_PLANE = 2
VBA_PS_FLAVOR3_NUM_BITS_FIM = 1
VBA_PS_FLAVOR3_NUM_BITS_DIE_PAGE_IN_BLOCK = 12
VBA_PS_FLAVOR3_NUM_BITS_META_BLOCK = 11
VBA_PS_FLAVOR3_NUM_BITS_META_DIE = 4

VBA_FTL_FLAVOR1_NUM_BITS_FMU_IN_MB = 17
VBA_FTL_FLAVOR1_NUM_BITS_MBID = 15

VBA_FTL_FLAVOR2_NUM_BITS_FMU_IN_MB = 17
VBA_FTL_FLAVOR2_NUM_BITS_MB  = 11
VBA_FTL_FLAVOR2_NUM_BITS_MD = 4

VBA_FTL_FLAVOR3_NUM_BITS_FMU_IN_MB = 17
VBA_FTL_FLAVOR3_NUM_BITS_MB = 11
VBA_FTL_FLAVOR3_NUM_BITS_CG = 1

VBA_TLC_FLAVOR_NUM_BITS_FMU = 2
VBA_TLC_FLAVOR_NUM_BITS_PLANE = 2
VBA_TLC_FLAVOR_NUM_BITS_LOW_FIM = 1 
VBA_TLC_FLAVOR_NUM_BITS_DIE_PAGE_IN_BLOCK = 12
VBA_TLC_FLAVOR_NUM_BITS_BLOCK = 11
VBA_TLC_FLAVOR_NUM_BITS_FIM_HIGH = 1
VBA_TLC_FLAVOR_NUM_BITS_DIE_IN_FIM = 3

VBA_SLC_FLAVOR_NUM_BITS_FMU = 2
VBA_SLC_FLAVOR_NUM_BITS_PLANE = 2
VBA_SLC_FLAVOR_NUM_BITS_LOW_FIM = 1
VBA_SLC_FLAVOR_NUM_BITS_STRING = 3
VBA_SLC_FLAVOR_NUM_BITS_WORDLINE = 8
VBA_SLC_FLAVOR_NUM_BITS_DIE_PAGE_IN_BLOCK  = 12
VBA_SLC_FLAVOR_NUM_BITS_RFU = 1
VBA_SLC_FLAVOR_NUM_BITS_SUB_BLOCK = 1
VBA_SLC_FLAVOR_NUM_BITS_BLOCK = 10
VBA_SLC_FLAVOR_NUM_BITS_FIM_HIGH  = 1
VBA_SLC_FLAVOR_NUM_BITS_DIE_IN_FIM = 3

# Define the bb_s structure
class FTL_flavor1_s(ctypes.Structure):
    _fields_ = [("fmuInMB", ctypes.c_uint, VBA_FTL_FLAVOR1_NUM_BITS_FMU_IN_MB),
                ("MBID", ctypes.c_uint, VBA_FTL_FLAVOR1_NUM_BITS_MBID)]

class FTL_flavor2_s(ctypes.Structure):
    _fields_ = [("fmuInMB", ctypes.c_uint, VBA_FTL_FLAVOR2_NUM_BITS_FMU_IN_MB),
                ("MB", ctypes.c_uint, VBA_FTL_FLAVOR2_NUM_BITS_MB),
                ("MD", ctypes.c_uint, VBA_FTL_FLAVOR2_NUM_BITS_MD)]

class PS_flavor3_s(ctypes.Structure):
    _fields_ = [("fmu", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_FMU),
                ("plane", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_PLANE),
                ("fim", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_FIM),
                ("diePageInBlk", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_DIE_PAGE_IN_BLOCK),
                ("MB", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_META_BLOCK),
                ("MD", ctypes.c_uint, VBA_PS_FLAVOR3_NUM_BITS_META_DIE)]

class vbaTlc_s(ctypes.Structure):
    _fields_ = [("fmu", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_FMU),
                ("plane", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_PLANE),
                ("lFim", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_LOW_FIM),
                ("diePageInBlk", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_DIE_PAGE_IN_BLOCK),
                ("block", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_BLOCK),
                ("psid", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_FIM_HIGH),
                ("dieInFim", ctypes.c_uint, VBA_TLC_FLAVOR_NUM_BITS_DIE_IN_FIM)]

class vbaSlc_sbm_s(ctypes.Structure):
    _fields_ = [("fmu", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_FMU),
                ("plane", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_PLANE),
                ("lFim", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_LOW_FIM),
                ("diePageInBlk", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_DIE_PAGE_IN_BLOCK),
                ("subBlock", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_SUB_BLOCK),
                ("block", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_BLOCK),
                ("psid", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_FIM_HIGH),
                ("dieInFim", ctypes.c_uint, VBA_SLC_FLAVOR_NUM_BITS_DIE_IN_FIM)]

# Define the vba union
class vba(ctypes.Union):
    _fields_ = [("vba32", ctypes.c_uint),
                ("FTL_flavor1", FTL_flavor1_s),
                ("FTL_flavor2", FTL_flavor2_s),
                ("PS_flavor3", PS_flavor3_s),
                ("vbaTlc", vbaTlc_s),
                ("vbaSlc_sbm", vbaSlc_sbm_s)]

# Define the VBA_Parser class
class VBA_Parser:
    def __init__(self):
        pass  # Add any initialization code here

    def parse_vba(self, hex_value):
        # Convert hex string to integer
        value = int(hex_value, 16)
        
        # Create a bytes object from the integer
        data = value.to_bytes((value.bit_length() + 7) // 8, 'big')
        
        # Convert bytes to a ctypes structure
        vba_union = vba()
        vba_bytes = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        ctypes.memmove(ctypes.byref(vba_union), ctypes.byref(vba_bytes), ctypes.sizeof(vba_bytes))
        
        # Access the fields of the union
        print("vba32:", vba_union.vba32)
        print("FTL_flavor1.fmuInMB:", vba_union.FTL_flavor1.fmuInMB)
        print("FTL_flavor1.MBID:", vba_union.FTL_flavor1.MBID)

        print("FTL_flavor2.fmuInMB:", vba_union.FTL_flavor2.fmuInMB)
        print("FTL_flavor2.MB:", vba_union.FTL_flavor2.MB)
        print("FTL_flavor2.MD:", vba_union.FTL_flavor2.MD)
 
        print("PS_flavor3.fm:", vba_union.PS_flavor3.fmu)
        print("PS_flavor3.plane:", vba_union.PS_flavor3.plane)
        print("PS_flavor3.fim:", vba_union.PS_flavor3.fim)
        print("PS_flavor3.diePageInBlk:", vba_union.PS_flavor3.diePageInBlk)
        print("PS_flavor3.MB:", vba_union.PS_flavor3.MB)
        print("PS_flavor3.MD:", vba_union.PS_flavor3.MD)

# Main function
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script.py <hex_value>")
        sys.exit(1)
    
    hex_value = sys.argv[1]
    parser = VBA_Parser()
    parser.parse_vba(hex_value)



