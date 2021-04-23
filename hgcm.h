#ifndef HGM_HELPER_H
#define HGM_HELPER_H

#define VBGLREQHDR_VERSION 0x10001
#define VBGLREQHDR_TYPE_DEFAULT 0
#define VERR_INTERNAL_ERROR -225

#define VBGL_IOCTL_CODE_SIZE(func, size) (0xc0005600 + (size<<16) + func)

#define VBGL_IOCTL_HGCM_CONNECT                    VBGL_IOCTL_CODE_SIZE(4, VBGL_IOCTL_HGCM_CONNECT_SIZE)
#define VBGL_IOCTL_HGCM_CONNECT_SIZE               sizeof(VBGLIOCHGCMCONNECT)

# define VBGL_IOCTL_HGCM_DISCONNECT                 VBGL_IOCTL_CODE_SIZE(5, VBGL_IOCTL_HGCM_DISCONNECT_SIZE)
# define VBGL_IOCTL_HGCM_DISCONNECT_SIZE            sizeof(VBGLIOCHGCMDISCONNECT)

#define IOCTL_HGCM_CALL 7

/** Guest Physical Memory Address; limited to 64 bits.*/
typedef uint64_t                RTGCPHYS64;
/** Unsigned integer which can contain a 64 bits GC pointer. */
typedef uint64_t                RTGCUINTPTR64;
/** Guest context pointer, 64 bits.
 */
typedef RTGCUINTPTR64           RTGCPTR64;

typedef uint8_t bool;


typedef struct VBGLREQHDR
{
    /** IN: The request input size, and output size if cbOut is zero.
     * @sa VMMDevRequestHeader::size  */
    uint32_t        cbIn;
    /** IN: Structure version (VBGLREQHDR_VERSION)
     * @sa VMMDevRequestHeader::version */
    uint32_t        uVersion;
    /** IN: The VMMDev request type, set to VBGLREQHDR_TYPE_DEFAULT unless this is a
     * kind of VMMDev request.
     * @sa VMMDevRequestType, VMMDevRequestHeader::requestType */
    uint32_t        uType;
    /** OUT: The VBox status code of the operation, out direction only. */
    int32_t         rc;
    /** IN: The output size.  This is optional - set to zero to use cbIn as the
     * output size. */
    uint32_t        cbOut;
    /** Reserved / filled in by kernel, MBZ.
     * @sa VMMDevRequestHeader::fRequestor */
    uint32_t        uReserved;
} VBGLREQHDR;

/**
 * HGCM host service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct
{
    char achName[128]; /**< This is really szName. */
} HGCMServiceLocationHost;

typedef enum
{
    VMMDevHGCMLoc_Invalid    = 0,
    VMMDevHGCMLoc_LocalHost  = 1,
    VMMDevHGCMLoc_LocalHost_Existing = 2,
    VMMDevHGCMLoc_SizeHack   = 0x7fffffff
} HGCMServiceLocationType;

/**
 * HGCM service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct HGCMSERVICELOCATION
{
    /** Type of the location. */
    HGCMServiceLocationType type;

    union
    {
        HGCMServiceLocationHost host;
    } u;
} HGCMServiceLocation;

typedef struct VBGLIOCHGCMCONNECT
{
    /** The header. */
    VBGLREQHDR                  Hdr;
    union
    {
        struct
        {
            HGCMServiceLocation Loc;
        } In;
        struct
        {
            uint32_t            idClient;
        } Out;
    } u;
} VBGLIOCHGCMCONNECT;

/**
 * For VBGL_IOCTL_HGCM_CALL and VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA.
 *
 * @note This is used by alot of HGCM call structures.
 */
typedef struct VBGLIOCHGCMCALL
{
    /** Common header. */
    VBGLREQHDR  Hdr;
    /** Input: The id of the caller. */
    uint32_t    u32ClientID;
    /** Input: Function number. */
    uint32_t    u32Function;
    /** Input: How long to wait (milliseconds) for completion before cancelling the
     * call.  This is ignored if not a VBGL_IOCTL_HGCM_CALL_TIMED or
     * VBGL_IOCTL_HGCM_CALL_TIMED_32 request. */
    uint32_t    cMsTimeout;
    /** Input: Whether a timed call is interruptible (ring-0 only).  This is ignored
     * if not a VBGL_IOCTL_HGCM_CALL_TIMED or VBGL_IOCTL_HGCM_CALL_TIMED_32
     * request, or if made from user land. */
    bool        fInterruptible;
    /** Explicit padding, MBZ. */
    uint8_t     bReserved;
    /** Input: How many parameters following this structure.
     *
     * The parameters are either HGCMFunctionParameter64 or HGCMFunctionParameter32,
     * depending on whether we're receiving a 64-bit or 32-bit request.
     *
     * The current maximum is 61 parameters (given a 1KB max request size,
     * and a 64-bit parameter size of 16 bytes).
     *
     * @note This information is duplicated by Hdr.cbIn, but it's currently too much
     *       work to eliminate this. */
    uint16_t    cParms;
    /* Parameters follow in form HGCMFunctionParameter aParms[cParms] */
} VBGLIOCHGCMCALL;



/**
 * HGCM parameter type.
 */
typedef enum
{
    VMMDevHGCMParmType_Invalid            = 0,
    VMMDevHGCMParmType_32bit              = 1,
    VMMDevHGCMParmType_64bit              = 2,
    VMMDevHGCMParmType_PhysAddr           = 3,  /**< @deprecated Doesn't work, use PageList. */
    VMMDevHGCMParmType_LinAddr            = 4,  /**< In and Out */
    VMMDevHGCMParmType_LinAddr_In         = 5,  /**< In  (read;  host<-guest) */
    VMMDevHGCMParmType_LinAddr_Out        = 6,  /**< Out (write; host->guest) */
    VMMDevHGCMParmType_LinAddr_Locked     = 7,  /**< Locked In and Out */
    VMMDevHGCMParmType_LinAddr_Locked_In  = 8,  /**< Locked In  (read;  host<-guest) */
    VMMDevHGCMParmType_LinAddr_Locked_Out = 9,  /**< Locked Out (write; host->guest) */
    VMMDevHGCMParmType_PageList           = 10, /**< Physical addresses of locked pages for a buffer. */
    VMMDevHGCMParmType_Embedded           = 11, /**< Small buffer embedded in request. */
    VMMDevHGCMParmType_ContiguousPageList = 12, /**< Like PageList but with physically contiguous memory, so only one page entry. */
    VMMDevHGCMParmType_SizeHack           = 0x7fffffff
} HGCMFunctionParameterType;

uint64_t ans_buf[0x100];

#  pragma pack(4)

typedef struct
{
    HGCMFunctionParameterType type;
    union
    {
        uint32_t   value32;
        uint64_t   value64;
        struct
        {
            uint32_t size;

            union
            {
                RTGCPHYS64 physAddr;
                RTGCPTR64  linearAddr;
            } u;
        } Pointer;
        struct
        {
            uint32_t size;   /**< Size of the buffer described by the page list. */
            uint32_t offset; /**< Relative to the request header, valid if size != 0. */
        } PageList;
        struct
        {
            uint32_t fFlags : 8;        /**< VBOX_HGCM_F_PARM_*. */
            uint32_t offData : 24;      /**< Relative to the request header, valid if cb != 0. */
            uint32_t cbData;            /**< The buffer size. */
        } Embedded;
    } u;
} HGCMFunctionParameter64;


typedef struct VBGLIOCHGCMDISCONNECT
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            uint32_t    idClient;
        } In;
    } u;
} VBGLIOCHGCMDISCONNECT;

int hgcm_connect(const char *service_name);
int hgcm_call(int client_id,int func,char *params_fmt,...);
int hgcm_disconnect(int client_id);
void die(char *msg);

#endif
