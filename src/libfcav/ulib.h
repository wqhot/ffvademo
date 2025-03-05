#define	AVRX_HDL_ANY_VID	-1

struct port_find_name_
{
    unsigned    port_id;
    char        node_name[8];
    char        port_name[8];
};

struct port_find_id2_
{
    char        node_name[8];
    char        port_name[8];
};

struct port_get_spt_
{
    unsigned	port_ids[255];
};

struct axi_dma_
{
    unsigned long long host_addr;
	unsigned long long axi_addr;
    unsigned    bytes;
};

struct ip_init_param_
{
    int mtu;
};

struct asm_init_param_
{
    int rxSeqCnt; 
    int rxSeqBytes;
    int cls;
    int txSeqCnt;
    int txSeqBytes;
};

struct asm_set_port_param_
{
    int pno;
};

typedef struct{
    unsigned	PortID;
    char 	NodeName[8];	//Ŀ�Ľڵ�����
    char	PortName[8];	//Ŀ�Ķ˿�����
    unsigned	Priority;       //���ȼ�
    unsigned    MsgID;	        //��Ϣ��ʶ
    unsigned    reserv;         //�Զ���Ĵ���
    unsigned    UsrReg;	        //�Զ���Ĵ���
    unsigned short  ServiceType;//��������
    unsigned short  AckMode;    //ACK��ʽ
} ASM_HEADER;


typedef struct {
    unsigned        DID : 24;
    unsigned        Cmd : 8;
    unsigned        Buff_ID;
    unsigned        offset;
    unsigned        Src_Addr;
    unsigned        Length;
    unsigned        ctrlFlag;
} RW_CMD;

#define RW_CMD_READ             0x01
#define RW_CMD_WRITE            0x02
#define RW_CMD_LOCK             0x03
#define RW_CMD_UNLOCK           0x04
#define RW_CMD_INTREAD          0x81
#define RW_CMD_INTWRITE         0x82
#define RW_CMD_LISTEND          0x00
#define RW_CMD_ARRAYEND         0xFF

#define RW_CMDLIST_MAXLEN       0x400
#define RW_CMDLIST_MAXNUM       0x40

struct asm_xfer_desc_
{
    ASM_HEADER  ah;
    void*       buf;
    int         bufBytes;
};

struct asm_set_hdl_
{
    unsigned    msgId;
    unsigned    prtId;
    union {
        void*   handler;
        void    (*handler1)(unsigned char*, unsigned, int);
        void    (*handler2)(ASM_HEADER*, unsigned char*, unsigned, int);
    };
    int         param1;
    int         needAsmHdr;
    int         slotIdx;
};

struct asm_get_hdl_
{
    unsigned    msgId;
    unsigned    prtId;
};

struct asm_wait_io_
{
    unsigned    msgId;	//in
    unsigned    prtId;	//in
    unsigned    needAH;	//in
    unsigned    param1; //in 
    union {				//in
        void*   usr_handler;
        void    (*handler1)(unsigned char*, unsigned, int);
        void    (*handler2)(ASM_HEADER*, unsigned char*, unsigned, int);
        void	(*khdl)(unsigned, unsigned char*, unsigned, void*);
    };
    unsigned char* buf; //out
    unsigned    len;    //out
    ASM_HEADER  ah;     //out
    void*       pRD;	//for driver use.
    void*       mcTs;   //for kernel mode.
};

struct avrx_wait_io_
{
    unsigned    real_vid;
    unsigned    user_specified_vid;
};

struct common_wait_io_  //fc common protocol
{
    void*		rxData;	//[out]
    unsigned	bytes;	//[out]
};

struct asm_ts_hook_
{
    unsigned    browPortId;
    unsigned	asmRxId1;
    unsigned	asmRxId2;
};

struct common_init_param_
{
    int rxSeqCnt; 
    int rxSeqBytes;
    int cls;
    int txSeqCnt;
    int txSeqBytes;
};

typedef struct {                //ͨ��Э��ṹͷ
    unsigned    PortID;			//
    char	    PortName[8];	//�˿�����
    char	    NodeName[8];	//�ڵ�����	
    unsigned    R_CTL;			//·���ֶ�
    unsigned    CS_CTL;
    unsigned    TYPE;			//�����ֶ�
    unsigned    S_CTL;			//���п����ֶ�
    unsigned short OX_ID;
    unsigned short RX_ID;
    unsigned    Param;
} CMN_HEADER;

struct common_xfer_desc_
{
    CMN_HEADER  ah;
    void*       buf;
    int         bufBytes;
};

struct common_add_grp_
{
	unsigned	grpID;
	unsigned	portID;
};

struct rw_add_buf_
{
    void*       buf;
    unsigned    buff_size;
    unsigned    buff_id; 
    unsigned    buff_spec;
};

struct rw_add_buf_ondev_
{
    unsigned long long addr_ondev;
    unsigned    buff_size;
    unsigned    buff_id; 
    unsigned    buff_spec;
};

struct rw_rw_prt_
{
    unsigned tgt_port_id;
    unsigned tgt_buff_id;
    unsigned ofs;
    unsigned len;
    void*    buf;
    unsigned flag;  //interrupt flag
};

struct rw_rw_mem_
{
    unsigned tgt_port_id;
    unsigned tgt_buff_id;
    unsigned ofs;
    unsigned len;
    void*    buf;
};

struct rw_rw_reg_
{
    unsigned tgt_port_id;
    unsigned tgt_buff_id; 
    unsigned ofs;
    unsigned reg_value;
};

struct rw_lock_
{
    unsigned tgt_port_id;
    unsigned tgt_buff_id;
};

struct rw_cmd_arr_
{
    unsigned total_bytes;
    RW_CMD*  cmds;
};

struct av_tx_ctrl_
{
    unsigned vid; 
    unsigned start;
    unsigned d25freq;
    unsigned targetId;
    unsigned colorAtr;
    unsigned posx;
    unsigned posy;
    unsigned scnWidth; 
    unsigned scnHeight; 
    void*    memAddr;
};

struct av_rx_ctrl_
{
    unsigned vid;
    unsigned start;
    unsigned srcId;
    unsigned posx;
    unsigned posy;
    unsigned scnWidth;
    unsigned scnHeight;
    unsigned clrBits;		//b31=receive by port1, b30=synchronize window, b29~16=clrType, b15~0=clrBits
    void*    memAddr;
};

struct pmap_param_
{
    union {
        void*           appAddr;
        unsigned long   phyAddr;
    } u;
};

struct port_setname_param_
{
    unsigned long long  name;
};

struct sata_cmd_
{
    unsigned    cmd;
    unsigned    lba;
    unsigned    lbaHi_secs;
    unsigned    prd_size;
    unsigned    rpt;
    void*       buf;
};

struct init_1553_
{
	unsigned	subCnt;
	unsigned	shareBytes;
};

//add_1553_sub_.sub_spec
#define	ADD_1553_SUB_READ_EN			0x8
#define	ADD_1553_SUB_WRITE_EN			0x4
#define	ADD_1553_SUB_READ_IE			0x2
#define	ADD_1553_SUB_WRITE_IE			0x1
#define	ADD_1553_SUB_LAST				0x100

struct add_1553_sub_
{
    void*       buf;
    unsigned    sub_size;
    unsigned    sub_id; 
    unsigned    sub_spec;
};

struct send_1553_msg_
{
	unsigned	ctrl;
	unsigned	nt1;
	unsigned	nt1Sub;
	unsigned	nt2;
	unsigned	nt2Sub;
	unsigned	bytes;
};

#define MAX_NUM_SYNC_CMD        256

struct b1553_cmd_ {
	unsigned		nt1;
	unsigned		nt1Sub;
	unsigned		byteCnt;
	unsigned		nt2;
	unsigned		nt2Sub;
	unsigned short  ctrlWord; 
	unsigned short  period;
	unsigned short  phase;
	unsigned short  ctrl_flag;
};

struct b1553_cmd_tbl_ {
	unsigned short 	maxFreq;
	unsigned short 	msgCount;
	struct b1553_cmd_ cmdlist[MAX_NUM_SYNC_CMD];
};

struct asm_set_kernel_rxhdl_
{   //if @hdl=0, remove previous hdl.
	int (*hdl)(unsigned char* buf, unsigned bytes, unsigned portId, unsigned msgId);
};

struct asm_set_kernel_rxhdl2_
{   //if @hdl=0, remove previous hdl.
	int (*hdl)(unsigned char* buf, unsigned bytes, unsigned portId, unsigned msgId, void* ctx);
    unsigned portId;
    unsigned msgId;
    void*    ctx;
};

struct ip_send_param_
{
    unsigned portId;
    unsigned bytes;
    void*    buf;
};

#define	FCVE_IOC_MAGIC	0x66
#define	DIOC_PRT_BR_EN  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x04, 0)
#define	DIOC_PRT_BR_DE  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x05, 0)
#define DIOC_IP_SEND    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x06, sizeof(struct ip_send_param_))
#define DIOC_PRT_SPT    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x07, sizeof(struct port_get_spt_))
#define	DIOC_PRT_FNAME  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x08, sizeof(struct port_find_name_))
#define	DIOC_PRT_F_ID2  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x09, sizeof(struct port_find_id2_))
#define	DIOC_PRT_GID0	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x0A, 0)
#define	DIOC_PRT_GID1	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x0B, 0)
#define	DIOC_PRT_SNN    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x0C, sizeof(struct port_setname_param_))
#define	DIOC_PRT_SPN    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x0D, sizeof(struct port_setname_param_))
#define	DIOC_UNMAP_BUF	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x14, sizeof(struct pmap_param_))
#define	DIOC_PMT_A2P	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x16, sizeof(struct pmap_param_))
#define	DIOC_PMT_P2A	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x17, sizeof(struct pmap_param_))
#define	DIOC_ASM_INIT	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x23, sizeof(struct asm_init_param_))
#define	DIOC_ASM_SETPRT _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x25, sizeof(struct asm_set_port_param_))
#define	DIOC_ASM2_XFER	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x27, sizeof(struct asm_xfer_desc_))
#define	DIOC_ASM3_XFER	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x28, sizeof(struct asm_xfer_desc_))
#define DIOC_ASM_KHDL	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x29, sizeof(struct asm_set_kernel_rxhdl_))
#define DIOC_ASM_SETHDL _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x2A, sizeof(struct asm_set_hdl_))
#define DIOC_ASM_WAITIO _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x2C, sizeof(struct asm_wait_io_))
#define DIOC_ASM_DELHDL _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x2D, sizeof(struct asm_get_hdl_))
#define DIOC_ASM_KHDL2	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x2F, sizeof(struct asm_set_kernel_rxhdl2_))
#define DIOC_ASM_TSHOOK _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x3C, sizeof(struct asm_ts_hook_))
#define	DIOC_RW_INIT	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x40, 0)
#define	DIOC_RW_FREE	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x41, 0)
#define	DIOC_RW_ADDBUF	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x42, sizeof(struct rw_add_buf_))
#define	DIOC_RW_DEVBUF	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x43, sizeof(struct rw_add_buf_ondev_))
#define	DIOC_RW_PORTRD  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x44, sizeof(struct rw_rw_prt_))
#define	DIOC_RW_PORTWR  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x45, sizeof(struct rw_rw_prt_))
#define	DIOC_RW_RDMEM	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x48, sizeof(struct rw_rw_mem_))
#define	DIOC_RW_WRMEM	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x49, sizeof(struct rw_rw_mem_))
#define DIOC_RW_LOCK    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x4B, sizeof(struct rw_lock_))
#define DIOC_RW_UNLOCK  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x4C, sizeof(struct rw_lock_))
#define	DIOC_RW_RDREG	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x4D, sizeof(struct rw_rw_reg_))
#define	DIOC_RW_WRREG	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x4E, sizeof(struct rw_rw_reg_))
#define DIOC_RW_CMDARR	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x4F, sizeof(struct rw_cmd_arr_))
#define	DIOC_CMN_INIT	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x50, sizeof(struct common_init_param_))
#define DIOC_CMN_WAITIO	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x52, sizeof(struct common_wait_io_))
#define	DIOC_CMN_SETPRT _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x55, sizeof(struct asm_set_port_param_))
#define	DIOC_CMN_XFER   _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x58, sizeof(struct common_xfer_desc_))
#define DIOC_CMN_ADDGRP	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x5A, sizeof(struct common_add_grp_))
#define DIOC_CMN_EXTGRP	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x5B, sizeof(struct common_add_grp_))
#define DIOC_CMN_QRYGRP	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x5C, sizeof(struct common_add_grp_))
#define	DIOC_AVTX_INIT  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x60, 0)
#define DIOC_AVTX_RESET _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x61, 0)
#define DIOC_AVTX_CTRL  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x62, sizeof(struct av_tx_ctrl_))
#define DIOC_AVCP_CTRL  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x63, sizeof(struct av_tx_ctrl_))
#define DIOC_AVRX_CTRL  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x64, sizeof(struct av_rx_ctrl_))
#define DIOC_AVRP_CTRL  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x65, sizeof(struct av_rx_ctrl_))
#define DIOC_AVRX_WAIT  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x6A, sizeof(struct avrx_wait_io_))
#define DIOC_SATA_INIT  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x70, 0)
#define DIOC_SATA_CMD   _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x78, sizeof(struct sata_cmd_))
#define DIOC_1553_INIT  _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x80, sizeof(struct init_1553_))
#define	DIOC_1553_ADDSB	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x81, sizeof(struct add_1553_sub_))
#define	DIOC_1553_DELSB	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x82, sizeof(struct add_1553_sub_))
#define	DIOC_1553_SENDM	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x84, sizeof(struct send_1553_msg_))
#define	DIOC_1553_SENDL	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x85, sizeof(struct b1553_cmd_tbl_))
#define	DIOC_1553_SENDB	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0x86, sizeof(struct b1553_cmd_tbl_))
#define	DIOC_DDR_INIT   _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0xF2, 0)
#define	DIOC_IP_INIT    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0xF3, sizeof(struct ip_init_param_))
#define DIOC_AXI_DTM    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0xF4, sizeof(struct axi_dma_))
#define DIOC_AXI_MTD    _IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0xF5, sizeof(struct axi_dma_))
#define DIOC_GLB_RST	_IOC(_IOC_WRITE, FCVE_IOC_MAGIC, 0xF8, 0)

