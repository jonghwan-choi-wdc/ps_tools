#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


#define EI_LAST_BACKWARDS_COMPATIBLE_VER     (2)
#define EI_CONFIG_STRUCT_VER                 (19) //This should be incremented for each modification, EI Configuration structure version number
#define EI_SINGLE_INJ_TABLE_SIZE             (6)
#define EI_MAX_BAD_BLOCKS_PER_PLANE_EOL      ((MAX_FBB_ALLOWED) + (CFG_MINIMUM_REQUIRED_SPARES_1Tb_DIE))  //MAIA-5503: Newly define macro for GBB.
#define EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX   (80) // must remain 80, or we will exceed the 4K configuration size
#define EI_MAX_DIES_PER_FIM                  (8)
#define EI_CONFIG_FLOWS_ARR_SZ              (10) //byte array size of flows, each bit represent flow ID to enable EI_Config_t
                                                
#define EI_INVALID_FLBA_ADDR                 (0xffffffffffffffffULL)
#define EI_SINGLE_ANY_ADDRESS                (0xfffffffffffffffeULL)

#define EI_SINGLE_NESTED_ERASE               (0xffff)

//Canary values, protection against memory overflow
#define EI_CANARY_VAL_START (0xDEADBEEF)
#define EI_CANARY_VAL_END   (0xFEEBDAED)

#define EI_OPID_TABLE_MAX_ENTRIES (8)
#define EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE  (3 * EI_OPID_TABLE_MAX_ENTRIES)
#define EI_CHANCES_TABLE_TYPE_MAX_ENTRIES (16)
#define EI_OP_IDS_TABLE_MAX_ENTRIES (40)
#define EI_GET_CONF_GENERAL_PLANE_AFFINITY_MASK(config_p, psId, dieInFim) config_p->generalConfig.planeAffinityMask[psId][dieInFim]
#define EI_SET_CONF_GENERAL_PLANE_AFFINITY_MASK(config_p, psId, dieInFim, value) { config_p->generalConfig.planeAffinityMask[psId][dieInFim] = value; } 

#define EI_AFFINITY_PLANE0 0x55555555
#define EI_AFFINITY_PLANE1 0xAAAAAAAA
#define EI_GET_OTHER_AFFINIFTY_PLANE(_affinityPlane_) (_affinityPlane_ == EI_AFFINITY_PLANE0) ? EI_AFFINITY_PLANE1 : EI_AFFINITY_PLANE0;
//#define EI_DBG_MSG_INFO(...) 

///-----------------------------------------------------------------------------
///     DEFINES
///-----------------------------------------------------------------------------

#define TOTAL_FIMS			(4)
#define NUM_PS_CPU			(2)

#define FIM_PER_PS                   (TOTAL_FIMS/NUM_PS_CPU)
#define CHIPS_PER_FIM                (1)

#define MAX_FBB_ALLOWED                            (74+1)
#define CFG_MINIMUM_REQUIRED_SPARES_1Tb_DIE     (4)

#define DIES_PER_META_DIE                       (2)

#define MAX_DIES_PER_FIM             (8)
#define FMUS_PER_PLANE_PAGE        (4)
#define PLANES_PER_DIE             (4)  
#define EI_MAX_DIES_PER_MD         (DIES_PER_META_DIE)
#define EI_MAX_DIES_PER_PS         (MAX_DIES_PER_PS)
#define EI_FMUS_PER_DIE_PAGE       (FMUS_PER_DIE_PAGE)
#define EI_FMUS_PER_PHYS_PAGE      (4)     
#define EI_FMUS_PER_DIE_PAGE_SHIFT (FMUS_PER_DIE_PAGE_LOG2)
#define EI_FMUS_PER_PLANE_SHIFT    (PS_FMUS_PER_PLANE_LOG2)
#define EI_MAX_FMUS_PER_DIE_PAGE   (EI_FMUS_PER_DIE_PAGE * TLC_PAGES_PER_LWL) //TLC  was used , now using EI_MAX_FMUS_PER_PS_REQ
#define EI_MAX_FMUS_PER_PS_REQ     (FMUS_PER_PLANE_PAGE * PLANES_PER_DIE)
#define EI_MAX_PLANES_PER_PS_OP    (PLANES_PER_DIE * DIES_PER_META_DIE)
#define EI_FMUS_PER_XOR_PAGE       (EI_FMUS_PER_PHYS_PAGE)
#define EI_SECTORS_PER_FMU_SHIFT   (SECTORS_PER_FMU_SHIFT)
#define EI_MAX_EI_PER_VECTOR       (8)     
#define EI_MAX_ALLOWED_SEC_INJ     (4)
#define EI_MAX_NESTED_READ_INJ     (8)
#define EI_MAX_NESTED_PROG_INJ     (1)
#define EI_PAGES_INJECT_IN_XOR     (8)
#define EI_PLANES_PER_DIE_SHIFT    (PLANES_PER_DIE_SHIFT)
#define EI_MAX_SGD_WL_NUM          (9)
#define EI_MAX_WL_FAIL_SGD_LOWTAIL_DETECT  (1) //limit the maximum number of WLs fail SGD detection at the same time
#define EI_MAX_PLANES_PER_METAPAGE (FMUS_PER_META_PAGE/FMUS_PER_PLANE_PAGE)
#define EI_BINS_PER_GROUP          (8)

#define EI_NUM_IV_ENTRIES_PER_PS   (16)
#define EI_MAX_NUM_PS              (2)
#define EI_IV_NUM_PS               (NUM_PS)

#define EI_PLANE0                  (0x01)
#define EI_PLANE1                  (0x02)
#define EI_PLANE2                  (0x04)
#define EI_PLANE3                  (0x08)
#define EI_DUAL_PLANE              (EI_PLANE0 | EI_PLANE1)
#define EI_MULTI_PLANE             (EI_PLANE0 | EI_PLANE1 | EI_PLANE2 | EI_PLANE3)

#define EI_PLANES_PER_DIE          (PLANES_PER_DIE)
#define EI_PLANE2PLANEBIT(plane)    ((uint32_t)(1<<plane))
#define EI_NUM_PLANES2BIT(startplane, numplanes)  ((uint32_t)(((1 << numplanes) -1) << startplane))


#define EI_XOR_STORE_PLANE_MASK    (PLANES_PER_DIE - 1)
#define EI_XOR_STORE_FIM_MASK      ((FIM_PER_PS - 1) << PLANES_PER_DIE_SHIFT)
#define EI_XOR_STORE_FIM_SHIFT     (PLANES_PER_DIE_SHIFT)

#define EI_PHY_BIN_NUMBER          (10)
#define EI_MAX_DIES_PER_FIM                  (8)

#define EI_MAX_DIES_PER_PS         (MAX_DIES_PER_PS)

#define MAX_DIES_PER_PS              (FIM_PER_PS * CHIPS_PER_FIM * MAX_DIES_PER_FIM) // max 16 dies per PS

#define BML_MAX_NUM_METADIES                  (MAX_DIES_PER_FIM * TOTAL_FIMS / DIES_PER_META_DIE) 
#define PROD_PS_ERASE_SUSPEND_READ
///-----------------------------------------------------------------------------
///     TYPES
///-----------------------------------------------------------------------------
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef uint32_t EI_ConfType_t;
typedef uint32_t EI_EPRCodeMapType_t;
typedef uint8_t EI_Percent_t;

///-----------------------------------------------------------------------------
///     GLOBALS
///-----------------------------------------------------------------------------
extern uint8_t gEI_ActiveFlow[EI_CONFIG_FLOWS_ARR_SZ];
///-----------------------------------------------------------------------------
///     ENUMS
///-----------------------------------------------------------------------------
// Error Injection operation ID
typedef enum EI_OpId_e
{
   // Host operations
   EI_OP_ID_HOST             = 0,
   EI_OP_ID_RMW              = 1,
   EI_OP_ID_FUA              = 2,
   EI_OP_ID_DEALLOC          = 3,

   // Management operations
   EI_OP_ID_LOG              = 4,
   EI_OP_ID_PRM              = 5,
   EI_OP_ID_MTM              = 6,
   EI_OP_ID_XOR              = 7,
   EI_OP_ID_XOR_ZONE_REBUILD = 8,
   EI_OP_ID_ACTIVE_RS        = 9,

   // UGSD operations
   EI_OP_ID_FLGP             = 10,
   EI_OP_ID_SCAN_FWD         = 11,
   EI_OP_ID_READ_HEADERS     = 12,

   // Relocation operations
   EI_OP_ID_RLC_HOST_VC      = 13,
   EI_OP_ID_RLC_HOST         = 14,
   EI_OP_ID_RLC_MTM          = 15,
   EI_OP_ID_RLC_XOR          = 16,
   EI_OP_ID_BRLC_HOST        = 17,
   EI_OP_ID_BRLC_RLC         = 18,
   EI_OP_ID_BRLC_MTM         = 19,
   EI_OP_ID_BRLC_XOR         = 20,

   //Infra operations
   EI_OP_ID_INFRA_IFS        = 21,
   EI_OP_ID_INFRA_FADI       = 22,
   EI_OP_ID_INFRA_BOOT       = 23,
   EI_OP_ID_INFRA_DIR        = 24,

   // New OPIDs, uncategorized :-(
   EI_OP_ID_HOSTLESS         = 25,

   // New operations added here!

   // No operations below this line
   EI_NUM_OF_OP_IDS          = 26 ,// Total number of operation IDs
   EI_OP_ID_INFRA_FIRST      = EI_OP_ID_INFRA_IFS, 
   EI_OP_ID_INFRA_LAST       = EI_OP_ID_INFRA_DIR, 
   EI_INVALID_OP             = 0xFF //Athena-Calypso EI merge notes: nice to have: merge its uses EI_INVALID_OP_ID
} EI_OpId_stub_t;
typedef uint32_t EI_OpId_t;

//Operation IDs that are not exposed to the user (internally used)
enum EI_IntOpId_e
{
   EI_INT_OP_ID_XOR_REC_FIRST = EI_NUM_OF_OP_IDS,
   EI_INT_OP_ID_XOR_REC_LOAD_PAGE = EI_INT_OP_ID_XOR_REC_FIRST,
   EI_INT_OP_ID_XOR_REC_START,
   EI_INT_OP_ID_XOR_REC_LAST = EI_INT_OP_ID_XOR_REC_START,
   EI_OP_ID_INFRA_ERASE,
   EI_OP_ID_INFRA_FADI_JTAG,

   EI_UNUSED_OP_ID,
   EI_TOTAL_NUM_OF_OP_IDS,
   EI_INVALID_OP_ID,
#ifdef PROD_PS_UT_SUPPORT
   EI_PSUT_FTL_EI_DISABLE_OP_ID, // default OPID for PSUT that disables FTL EI listener to look at the operation and generate flow param etc
#endif //PROD_PS_UT_SUPPORT
   EI_INT_OP_ID_XOR_REC_TOTAL = EI_INT_OP_ID_XOR_REC_LAST - EI_INT_OP_ID_XOR_REC_FIRST + 1,
};
typedef EI_OpId_t EI_IntOpId_t;

//Operation IDs that are not exposed to the user (internally used)
typedef enum EI_ConfType_e
{
   EI_CONF_TYPE_RANDOM,
   EI_CONF_TYPE_SINGLE,
   EI_CONF_TYPE_TOTAL,
} EI_ConfType_stub_t;

typedef enum EI_ReadFailureType_e
{
   EI_READ_SW_TH = 0,                //Means that read should fail On Syndrom weight above threshold
   EI_READ_BES5_SB_0,                //Means that read should fail until BES7 attempted
   EI_READ_BES5_SB_1,                //Means that read should fail until SB1 attempted
   EI_READ_BES7_SB_2_NOLA,           //Means that read should fail until SB2 attempted
   EI_READ_BES7_SB_1,                //Means that read should fail until BES7 SB1 attempted
   EI_READ_BES7_SB_0,                //Means that read should fail until BES7 SB0 attempted
   EI_READ_UECC_REC,                 //Means that read should always fail (recoverable)
   EI_READ_UECC_UNREC,               //Means that read should always fail (un-recoverable)
   EI_READ_LDPC_RAM_BIT_FLIP,        //LDPC RAM Bit Flip read failure, This failure level can't be mixed with others in the same PS request
   EI_READ_UECC_REC_BY_DRAM,         //Means that read should always fail - recoverable by DRAM (in case of open block) or by XRAM [EI-Athena]
   EI_NUM_OF_READ_FAILURE_TYPES      //Total amount of levels
} EI_ReadFailureType_stub_t;
typedef uint32_t EI_ReadFailureType_t; //This represent the values in the "chances" configuration table

typedef enum EI_ProgFailureType_e
{
   EI_PROG_FAILURE_TYPE_1WL               = 0,
   EI_PROG_FAILURE_TYPE_WL2WL             = 1,
   EI_PROG_FAILURE_TYPE_LWL2LWL           = 2,
   EI_PROG_FAILURE_TYPE_1WL_2PLANES       = 3,
   EI_PROG_FAILURE_TYPE_2PLANES_WL2WL     = 4,
   EI_PROG_FAILURE_TYPE_WL2WL_P2P         = 5,
   EI_PROG_FAILURE_TYPE_WRITE_ABORT       = 6,
   EI_PROG_FAILURE_TYPE_EPWR              = 7,
   EI_PROG_FAILURE_TYPE_1LWL              = 8,
   //Athena-Calypso EI merge notes: keep Calypso index of EI_PROG_FAILURE_TYPE_SKIP_PAR_PAGES =9 (was 11 in athena)
   EI_PROG_FAILURE_TYPE_SKIP_PAR_PAGES    = 9,  //Do not write parity pages when finishing the zone.  Use to emulate UGSD in PS UT only
   EI_PROG_FAILURE_TYPE_EPWR_BES7         = 10, //Placeholder until Hermes II changes are merged
   EI_PROG_FAILURE_TYPE_EPWR_BES5         = 11, //Placeholder until Hermes II changes are merged
   EI_PROG_FAILURE_TYPE_BLU_FAILURES      = 12, //One of the Block Level UECC failures. Use failChanceProgBlu for final failure

   EI_NUM_OF_PROG_FAILURE_TYPES,                //Total amount of levels
} EI_ProgFailureType_stub_t;
typedef uint32_t EI_ProgFailureType_t;

typedef enum EI_ProgBluFailureType_e
{
   EI_PROG_BLU_FAILURE_TYPE_BLU                       = 0,  //Block Level UECC
   EI_PROG_BLU_FAILURE_TYPE_BLU_2PLANES               = 1,  //Block Level UECC + 1WL on other plane
   EI_PROG_BLU_FAILURE_TYPE_BLU_P2P                   = 2,  //Block Level UECC + UECC on other plane
   EI_PROG_BLU_FAILURE_TYPE_BLU_STRING                = 3,  //Block Level UECC on certain String/s
   EI_PROG_BLU_FAILURE_TYPE_SILENT_BLU_EPWR           = 4,  //Silent Block Level UECC discovered during EPWR - therefore it configures EPWR injection with status 3

   EI_NUM_OF_PROG_BLU_FAILURE_TYPES,
} EI_ProgBluFailureType_stub_t;
typedef uint32_t EI_ProgBluFailureType_t;


typedef enum EI_EraseFailureType_e
{
   EI_ERASE_SINGLE = 0,
   EI_ERASE_WRITE_ABORT,   //Write abort (UGSD)
   EI_ERASE_SGD,
   EI_ERASE_SR_PRER, //[MAIA-3556] Erase S/R read with MSMR
   EI_NUM_OF_ERASE_FAILURE_TYPES
} EI_EraseFailureType_stub_t;
typedef uint32_t EI_EraseFailureType_t;

typedef enum EI_NestedProgFailureType_e
{
   EI_NESTED_PROG_INJ_ERASE = 0,
   EI_NESTED_PROG_INJ_PAGE_0,
   EI_NESTED_PROG_INJ_PAGE_1,
   EI_NESTED_PROG_INJ_PAGE_LAST,
   EI_NESTED_PROG_INJ_PAGE_PRE_LAST,
   EI_NESTED_PROG_INJ_PAGE_OTHER,
   EI_NESTED_PROG_INJ_LATCH,
   EI_NESTED_PROG_INJ_RAND_EPWR,

   EI_NUM_OF_NESTED_PROG_FAILURE_TYPES,
} EI_NestedProgFailureType_stub_t;
typedef uint32_t EI_NestedProgFailureType_t;

typedef enum EI_NestedEraseFailureType_s
{
   EI_NESTED_ERASE_INJ_ERASE = 0,
   
   EI_NUM_OF_NESTED_ERASE_FAILURE_TYPES,
} EI_NestedEraseFailureType_stub_t;
typedef uint32_t EI_NestedEraseFailureType_t;

typedef enum EI_CfgDstFailureType_e
{
   EI_CFG_DST_FAILURE_TYPE_LDPC_TIMEOUT = 0,
   EI_NUM_OF_CFG_DST_FAILURE_TYPES
} EI_CfgDstFailureType_t;

typedef enum EI_CfgSgdFailureType_e
{
   EI_CFG_SGD_FAILURE_TYPE_DETECT_DOWNSHIFT_FAIL = 0,   // bit 0
   EI_CFG_SGD_FAILURE_TYPE_SOFT_PROG_FAILURE,           // bit 1
   EI_CFG_SGD_FAILURE_TYPE_EF_AFTER_CORRECT_OK,         // bit 2
   EI_CFG_SGD_FAILURE_TYPE_EF_AFTER_CORRECT_FAIL,       // bit 3
   EI_CFG_SGD_FAILURE_TYPE_EF_BEFORE_SGD,               // bit 4
   EI_CFG_SGD_FAILURE_TYPE_OVER_PROG,                   // bit 5
   EI_CFG_SGD_FAILURE_TYPE_DETECT_UPSHIFT_FAIL,         // bit 6
   EI_CFG_SGD_FAILURE_TYPE_EF_AFTER_UPSHIFT_FAIL,       // bit 7
   EI_NUM_OF_CFG_SGD_FAILURE_TYPES
} EI_CfgSgdFailureType_t;

typedef enum EI_FlashAccessType_e
{
   EI_FLASH_READ = 0,
   EI_FLASH_PROG,

   EI_NUM_OF_OP_TBL_ACCESS_TYPES,
   
   EI_FLASH_ERASE = EI_NUM_OF_OP_TBL_ACCESS_TYPES,
   EI_NUM_OF_STANDARD_FLASH_ACCESS_TYPES,

   EI_FLASH_XOR_REC = EI_NUM_OF_STANDARD_FLASH_ACCESS_TYPES,
   EI_NUM_OF_FLASH_ACCESS_TYPES_FOR_STATISTICS,

   EI_FLASH_DST = EI_NUM_OF_FLASH_ACCESS_TYPES_FOR_STATISTICS,  //Device Self Test
   EI_FLASH_DRD,                                                //Data Retention Detection

   EI_NUM_OF_FLASH_ACCESS_TYPES, // Total number of Flash access types
   
   EI_ILLEGAL_FLASH_ACCESS_TYPE,
} EI_FlashAccessType_stub_t;
typedef uint8_t EI_FlashAccessType_t;

typedef enum EI_OperationToInject_e
{
   EI_INJ_OP_READ = 0,
   EI_INJ_OP_PROG,
   EI_INJ_OP_ERASE,

   EI_INJ_OP_XOR_LOAD,
   EI_INJ_OP_XOR_REBUILD,

   EI_INJ_OP_XOR_REC_LOAD,
   EI_INJ_OP_XOR_REC_UNROLL_PS0,
   EI_INJ_OP_XOR_REC_UNROLL_PS1,

   EI_INJ_OP_DST, //Device self test
   EI_INJ_OP_DRD, //Data retention detection
   EI_INJ_OP_XOR_STORE,

   EI_NUM_OF_INJ_OP_TYPES,
   EI_ILLEGAL_INJ_OP_TYPE
} EI_OperationToInject_stub_t;
typedef uint8_t EI_OpToInject_t;

typedef enum EI_SingleInjAddrType_e
{
   EI_ADDR_TYPE_VBA = 0,
   EI_ADDR_TYPE_DEVBA, //For Infra use only
   EI_ADDR_TYPE_LBA,
   EI_ADDR_TYPE_OPID,

   EI_NUM_OF_ADDR_TYPES, // Total number of address types
} EI_SingleInjAddrType_stub_t;
typedef uint8_t EI_SingleInjAddrType_t;

typedef enum EI_PageFailurePlacement_e
{
   EI_PLACEMENT_PAGE_0,
   EI_PLACEMENT_PAGE_1,
   EI_PLACEMENT_PAGE_LAST,
   EI_PLACEMENT_PAGE_PRE_LAST,
   EI_PLACEMENT_PAGE_OTHER,

   EI_PLACEMENT_PAGE_TOTAL,  // Total number of page placements
   EI_PLACEMENT_PAGE_ILLEGAL

} EI_PageFailurePlacement_stub_t;
typedef uint32_t EI_PageFailurePlacement_t;

typedef enum EI_ZoneState_e
{
   EI_ZONE_STATE_OPEN_ZONE_OPEN_BLOCK,
   EI_ZONE_STATE_CLOSED_ZONE_OPEN_BLOCK,
   EI_ZONE_STATE_CLOSED_ZONE_CLOSED_BLOCK,
   EI_ZONE_STATE_TOTAL,
   EI_ZONE_STATE_ILLEGAL,
   EI_ZONE_STATE_WORKAROUND_RLC_HOST,
} EI_ZoneState_stub_t;
typedef uint32_t EI_ZoneState_t;

typedef enum EI_NestedFailueChances_e
{
   EI_NESTED_PF_EF_DURING_PF,
   EI_NESTED_READ_DURING_PF,
   EI_NESTED_TOTAL,
} EI_NestedFailueChances_stub_t;
typedef uint32_t EI_NestedFailueChances_t;

typedef enum EI_BlockType_e
{
   EI_BLOCK_TYPE_SLC = 0,
   EI_BLOCK_TYPE_MLC,
   EI_BLOCK_TYPE_HYBRID,
   EI_BLOCK_TYPE_IRJB,

   EI_NUM_OF_BLOCK_TYPES // Total number of block types
} EI_BlockType_stub_t;
typedef uint32_t EI_BlockType_t;

typedef enum EI_OperationalState_e
{
   EI_OPERATIONAL_STATE_SPECIAL = 0,
   EI_OPERATIONAL_STATE_MOUNT,
   EI_OPERATIONAL_STATE_MOUNT_DPS,
   EI_OPERATIONAL_STATE_NORMAL,
   EI_OPERATIONAL_STATE_TOTAL,
} EI_OperationalState_stub_t;
typedef uint8_t EI_OperationalState_t;

typedef enum EI_SpecialOpst_e
{
   EI_SPECIAL_OPST_FNVM_OBM_CLOSE_OPNBLKS = 0x1,
   EI_SPECIAL_OPST_FNVM_OTHER_CASES       = 0x2,
   EI_SPECIAL_OPST_BKOPS                  = 0x4,
   EI_SPECIAL_OPST_PLP                    = 0x8,
   // Add new special OPST here

   EI_SPECIAL_OPST_ALL                    = 0xFF,
} EI_SpecialOpst_stub_t;
typedef uint8_t EI_SpecialOpst_t;

typedef enum EI_LogCopyLocation_e
{
   EI_LOG_COPY_0_BEGINNING = 0,
   EI_LOG_COPY_0_MIDDLE,
   EI_LOG_COPY_0_ENDING,
   EI_LOG_COPY_1_BEGINNING,
   EI_LOG_COPY_1_MIDDLE,
   EI_LOG_COPY_1_ENDING,
   EI_LOG_COPY_LOCATION_TOTAL,
} EI_LogCopyLocation_stub_t;
typedef uint8_t EI_LogCopyLocation_t;

typedef enum
{
   EI_LOG_WRITE_REASON_FIRST_MOUNT = 0,
   EI_LOG_WRITE_REASON_MOUNT_GSD_DONE,
   EI_LOG_WRITE_REASON_MOUNT_UGSD_DONE,
   EI_LOG_WRITE_REASON_GSD,
   EI_LOG_WRITE_REASON_BLOCK_ALLOCATED,
   EI_LOG_WRITE_REASON_FLASH_FILL,
   EI_LOG_WRITE_REASON_PWS,
   EI_LOG_WRITE_REASON_FNVM,
   EI_LOG_WRITE_REASON_DECOMMISSIONED,
   EI_LOG_WRITE_REASON_FE_SMART,
   EI_LOG_WRITE_REASON_INFRA_SMART,
   EI_LOG_WRITE_REASON_FTL_SMART,
   EI_LOG_WRITE_REASON_BRLC_BLOCK_ALLOC_FAILURE,
   EI_LOG_WRITE_REASON_BRLC_COMPLETE,
   EI_LOG_WRITE_REASON_FMBL_NOT_FULL,
   EI_LOG_WRITE_REASON_READONLY,
   EI_LOG_WRITE_REASON_TOTAL,
} EI_LogWriteReason_stub_t;
typedef uint8_t EI_LogWriteReason_t;

typedef enum EI_XorLoadType_e
{
   EI_XOR_LOAD_TYPE_XRAM = 0,   // the regular XOR parity stored in XRAM
   EI_XOR_LOAD_TYPE_DRAM,       // block level XOR parity stored in DRAM
   EI_XOR_LOAD_TYPE_TOTAL,
} EI_XorLoadType_stub_t;
typedef uint8_t EI_XorLoadType_t;

typedef enum EI_RebuildTarget_e
{
   EI_REBUILD_TARGET_XRAM = 0,  // the regular XOR parity stored in XRAM
   EI_REBUILD_TARGET_DRAM,      // block level XOR parity stored in DRAM
   EI_REBUILD_TARGET_TOTAL,
} EI_RebuildTarget_stub_t;
typedef uint8_t EI_RebuildTarget_t;

typedef enum EI_RebuildReason_e
{
   EI_REBUILD_REASON_UGSD = 0,
   EI_REBUILD_REASON_LOAD_FAILURE,
   EI_REBUILD_REASON_TOTAL,
} EI_RebuildReason_stub_t;
typedef uint8_t EI_RebuildReason_t;

typedef enum EI_RlcType_e
{
   EI_RLC_TYPE_OTHER = 0,  // for dynamic, blind, SLC static:  same chance used 
   EI_RLC_TYPE_STATIC,     // was needed to avoid in TLC static 
   EI_RLC_TYPE_TOTAL,
} EI_RlcType_stub_t;
typedef uint8_t EI_RlcType_t;

typedef enum EI_RlcOperation_e
{
   EI_RLC_OPERATION_BOTH = 0,             // any RLC will be okay
   EI_RLC_OPERATION_NORMAL_ONLY,          // Normal RLC only case
   EI_RLC_OPERATION_INTERLEAVED_ONLY,     // Interleaved RLC only case 
   EI_RLC_OPERATION_TOTAL,
} EI_RlcOperation_stub_t;
typedef uint8_t EI_RlcOperation_t;

typedef enum EI_FadiType_e
{
   EI_FADI_TYPE_OTHER = 0,
   EI_FADI_TYPE_JTAG,
   EI_FADI_TYPE_TOTAL,
} EI_FadiType_stub_t;
typedef uint8_t EI_FadiType_t;

typedef enum EI_HostlessType_e
{
   EI_HOSTLESS_TYPE_OTHER = 0,
   EI_HOSTLESS_TYPE_RPMB,
   EI_HOSTLESS_TYPE_BOOTPARTITION,
   EI_HOSTLESS_TYPE_TOTAL,
} EI_HostlessType_stub_t;
typedef uint8_t EI_HostlessType_t;

enum EI_eraseType_e
{
   EI_ERASE_TYPE_NORMAL,
   EI_ERASE_TYPE_FNVM,
   EI_ERASE_TYPE_PRE_ERASE,
   EI_ERASE_TYPE_RE_ERASE,
   EI_ERASE_TYPE_COUNT,
   EI_ERASE_TYPE_INVALID = 0xFF
};
typedef uint8_t EI_eraseType_t;


///-----------------------------------------------------------------------------
// This enumeration describes a read failure level, i.e. until which read attempt should PS fail the read.
typedef enum EI_ReadFailureLevel_e
{
   EI_READ_FAILURE_LEVEL_NONE                    = 0, //Means that read should not fail
   EI_READ_FAILURE_LEVEL_FIRST_FAIL              = 1,
   EI_READ_FAILURE_LEVEL_SW_TH                   = EI_READ_FAILURE_LEVEL_FIRST_FAIL, //Means that read should fail until Syndrome Weight above Threshold
   EI_READ_FAILURE_LEVEL_BES5_SB0                = 2, //Means that read should fail until SB0 attempted
   EI_READ_FAILURE_LEVEL_BES5_SB1                = 3, //Means that read should fail until SB1 attempted
   EI_READ_FAILURE_LEVEL_BES7_SB2_NOLA           = 4, //Means that read should fail until SB2 attempted
   EI_READ_FAILURE_LEVEL_BES7_SB1                = 5, //Means that read should fail until BES7 SB1 attempted
   EI_READ_FAILURE_LEVEL_BES7_SB0                = 6, //Means that read should fail until BES7 SB0 attempted
   EI_READ_FAILURE_LEVEL_UECC                    = 7, //Means that read should always fail
   EI_READ_FAILURE_LEVEL_LDPC_RAM_BIT_FLIP       = 8, //LDPC RAM Bit Flip read failure, This failure level can't be mixed with others in the same PS request
   EI_READ_FAILURE_LEVEL_BES7_SB2_LA             = 9, //Means that read should fail until BES7 SB2 attempted
   EI_READ_FAILURE_LEVEL_TOTAL                   = 10 //Total amount of levels
} EI_ReadFailureLevel_t;

// This enumeration describes different program failure disturb patterns. This is the general idea and is subject to changes according to memory behavior.
typedef enum EI_ProgFailurePattern_e
{
   EI_PROG_FAILURE_PATTERN_1WL             = 0,
   EI_PROG_FAILURE_PATTERN_WL2WL           = 1,
   EI_PROG_FAILURE_PATTERN_LWL2LWL         = 2,
   EI_PROG_FAILURE_PATTERN_1WL_2PLANES     = 3,
   EI_PROG_FAILURE_PATTERN_2PLANES_WL2WL   = 4,
   EI_PROG_FAILURE_PATTERN_WL2WL_P2P       = 5,
   EI_PROG_FAILURE_PATTERN_WRITE_ABORT     = 6, //Write abort
   EI_PROG_FAILURE_PATTERN_EPWR            = 7, //EPWR
   EI_PROG_FAILURE_PATTERN_1LWL            = 8,
   //Athena-Calypso EI merge notes: EI_PROG_FAILURE_PATTERN_SKIP_PAR_PAGES was 11 in Athena
   EI_PROG_FAILURE_PATTERN_SKIP_PAR_PAGES  = 9, //Do not write parity pages when finishing the zone.  Use to emulate UGSD in PS UT only
   EI_PROG_FAILURE_PATTERN_EPWR_BES5       = 10,  //Placeholder until Hermes II changes are merged
   EI_PROG_FAILURE_PATTERN_EPWR_BES7       = 11, //Placeholder until Hermes II changes are merged


   // BLU Failure Patterns
   EI_PROG_FAILURE_PATTERN_BLU             = 12, //Block Level UECC
   EI_PROG_FAILURE_PATTERN_BLU_2PLANES     = 13, //Block Level UECC + 1WL on other plane
   EI_PROG_FAILURE_PATTERN_BLU_P2P         = 14, //Block Level UECC + UECC on other plane
   EI_PROG_FAILURE_PATTERN_BLU_STRING      = 15, //Block Level UECC on certain String/s
   EI_PROG_FAILURE_PATTERN_SILENT_BLU_EPWR = 16, //Silent Block Level UECC discovered during EPWR - therefore it configures EPWR injection with status 3
//Athena-Calypso EI merge notes: EI_PROG_FAILURE_PATTERN_TOTAL jumped from 10 to 17
   EI_PROG_FAILURE_PATTERN_TOTAL           = 17  //Total amount of patterns
} EI_ProgFailurePattern_t;

typedef enum EI_EraseFailurePattern_e
{
   EI_ERASE_FAILURE_PATTERN_SIMPLE      = 0, //A simple single string failure
   EI_ERASE_FAILURE_PATTERN_WRITE_ABORT = 1, //Write abort
   EI_ERASE_FAILURE_PATTERN_SGD_ONLY    = 2, //No erase failure, just SGD detection
   EI_ERASE_FAILURE_PATTERN_SUSRES_PRER = 3, //suspend/resume erase by read or force pre-erase with/without EF - MAIA-3556 Erase S/R read with MSMR
   EI_ERASE_FAILURE_PATTERN_TOTAL            //Total amount of patterns
} EI_EraseFailurePattern_t;

typedef struct EI_InjXor_s
{  // this is a generic EI data structure for XOR
   // FTL responsible to not inject here in case of load from RAM.
   uint8_t readInjBytemap[EI_FMUS_PER_XOR_PAGE];   // A byte map of FMUs in the read operation to inject. derived from EI_ReadFailureLevel_t. Relevant for read operation only.
                                                   // This readInjBytemap is valid for: 
                                                   //       - Start XOR Recocvery Request
                                                   //       - Get Parity Page for Recovery Request
                                                   //       - Rebuild XOR Request
                                                   //       - Load XOR Request
   uint16_t offsetInBin;                           // Indicates the relative offset in bin on which read should fail with the failure mode specified in readinjBytemap. 
                                                   // The "offset in bin" is the offset within a PS
                                                   // This offsetInBin is valid for: 
                                                   //       - Start XOR Recocvery Request
                                                   //       - Rebuild XOR Request
                                                   //       - Load XOR Request: - this is a special case
                                                   //                           - the Load XOR always loads 128K or 8 plane-pages from the XOR block
                                                   //                           - the offsetInBin, in this case should be 0-7 (for any correspodning bin in offset)
                                                   //                      
   uint8_t binNumber;                              // Which bin we need to inject the errors above
                                                   // This binNumber is valid for: 
                                                   //       - Rebuild XOR Request
   uint8_t secInjFlag;
} EI_InjXor_t;



typedef struct EI_InjDescXor_s
{  // this is EI for XOR Recovery
   EI_InjXor_t    ei[EI_MAX_EI_PER_VECTOR];        // each request can have up to 2 EIs
                                                   // The following requests could have up to EI_MAX_EI_PER_VECTOR:
                                                   // - XOR Recovery Start
                                                   // - XOR Rebuild
                                                   // - XOR Load
                                                   // The following could have ONLY 1 error injection, and we will use the first in the table
                                                   // - Get Parity Page for Recovery Request
   uint32_t       size;                            // number of valid ei
   uint8_t        rfu[80 - 4 - sizeof(EI_InjXor_t) * EI_MAX_EI_PER_VECTOR]; // adding RFU so that the total size will be EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX
} EI_InjDescXor_t;

typedef struct EI_InjDescDst_s
{
   uint8_t  failureType[EI_MAX_DIES_PER_PS];       //derived from  EI_DstFailureType_t
   uint8_t  rfu[80 - EI_MAX_DIES_PER_PS];          // adding RFU so that the total size will be EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX
} EI_InjDescDst_t;



///-----------------------------------------------------------------------------
///     INTERNAL STRUCTURES
///-----------------------------------------------------------------------------

typedef struct EI_SingleReadConfigParams_s
{
   EI_ReadFailureLevel_t   failureType;                              // configures the type of read error per single FMU (this field was replaced by failurePerFmu array)
   uint8_t                 triggerRlc;
   uint8_t                 rfu1[3];
   uint8_t                 failurePerFmuMap[EI_MAX_FMUS_PER_PS_REQ]; // configures the type of read error to generate per each FMU in page
   uint8_t                 rfu[EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX - 24];
} EI_SingleReadConfigParams_t;

typedef struct EI_SingleProgConfigParams_s
{
   EI_ProgFailurePattern_t failureType;

   uint16_t nestedInjReadPagesList[EI_MAX_NESTED_READ_INJ];
   uint8_t  nestedInjReadByteMapsList[EI_MAX_NESTED_READ_INJ][EI_FMUS_PER_PHYS_PAGE];

   EI_EPRCodeMapType_t nestedInjProgEraseList[EI_MAX_ALLOWED_SEC_INJ]; // List of nested prog/erase injections, values are page # or (-1) for erase
   uint8_t  nestedListProgEraseSize;                        // Sizes of the nested prog/erase injection lists.
   uint8_t  nestedListReadSize;                             // Sizes of the nested read injection lists.

   uint8_t  doInjectMultiplane;                             // Indicates that the injection should be multiplane
   uint8_t  isParityPage;
   EI_EPRCodeMapType_t EPWRbitmap;
   uint8_t  XORStoreMetaPgOffs;                             // for PF case during XOR store only.  Inject on the metaPage offset indicated here. (store commands all start on Fim0 plane0, so we can offset from there.)
                                                            // XORStoreMetaPgOffs can have 0 ~ 9 for SLC, 0 ~ 29 for TLC at BiCS6
   uint8_t  rfu[EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX - 77];
} EI_SingleProgConfigParams_t;

typedef struct EI_SingleSgdBitmapParams_s
{
   uint32_t  lowTailSgdDetectFailStringInWLBitmap;

   uint16_t  lowTailSgdDetectFailWLBitmap;         //lowTail detect fail WL has to be enabled in lowTailSgdDetectWLBitMap
   uint8_t   lowTailSgdCorrectFailWL;              //correct fail WL has to be detect-fail WL in above bitmap
   uint8_t   lowTailSgdCorrectFailStringInWL;      //can be any string in correct fail WL

   uint8_t   lowTailSgdOverProgramWL;              //over program WL has to be detect-fail WL in above bitmap
   uint8_t   lowTailSgdOverProgramStringInWL;      //can be any string in over program WL
   uint8_t   upTailSgdDetectFailWL;                //upTail detect fail WL has to be enable in upTailSgdDetectWLBitMap
   uint8_t   upTailSgdDetectFailStringInWL;        //can be any string in uptail detect fail WL
} EI_SingleSgdBitmapParam_t;

typedef struct EI_SingleEraseConfigParams_s
{
   EI_EraseFailurePattern_t   failureType;
   uint8_t                    numNestedEraseInjections;           // Number of nested erase failures, up to EI_MAX_ALLOWD_SEC_INJ
   uint8_t                    doInjectMultiplane;                 // Indicates that the injection should be multiplane
   uint8_t                    doPowerAbortOnLastNestedInjection;
   uint8_t                    sgdHeaderErase[PLANES_PER_DIE];     // SGD header, value is taken from enum: EI_SGD_TYPE_e (all combinations are allowed)
   uint8_t                    forcePreErase;                      // MAIA-9749: Adapt force pre-erase feature
   uint16_t                   lowTailSgdDetectWLBitmap;           //this bitmap has to match MORPHEUS, cannot be randomly defined
   uint16_t                   upTailSgdDetectWLBitmap;            //this bitmap has to match MORPHEUS, cannot be randomly defined
   EI_SingleSgdBitmapParam_t  sgdEIBitmap[PLANES_PER_DIE];
#if defined(PROD_PS_ERASE_SUSPEND_READ)
   uint8_t                    numPreEraseSuspendRead;
   uint8_t                    numPostEraseSuspendRead;
   uint8_t                    forceSuspendEraseFail:1;
   uint8_t                    disablePreEraseTimeLimit:1;
   uint8_t                    disablePostEraseTimeLimit:1;
   uint8_t                    reserved:5;

   uint8_t                    REHTypeAtESR:7;                     // Bit 0~6 : Based on EI_ReadFailureLevel_t: 2 - SB0, 3 - SB1, 4 - SB2, 5 - SB2+DLA, 6 - CLIP, 7 - UECC, 8 - LDPC RAM bit flip
   uint8_t                    REHonIsSuspend:1;                 // Bit   7 : REH will occurre at which ESR step (0 -  PS_DGM_DIE_ERASE_STATE_SERVICE_PRIOR_READS, 1 - PS_DGM_DIE_ERASE_STATE_TO_SUSPEND)
   uint8_t                    ReadIndexAtESR;                     // Which read will occurr the REH (ex 1st read or 2nd read), it should be less then "numPostEraseSuspendRead"
   uint8_t                    rfu1[11];
#else
   uint8_t                    rfu1[16];                            //pad for memory align
#endif
} EI_SingleEraseConfigParams_t;

// Restrictions (gaps) in global configurations table
typedef struct EI_RestrictionsConfig_s
{
   uint32_t cooldownTime;                                // Min time to wait before it can be injected again (uSec)
   uint32_t consecutiveGap;                              // Min operation iterations before another injection is possibla
   uint8_t  fullCS;                                      // Indicates that a full control sync needs to occur between injections
   uint8_t  multyDiesPfEnable;                           // Enable multi-die injections 
   uint8_t  avoidRoOnMtmUecc;                            // Avoid RO when MTM hits UECC
   uint8_t  multiPlanePfDisable;                         // Enable multi-plane(multiDiesPfEnable must be false) on PF injection
   uint8_t  maxNumberOfNestedRecoverableReadsDuringPf;   // Max number of nested read injections that trigger XOR during PF
   uint8_t  rfu [3];
} EI_RestrictionsConfig_t;

// General configuration section in the gloabl configurations table
typedef struct EI_GeneralConfig_s
{
   EI_ConfType_t confType;
   uint32_t      randomSeed;
   uint8_t       errLogEnable;
   uint8_t       mbReviveEnableFull; //FTL + PS revive
   uint8_t       jbReviveEnable;
   uint8_t       doSurvivePowerCycles;
   uint16_t      drdOpbBitmap;
   uint8_t       mbReviveDisableFtl;   
   uint8_t       planeAffinityMask[EI_MAX_NUM_PS][MAX_DIES_PER_FIM];  //Only valid for random configuration mode
                 // Bitmask mapping is as followes:
                 // -----------------------------------------
                 // |       Fim-1       |       Fim-0       |
                 // -----------------------------------------
                 // |P-3 |P-2 |P-1 |P-0 |P-3 |P-2 |P-1 |P-0 |
                 // -----------------------------------------
                 //   7    6    5    4    3    2    1    0   
                 //  MSB                                LSB
   uint8_t       specialOpstBitmap;
} EI_GeneralConfig_t;


///-----------------------------------------------------------------------------
///     INTERNAL UNIONS
///-----------------------------------------------------------------------------

typedef union EI_OpidSpecificChancesTable_u
{
   uint8_t                      flat[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE];

   struct
   {
      EI_Percent_t              locations[EI_LOG_COPY_LOCATION_TOTAL];
      EI_Percent_t              reasons[EI_LOG_WRITE_REASON_TOTAL];
      EI_Percent_t              rfu[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_LOG_COPY_LOCATION_TOTAL - EI_LOG_WRITE_REASON_TOTAL];
   } logChances;

   struct
   {
      EI_Percent_t              xorLoadTypes[EI_XOR_LOAD_TYPE_TOTAL];
      EI_Percent_t              rfu[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_XOR_LOAD_TYPE_TOTAL];
   } xorChances;

   struct
   {
      EI_Percent_t              rebuildTargets[EI_REBUILD_TARGET_TOTAL];
      EI_Percent_t              rfu0[EI_OPID_TABLE_MAX_ENTRIES - EI_REBUILD_TARGET_TOTAL];
      EI_Percent_t              rebuildReasons[EI_REBUILD_REASON_TOTAL];
      EI_Percent_t              rfu1[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_OPID_TABLE_MAX_ENTRIES - EI_REBUILD_REASON_TOTAL];
   } rebuildChances;

   struct
   {
      EI_Percent_t              rlcTypes[EI_RLC_TYPE_TOTAL];
      EI_Percent_t              rlcOperation;  //MAIA-9027: Adapt IRLC type for checking injection condition
      EI_Percent_t              rfu[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_RLC_TYPE_TOTAL - sizeof(EI_Percent_t)];
   } rlcChances;

   struct
   {
      EI_Percent_t              fadiTypes[EI_FADI_TYPE_TOTAL];
      EI_Percent_t              rfu[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_FADI_TYPE_TOTAL];
   } fadiChances;

   struct
   {
      EI_Percent_t              hostlessTypes[EI_HOSTLESS_TYPE_TOTAL];
      EI_Percent_t              rfu[EI_OPID_SPECIFIC_CHANCES_TABLE_SIZE - EI_HOSTLESS_TYPE_TOTAL];
   } hostlessChances;

} EI_OpidSpecificChancesTable_t;

typedef struct EI_InjDescDrd_s
{
   uint16_t  opbBitmap;                            // Fixed bitmap of OPBs that will return DRI when they are checked
   uint8_t   rfu[78];                              // adding RFU so that the total size will be EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX
} EI_InjDescDrd_t;

typedef union EI_SingleConfigParams_u
{
   EI_SingleReadConfigParams_t  read;
   EI_SingleProgConfigParams_t  prog;
   EI_SingleEraseConfigParams_t erase;
   EI_InjDescXor_t              xor_rec;
   EI_InjDescDst_t              dst; 
   EI_InjDescDrd_t              drd;
   uint8_t                      bytes[EI_SPECIFIC_SINGLE_CONFIG_BYTE_MAX];
} EI_SingleConfigParams_t;

///-----------------------------------------------------------------------------
///     EI CONFIGURATION SUB-STRUCTURES
///-----------------------------------------------------------------------------

typedef struct EI_OpTableEntry_s
{
   uint8_t                       valid;
   uint8_t                       isParticipantInBurst;
   EI_Percent_t                  chanceToInitiateBurstOnRead;
   EI_Percent_t                  chanceToInitiateBurstOnProg;
   EI_Percent_t                  operationalStateChances  [EI_OPERATIONAL_STATE_TOTAL]; // Table of values from EI_OperationalState_t
   EI_Percent_t                  opTypeFailureChances     [EI_OPID_TABLE_MAX_ENTRIES];  // Table of values from EI_FlashAccessType_t
   EI_Percent_t                  blockTypeFailureChances  [EI_OPID_TABLE_MAX_ENTRIES];  // Table of values from EI_BlockType_t
   EI_Percent_t                  pagePlacementChances     [EI_OPID_TABLE_MAX_ENTRIES];  // Table of values from EI_PageFailurePlacement_t
   EI_Percent_t                  zoneStateChances         [EI_OPID_TABLE_MAX_ENTRIES];  // Table of values from EI_ZoneState_t
   EI_OpidSpecificChancesTable_t opidSpecificChances;
   EI_Percent_t                  nestedFailuresChances    [EI_OPID_TABLE_MAX_ENTRIES];  // Table of values from EI_NestedFailueChances_t
} EI_OpTableEntry_t;

typedef struct EI_ChancesTable_s
{
   EI_Percent_t              failChanceOriginalRead   [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_ReadFailureType_t
   EI_Percent_t              failChanceXorRecRead     [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_ReadFailureType_t
   EI_Percent_t              failChancePfRecRead      [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_ReadFailureType_t
   EI_Percent_t              failChanceOriginalProg   [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_ProgFailureType_t
   EI_Percent_t              failChanceNestedProg     [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_NestedProgFailureType_t
   EI_Percent_t              failChanceOriginalErase  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_EraseFailureType_t
   EI_Percent_t              failChanceNestedErase    [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_NestedEraseFailureType_t
   EI_Percent_t              failChanceDst            [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_CfgDstFailureType_t
   EI_Percent_t              failChanceSgd            [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_CfgSgdFailureType_t
   EI_Percent_t              failChanceProgBlu        [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];   // Table of values from EI_ProgBluFailureType_t
   EI_Percent_t              rfuTbl3                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
   EI_Percent_t              rfuTbl4                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
   EI_Percent_t              rfuTbl5                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
   EI_Percent_t              rfuTbl6                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
   EI_Percent_t              rfuTbl7                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
   EI_Percent_t              rfuTbl8                  [EI_CHANCES_TABLE_TYPE_MAX_ENTRIES];
} EI_ChancesTable_t;

typedef struct EI_AllOpsFailureChances_s
{
   EI_Percent_t              chanceEraseFailure;
   EI_Percent_t              chanceXorRecDuringPfRecFailure;
   EI_Percent_t              chanceNestedEraseFailureDuringEraseFailure;
   EI_Percent_t              chancePsDecommissionRequests;
   EI_Percent_t              chanceProgParityPage; //Only relevant for Prog operations (XOR protected)
   EI_Percent_t              chanceDstFailure;
   EI_Percent_t              chanceUnrecXorRecRead;
   EI_Percent_t              chanceToTriggerRlcAfterReadInj;
   EI_Percent_t              chanceNestedWriteAbort;
   EI_Percent_t              chanceNestedEraseAbort;
   EI_Percent_t              chanceEraseFailureDuringFnvm;
   EI_Percent_t              chanceDrd;
   EI_Percent_t              chanceSgdDownShift;
   EI_Percent_t              chanceSgdUpShift;
   EI_Percent_t              chanceReadDuringSuspendErase;
   EI_Percent_t              chanceEraseFailureFadiJtag;
} EI_AllOpsFailureChances_t;

typedef struct EI_BurstModeParameters_s
{
   uint32_t                  cooldownTimeBurst;       // Min time to wait before it can be injected again (uSec)
   uint32_t                  consecutiveGapBurst;     // Min operation iterations before another injection is possibla
   uint8_t                   maxInjectionsPerBurst;
   uint8_t                   minInjectionsPerBurst;
   uint8_t                   isBackToBack;
   uint8_t                   chanceToInitiateBurstOnErase;
   uint8_t                   chanceEraseFailureDuringBurst;
   uint8_t                   isSolePfDuringBurst;
   uint8_t                   rfu[2];
} EI_BurstModeParameters_t;

typedef struct EI_EraseChances_s
{
   EI_Percent_t eraseFailureChances[EI_ERASE_TYPE_COUNT];           // Table of values from EI_eraseType_t
   uint8_t rfu[4];
   EI_Percent_t blockTypeFailureChances[EI_OPID_TABLE_MAX_ENTRIES]; // Table of values from EI_BlockType_t
} EI_EraseChances_t;

typedef struct EI_PlpModeParameters_s
{
   uint16_t                   plpFirstTriggerId;         // First trigger ID to inject
   uint16_t                   plpSecondTriggerId;        // Second trigger ID to inject
   EI_Percent_t               plpChance;                 // Chance to inject PLP action
   uint8_t                    isPlpActionDpa;            // Which action is injected : DPA or Infra simulation
   uint8_t                    isPlpUsingWaitingLoop;     // Flag to indicate whether to wait for PLP start in waiting loop
   uint8_t                    isPlpAllowedDuringMount;   // Flag to indicate whether to allow PLP injections during mount
   uint16_t                   plpMaxDelayInMilliSeconds; // PLP will occur after a random time between 1 and the delay value
   uint8_t                    isPfEfAllowedDuringPlp;    // Flag to indicate whether to allow PF/EF injections during PLP
   uint8_t                    rfu[1];
} EI_PlpModeParameters_t;

typedef struct EI_ConfigArraysExtenstion_s
{
   uint8_t                   rfuBuff[24];
   EI_PlpModeParameters_t    plpModeParameters;
   EI_Percent_t              chanceCapTestFailure;
   EI_Percent_t              chancePlpProcessFailure;
   EI_EraseChances_t         eraseChances;
   uint8_t                   eiEnConfigFlowID[EI_CONFIG_FLOWS_ARR_SZ];  //byte array of flows, each bit represent flow ID to enable EI_Config_t
   uint8_t                   rfuBuffer[226- EI_CONFIG_FLOWS_ARR_SZ];
} EI_ConfigArraysExtenstion_t;


typedef struct EI_SingleInjTableEntry_s
{
   uint64_t                  addr;
   uint8_t                   addrType;     // Derived from: EI_SingleInjAddrType_t
   uint8_t                   nsId;         // Namespace ID
   uint8_t                   opToInject;   // Derived from: EI_OpToInject_t
   uint8_t                   isValid;
   uint8_t                   nthOpCounter; // "N" Counter to delay injection until the "Nth" instance of an operation whose address is specified with "Op-ID"  or with "useAnyAddress" (0xfffffffe)
   uint8_t                   rfu[3];

   EI_SingleConfigParams_t   injParams;
} EI_SingleInjTableEntry_t;


typedef struct EI_MetaDieDecommissionTable_s
{
   uint32_t                   currentTableSize;
   uint16_t                   metaBlockTable[EI_MAX_BAD_BLOCKS_PER_PLANE_EOL];
} EI_MetaDieDecommissionTable_t;

typedef struct EI_revive_s
{
   EI_MetaDieDecommissionTable_t  metaDieDecommissionTable[BML_MAX_NUM_METADIES];

} EI_Revive_t;

///-----------------------------------------------------------------------------
///     FINAL CONFIGURATION STRUCTURE
///-----------------------------------------------------------------------------

///-----------------------------------------------------------------------------
/// Full EI (Error Injection) configuration structure
///-----------------------------------------------------------------------------
typedef struct EI_Config_s
{
   // Header
   uint32_t                      canaryValStart;
   uint32_t                      version;
   uint8_t                       rfu0 [8];

   // Global configuration
   EI_GeneralConfig_t            generalConfig;                               //start: 0x10  size: 0x20
 
   // Random injection configuration
   EI_RestrictionsConfig_t       restrictions;                                //start: 0x30  size: 0x10
   EI_AllOpsFailureChances_t     allOpsChance;                                //start: 0x40  size: 0x10
   EI_ChancesTable_t             chances;                                     //start: 0x50  size: 0x100
   EI_OpTableEntry_t             operations[EI_OP_IDS_TABLE_MAX_ENTRIES];     //start: 0x150 size: 72*40 = 0xb40

   // Single injection
   EI_SingleInjTableEntry_t      singleInjections[EI_SINGLE_INJ_TABLE_SIZE];  //start: 0xc90 size: 96*6 = 0x240

   // More random injection configuration
   EI_BurstModeParameters_t      burstMode;                                   //start: 0xed0 size: 0x10
   EI_ConfigArraysExtenstion_t   configArraysExtenstion;                      //start: 0xee0 size: 0x118

   //rfu buffer is in configArraysExtenstion

   // Footer
   uint8_t                       rfu1[3];                                     //start: 0xff8
   uint8_t                       isConfigValid;
   uint32_t                      canaryValEnd;
} EI_Config_t;


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t filesize = sb.st_size;
    if (filesize < sizeof(EI_Config_t)) {
        fprintf(stderr, "File size is smaller than ee structure\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    EI_Config_t *mapped = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error mmapping the file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    printf("canaryStart : 0x%x\n", mapped->canaryValStart);
    printf("Version : 0x%x\n", mapped->version);

    if (munmap(mapped, filesize) == -1) {
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }

    return 0;
}

