#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>

#include "ulib.h"
#include "fcBrowse.h"

#define CARD_DEV_NAME 	"/dev/fcve%d"
#define ASM_DEV_NAME 	"/dev/asm%d"
#define ASM_DEV0_NAME	"/dev/asm0"
#define RW_DEV_NAME 	"/dev/fcrw0"
#define FC1553_DEV_NAME	"/dev/fc15530"
#define AV_DEV_NAME 	"/dev/fcav0"
#define PRT_DEV_NAME 	"/dev/fc_port%d"
#define SATA_DEV_NAME 	"/dev/fcsata0"
#define COMMON_DEV_NAME "/dev/common0"

#define	MAX_HDL         32
#define	MAX_CARD		2
static volatile int g_multiple_card_detected = 0;

#define PORT_PER_CARD	2
#define	PORT_MAX_PEER	254

static int open_dev_by_idx(const char* name, int idx)
{
    char    dev_name[32];
    int     retv;

    sprintf(dev_name, name, idx);
    return  open(dev_name, O_RDWR);
}

//used if multiple card detected, to select a card. however, FC protocol's default tx port is managed by the driver.
struct port_route_table_ {
    unsigned	portNr;
    unsigned	portId;
    unsigned	peerIds[PORT_MAX_PEER];
} g_route[MAX_CARD*PORT_PER_CARD] = {0,0,0,0};

static void build_route(void)
{
    int fd;
    unsigned  i, j;

    memset(g_route, 0, sizeof(g_route));
    for (i=0; i<MAX_CARD; ++i)
    {
        for (j=0; j<PORT_PER_CARD; ++j)
        {
            fd = open_dev_by_idx(PRT_DEV_NAME, i*4 +j);
			if (fd <=0)
				continue;
            g_route[i * PORT_PER_CARD + j].portNr = i*4 + j;
			ioctl(fd, DIOC_PRT_SPT, &g_route[i * PORT_PER_CARD + j].portId);
            close(fd);
        }
    }
} 

static int select_card(unsigned tgtPortId)
{
    unsigned  i, j, renewed = 0;

    if (!g_multiple_card_detected || !tgtPortId)
        return	0;

    while (1)
    {
        for (i=0; i<MAX_CARD*PORT_PER_CARD; ++i)
        {
            if (!g_route[i].portNr)
			    continue;
		
            for (j=0; (j<PORT_MAX_PEER) && g_route[i].peerIds[j]; ++j)
                if (g_route[i].peerIds[j] == tgtPortId)
				    return	i / PORT_PER_CARD;
        }
        if (renewed)
		    break;

        build_route();
        renewed = 1;
    };
    return 0;
}

int rawIpSend(unsigned srcPortNr, unsigned tgtPortId, unsigned char* buf, unsigned bytes)
{
    int     fd, retv;
    struct ip_send_param_ param;

    fd = open_dev_by_idx(PRT_DEV_NAME, srcPortNr);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    param.bytes = bytes;
    param.portId = tgtPortId;
    param.buf = buf;
    retv = ioctl(fd, DIOC_IP_SEND, &param);
    close(fd);
    return  retv;
}

int getNPortId(int pno)
{   //获取端口标识
    int     fd, retv;

    if ((pno < 0) || (pno >= MAX_CARD*4))
        return  -1;

    fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    retv = ioctl(fd, pno ? DIOC_PRT_GID1 : DIOC_PRT_GID0, 0);
    close(fd);
    return  retv;
}

void port_broadcast_enable(unsigned pno)
{
    int     fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return;
    }
    ioctl(fd, DIOC_PRT_BR_EN, 0);
    close(fd);
}

void port_broadcast_disable(unsigned pno)
{
    int     fd = open_dev_by_idx(PRT_DEV_NAME, pno);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return;
    }
    ioctl(fd, DIOC_PRT_BR_DE, 0);
    close(fd);
}

int setNNodeName(int pno, char *pNodeName)
{
    struct port_setname_param_  param;
    int     fd, i;

    if ((pno < 0) || (pno >= MAX_CARD*4))
        return -1;

    if (!pNodeName || !pNodeName[0])
        return -1;

    fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return -1;
    }
    param.name = 0ULL;

    for (i = 0; i < 8; ++i)
    {
        ((unsigned char*)(&param.name))[i] = pNodeName[i];
        if (!pNodeName[i])
            break;
    }
    i = ioctl(fd, DIOC_PRT_SNN, &param);
    close(fd);
    return  i;
}

int setNPortName(int pno, char *pPortName)
{
    struct port_setname_param_  param;
    int     fd, i;

    if ((pno < 0) || (pno >= MAX_CARD*4))
        return -1;

    if (!pPortName || !pPortName[0])
        return -1;

    fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return -1;
    }
    param.name = 0ULL;

    for (i = 0; i < 8; ++i)
    {
        ((unsigned char*)(&param.name))[i] = pPortName[i];
        if (!pPortName[i])
            break;
    }
    i = ioctl(fd, DIOC_PRT_SPN, &param);
    close(fd);
    return  i;
}

int FindFullName(int pno, unsigned pid, char *nname, char *pname)
{
    int     fd, retv;
    struct port_find_name_ param;

    fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    param.port_id = pid;
    retv = ioctl(fd, DIOC_PRT_FNAME, &param);
    memcpy(nname, param.node_name, 8);
    memcpy(pname, param.port_name, 8);
    close(fd);
    return  retv;
}

unsigned FindPortID2(int pno, char *pPortName, char *pNodeName)
{
    int     fd, retv;
    struct port_find_id2_ param;

    fd = open_dev_by_idx(PRT_DEV_NAME, pno);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    memcpy(param.port_name, pPortName, sizeof(param.port_name));
    memcpy(param.node_name, pNodeName, sizeof(param.node_name));
    retv = ioctl(fd, DIOC_PRT_F_ID2, &param);
    close(fd);
    return  retv;
}

int asm_Port_Set(int dno, int pno)
{
    struct asm_set_port_param_ param;
    int retv, fd = open_dev_by_idx(ASM_DEV_NAME, dno);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.pno = pno;
    retv = ioctl(fd, DIOC_ASM_SETPRT, &param);
    close(fd);
    return  retv;
}

int asm_Initial(int rxSeqCnt, int rxSeqBytes, int cls, int txSeqCnt, int txSeqBytes)
{
    struct asm_init_param_ param;
    int retv, idx, fd;

    for (idx = 0; idx < MAX_CARD; ++idx)
    {
        if ((fd = open_dev_by_idx(ASM_DEV_NAME, idx)) < 0)
        {
            if (!idx)
                printf("open asm error: %d\n", fd);
            return fd;
        }
        if (idx)
            g_multiple_card_detected = 1;

        param.rxSeqCnt = rxSeqCnt;
        param.rxSeqBytes = rxSeqBytes;
        param.txSeqCnt = txSeqCnt;
        param.txSeqBytes = txSeqBytes;
        param.cls = cls;
        retv = ioctl(fd, DIOC_ASM_INIT, &param);
        close(fd);
    }
    return  retv;
}

int asm3_Transfer(ASM_HEADER *phd, const char* buff, int len)
{
    struct asm_xfer_desc_ param;
    int retv, fd = open_dev_by_idx(ASM_DEV_NAME, select_card(phd->PortID));

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    memcpy(&param.ah, phd, sizeof(ASM_HEADER));
    param.buf = (void*)buff;
    param.bufBytes = len;
    retv = ioctl(fd, DIOC_ASM3_XFER, &param);
    close(fd);

    if (retv <0)
        build_route();
    return  retv;
}

int asm2_Transfer(ASM_HEADER *phd, const char* buff, int len)
{
    struct asm_xfer_desc_ param;
    int retv, fd = open_dev_by_idx(ASM_DEV_NAME, select_card(phd->PortID));

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    memcpy(&param.ah, phd, sizeof(ASM_HEADER));
    param.buf = (void*)buff;
    param.bufBytes = len;
    retv = ioctl(fd, DIOC_ASM2_XFER, &param);
    close(fd);

    if (retv <0)
        build_route();
    return  retv;
}

struct wait_io_param_
{
    int			fd;       //用这个值来判断call_back是否有效。若<=0，则表明损坏，或已被移除。
    unsigned 	ctl_code;
    void		(*hdl)(void*);
    //void* 	ctx;  	  //实际这里放的是struct xxx_wait_io
};

static struct wait_io_param_* g_hdl_tbl[MAX_HDL] = {0};

static void* wait_io_thread(void* ctx)
{
    struct wait_io_param_* p = (struct wait_io_param_*)ctx;

    while (p->fd >0)
    {
		int retv;

        retv = ioctl(p->fd, p->ctl_code, p+1);
		if (retv >= 0)
	        (p->hdl)(p+1);
		else
			printf("ioctl ret %d\n", retv);
    }
    free(p);  //执行到此表示被移除了。
    return  0;
}

static int create_wait_io_thread(const char* filename, unsigned ctl_code, void* handler, void* ctx)
{   //返回在g_hdl_tbl中的索引+1, <=0表示失败。
    pthread_t   			ntid;
    unsigned				tbl_idx;
    struct wait_io_param_*	p = (struct wait_io_param_*)malloc(sizeof(struct wait_io_param_) + (_IOC_SIZE(ctl_code)));

    if (!p)
    {
        printf("create_wait_io_thread: malloc failed!\n");
        return -2;
    }
    
    p->hdl = handler;
    p->ctl_code = ctl_code;
    if (_IOC_SIZE(ctl_code))
		memcpy(p+1, ctx, _IOC_SIZE(ctl_code));

    if ((p->fd = open(filename, O_RDWR)) >0)
    {
        for (tbl_idx = 0; tbl_idx < MAX_HDL; ++tbl_idx)
        {
            if (!g_hdl_tbl[tbl_idx])
            {
				if (0 == pthread_create(&ntid, 0, wait_io_thread, p))
				{
				    g_hdl_tbl[tbl_idx] = p; 
				    return tbl_idx + 1;
				} else {
                    printf("create thread error.\n");
                }
                break;
            }
        }
        if (tbl_idx >= MAX_HDL)
            printf("no room for new handler.\n");
        close(p->fd);
    } else
        printf("open %s error %d.\n", filename, p->fd);

    free(p);
    return -1;
}

int remove_wait_io_thread(int idx)
{
    if ((idx >=1) && (idx <= MAX_HDL))
    {
        if (g_hdl_tbl[idx-1] && (g_hdl_tbl[idx-1]->fd > 0))
		{
		    close(g_hdl_tbl[idx-1]->fd);
		    g_hdl_tbl[idx-1]->fd = 0;
            g_hdl_tbl[idx-1] = 0;
		    return 1;
        }
    }
    return -1;
}

int add_avrx_handler(void (*handler)(void*), unsigned vid)
{
    struct avrx_wait_io_ param = {0, vid};
    return create_wait_io_thread(AV_DEV_NAME, DIOC_AVRX_WAIT, handler, &param);
}

int del_avrx_handler(int idx)
{
    return remove_wait_io_thread(idx);
}

unsigned char* map_asm_dma_buf(int card_idx, unsigned bytes)
{
    unsigned char* retv;
    int fd = open_dev_by_idx(ASM_DEV_NAME, card_idx);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  0;
    }
    retv = (unsigned char*)mmap(0, bytes, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    return  retv;
}

pthread_mutex_t g_asm_mutex = PTHREAD_MUTEX_INITIALIZER;
struct asm_user_q_ {
    ASM_HEADER*    pShd;
    union {
        void*   handler;
        void    (*handler1)(unsigned char*, unsigned, int);
        void    (*handler2)(ASM_HEADER*, unsigned char*, unsigned, int);
    };
    unsigned       len;
    unsigned       needAH;
    unsigned       param1;
    unsigned char  buf[8192];
};

#define ASM_UQ_CNT	128
struct asm_user_q_ g_asm_uq[ASM_UQ_CNT];
unsigned g_asm_qhead = 0;
unsigned g_asm_qtail = 0; //head == tail means empty.

//#define	NO_PROVIDER	1

#ifdef	NO_PROVIDER
static void asm_hdl_provider(struct asm_wait_io_* p)
{
    if (p->len > sizeof(g_asm_uq->buf))
        return;

    if (p->needAH)
        p->handler2(&p->ah, p->buf, p->len, p->param1);
    else
        p->handler1(p->buf, p->len, p->param1);
}
#else
static void asm_hdl_provider(struct asm_wait_io_* p)
{
    int new_tail, hd = *(unsigned*)(p->buf);

    if (p->len > sizeof(g_asm_uq->buf))
        return;

    pthread_mutex_lock(&g_asm_mutex);
    new_tail = (g_asm_qtail + 1) % ASM_UQ_CNT;
    if (new_tail != g_asm_qhead)
    {
        g_asm_uq[g_asm_qtail].pShd = &p->ah;
        memcpy(g_asm_uq[g_asm_qtail].buf, p->buf, p->len);
        *(unsigned*)(g_asm_uq[g_asm_qtail].buf) = hd;
        g_asm_uq[g_asm_qtail].len = p->len;
        g_asm_uq[g_asm_qtail].needAH = p->needAH;
        g_asm_uq[g_asm_qtail].handler = p->usr_handler;
        g_asm_uq[g_asm_qtail].param1 = p->param1;
        g_asm_qtail = new_tail;
    }
    pthread_mutex_unlock(&g_asm_mutex);
}
#endif

static void* asm_hdl_consumer(void* unused)
{
    unsigned qh, qt, tmp_len;

    while (1)
    {
        pthread_mutex_lock(&g_asm_mutex);
        qh = g_asm_qhead;
        qt = g_asm_qtail;
        if (qh != qt)
        {
            g_asm_qhead = (qh + 1) % ASM_UQ_CNT;
        }
        pthread_mutex_unlock(&g_asm_mutex);

        if (qh == qt)
        {
            usleep(50);
            continue;
        }
        if (g_asm_uq[qh].needAH)
            g_asm_uq[qh].handler2(g_asm_uq[qh].pShd, g_asm_uq[qh].buf, g_asm_uq[qh].len, g_asm_uq[qh].param1);
        else
            g_asm_uq[qh].handler1(g_asm_uq[qh].buf, g_asm_uq[qh].len, g_asm_uq[qh].param1);
    }
    return 0;
}

static int add_asm_handler(unsigned asmId, unsigned srcPortId, void* handler, int param1, int needAsmHdr)
{
    struct asm_wait_io_ param;
    pthread_t   		ntid;
    unsigned            i = (0xf1ea == asmId) ? 4 : 1;

    param.msgId  = asmId;
    param.prtId  = srcPortId;
    param.param1 = param1;
    param.needAH = needAsmHdr;
    param.usr_handler = handler;
#ifdef	NO_PROVIDER
	create_wait_io_thread(ASM_DEV0_NAME, DIOC_ASM_WAITIO, asm_hdl_provider, &param);
#else
    map_asm_dma_buf(0, 16*1024*1024);

	if (0 != pthread_create(&ntid, 0, asm_hdl_consumer, 0))
    {
        printf("pthread_create error.\n");
        return -1;
    }
    while (i--)
        create_wait_io_thread(ASM_DEV0_NAME, DIOC_ASM_WAITIO, asm_hdl_provider, &param);
#endif
    return 1;
}

int add_asm_cmdHandler(unsigned asmId, unsigned srcPortId, void (*handler)(unsigned char*, unsigned, int), int param1)
{   //返回>=0表示成功
    return add_asm_handler(asmId, srcPortId, handler, param1, 0);
}

int add_asmhead_cmdHandler(unsigned asmId, unsigned srcPortId, void (*handler)(ASM_HEADER*, unsigned char*, unsigned, int), int param1)
{
    return add_asm_handler(asmId, srcPortId, handler, param1, 1);
}

int del_asm_cmdHandler(unsigned asmId, unsigned srcPortId)
{
    unsigned idx;

    for (idx = 0; idx < MAX_HDL; ++idx)
    {
        if (g_hdl_tbl[idx] && (DIOC_ASM_WAITIO == g_hdl_tbl[idx]->ctl_code))
        {
            struct asm_wait_io_* p = (struct asm_wait_io_*)(g_hdl_tbl[idx] +1);
            if ((p->msgId == asmId) && (p->prtId == srcPortId))
                return remove_wait_io_thread(idx+1);
        }
    }
    return  -1;
}

static void showPhy(void* p)
{
    unsigned long virPageIdx = (unsigned long)p / (unsigned long)getpagesize();
    unsigned long pfn_item;
    int fd = open("/proc/self/pagemap", O_RDONLY);
    
    if (fd <0)
    {
        printf("open /proc/self/pagemap failed.\n");
        return;
    }
    if (lseek(fd, virPageIdx*8, SEEK_SET) < 0)
    {
        printf("lseek failed!\n");
        return;
    }
    if (8 != read(fd, &pfn_item, 8))
    {
        printf("read failed!\n");
        return;
    }
    printf("pfn_item = %lx\n", pfn_item);
    close(fd);
}

int rw_Initial(void)
{
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_RW_INIT, 0);
    close(fd);
    return  retv;
}

int rw_free(void)
{
    int fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    ioctl(fd, DIOC_RW_FREE, 0);
    close(fd);
    return  0;
}

int rw_Add_Buff(char* buff_addr, int buff_size, unsigned buff_id, int buff_spec)
{
    struct rw_add_buf_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.buf = buff_addr;
    param.buff_size = buff_size;
    param.buff_id = buff_id;
    param.buff_spec = buff_spec;
    retv = ioctl(fd, DIOC_RW_ADDBUF, &param);
    close(fd);
    return  retv;
}

int rw_Add_BuffOnDev(unsigned long long addr_ondev, int buff_size, unsigned buff_id, int buff_spec)
{
    struct rw_add_buf_ondev_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.addr_ondev = addr_ondev;
    param.buff_size = buff_size;
    param.buff_id = buff_id;
    param.buff_spec = buff_spec;
    retv = ioctl(fd, DIOC_RW_DEVBUF, &param);
    close(fd);
    return  retv;
}

int rw_PortRead(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len, int intFlag)
{
    struct rw_rw_prt_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id;
    param.buf = buf;
    param.ofs = ofs;
    param.len = len;
    param.flag = intFlag;
    retv = ioctl(fd, DIOC_RW_PORTRD, &param);
    close(fd);
    return  retv;
}

int rw_PortWrite(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len, int intFlag)
{
    struct rw_rw_prt_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id;
    param.buf = buf;
    param.ofs = ofs;
    param.len = len;
    param.flag = intFlag;
    retv = ioctl(fd, DIOC_RW_PORTWR, &param);
    close(fd);
    return  retv;
}

int rw_read_mem(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len)
{
    struct rw_rw_mem_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id;
    param.buf = buf;
    param.ofs = ofs;
    param.len = len;
    retv = ioctl(fd, DIOC_RW_RDMEM, &param);
    close(fd);
    return  retv;
}

int rw_write_mem(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len)
{
    struct rw_rw_mem_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id;
    param.buf = buf;
    param.ofs = ofs;
    param.len = len;
    retv = ioctl(fd, DIOC_RW_WRMEM, &param);
    close(fd);
    return  retv;
}

int rw_read_reg(unsigned port_id, unsigned buff_id, unsigned ofs)
{
    struct rw_rw_reg_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id; 
    param.ofs = ofs;
    retv = ioctl(fd, DIOC_RW_RDREG, &param);
    close(fd);
	//printf("DIOC_RW_RDREG return %x\n", retv);
    return (retv == 0) ? param.reg_value : (0xFF000000 | retv);
}

int rw_write_reg(unsigned port_id, unsigned buff_id, unsigned ofs, unsigned reg_val)
{
    struct rw_rw_reg_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.tgt_port_id = port_id;
    param.tgt_buff_id = buff_id; 
    param.ofs = ofs;
    param.reg_value = reg_val;
    retv = ioctl(fd, DIOC_RW_WRREG, &param);
    close(fd);
    return  retv;
}

int rw_cmd_arr(unsigned port_id, unsigned buff_id, RW_CMD* cmds)
{
    struct rw_cmd_arr_ param = {0,0};
    int i, retv = -1, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    for (i=0; i<RW_CMDLIST_MAXNUM*RW_CMDLIST_MAXLEN; ++i)
    {
	if (RW_CMD_ARRAYEND == cmds[i].Cmd)
        {
	    param.total_bytes = sizeof(RW_CMD)*(i+1);
	    param.cmds = cmds;
	    retv = ioctl(fd, DIOC_RW_CMDARR, &param);
	    break;
        }
    }
    close(fd);
    return  retv;
}

int rw_lock(unsigned port_id, unsigned buff_id)
{
    struct rw_lock_ param = { port_id, buff_id };
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_RW_LOCK, &param);
    close(fd);
    return  retv;
}

int rw_unlock(unsigned port_id, unsigned buff_id)
{
    struct rw_lock_ param = { port_id, buff_id };
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_RW_UNLOCK, &param);
    close(fd);
    return  retv;
}

int fc1553_Initial(int sub_cnt, int share_bytes)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);
	struct init_1553_ param = {sub_cnt, share_bytes};

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_1553_INIT, &param);
    close(fd);
    return  retv;
}

int fc1553_add_sub(unsigned sub_id, void* buf, unsigned sub_size, unsigned sub_spec)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);
	struct add_1553_sub_ param;

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.buf      = buf;
    param.sub_size = sub_size;
    param.sub_id   = sub_id;
    param.sub_spec = sub_spec;

    retv = ioctl(fd, DIOC_1553_ADDSB, &param);
    close(fd);
    return  retv;
}

int fc1553_del_sub(unsigned sub_id)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);
	struct add_1553_sub_ param;

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.buf      = 0;
    param.sub_size = 0;
    param.sub_id   = sub_id;
    param.sub_spec = 0;

    retv = ioctl(fd, DIOC_1553_DELSB, &param);
    close(fd);
    return  retv;
}

int fc1553_msg_send(int nt1, int nt1Sub, int nt2, int nt2Sub, int ctrl, int bytes)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);
	struct send_1553_msg_ param;

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
	param.ctrl   = ctrl; 
	param.nt1    = nt1; 
	param.nt1Sub = nt1Sub; 
	param.nt2    = nt2;
	param.nt2Sub = nt2Sub; 
	param.bytes  = bytes;

    retv = ioctl(fd, DIOC_1553_SENDM, &param);
    close(fd);
    return  retv;
}

int fc1553_lst_send(struct b1553_cmd_tbl_* tbl)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
	retv = ioctl(fd, DIOC_1553_SENDL, tbl);
    close(fd);
    return  retv;
}

int fc1553_blk_send(struct b1553_cmd_tbl_* tbl)
{
    int retv, fd = open(FC1553_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
	retv = ioctl(fd, DIOC_1553_SENDB, tbl);
    close(fd);
    return  retv;
}

int av_tx_init(void)
{
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_AVTX_INIT, 0);
    close(fd);
    return  retv;
}

int av_tx_reset(void)
{
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_AVTX_RESET, 0);
    close(fd);
    return  retv;
}

int av_tx_ctrl(unsigned vid, unsigned start, unsigned freq, unsigned targetId, unsigned colorAtr, unsigned posx, unsigned posy, unsigned width, unsigned height, void* memAddr)
{
    struct av_tx_ctrl_ param;
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.vid = vid;
    param.start = start; 
    param.d25freq = freq;
    param.targetId = targetId;
    param.colorAtr = colorAtr;
    param.posx = posx;
    param.posy = posy;
    param.scnWidth = width;
    param.scnHeight = height;
    param.memAddr = memAddr;
    retv = ioctl(fd, DIOC_AVTX_CTRL, &param);
    close(fd);
    return  retv;
}

int av_rx_ctrl(unsigned vid, unsigned start, unsigned srcId, unsigned posx, unsigned posy, unsigned width, unsigned height, unsigned clrBits, void* memAddr)
{
    struct av_rx_ctrl_ param;
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.vid = vid;
    param.start = start; 
    param.srcId = srcId;
    param.posx = posx;
    param.posy = posy;
    param.scnWidth = width;
    param.scnHeight = height;
    param.clrBits = clrBits;
    param.memAddr = memAddr;
    retv = ioctl(fd, DIOC_AVRX_CTRL, &param);
    close(fd);
    return  retv;
}

int av_cp_ctrl(unsigned vid, unsigned start, unsigned freq, unsigned targetId, unsigned colorAtr, unsigned width, unsigned height, void* memAddr)
{
    struct av_tx_ctrl_ param;
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.vid = vid;
    param.start = start; 
    param.d25freq = freq;
    param.targetId = targetId;
    param.colorAtr = colorAtr;
    param.scnWidth = width;
    param.scnHeight = height;
    param.memAddr = memAddr;
    retv = ioctl(fd, DIOC_AVCP_CTRL, &param);
    close(fd);
    return  retv;
}

//返回进程虚拟地址, 0表示失败；
unsigned char* map_txbuf(unsigned bytes)
{
    unsigned char* retv;
    int fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  0;
    }
    retv = (unsigned char*)mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return  retv;
}

unsigned char* map_videobuf(unsigned phyAddr, unsigned bytes)
{
    unsigned char* retv;
    int fd = open(AV_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  0;
    }
    retv = (unsigned char*)mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phyAddr);
    close(fd);
    return  retv;
}

unsigned char* map_rwbuf(unsigned bytes)
{
    unsigned char* retv;
    int fd = open(RW_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  0;
    }
    retv = (unsigned char*)mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return  retv;
}

unsigned get_rwbuf_phy(unsigned char* buf)
{
    int fd = open(RW_DEV_NAME, O_RDWR);
    struct pmap_param_ param;
    unsigned retv;

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  0;
    }
    param.u.appAddr = buf;
    retv = ioctl(fd, DIOC_PMT_A2P, &param);
    close(fd);
    if (0 == retv)
        return param.u.phyAddr;
    return 0;
}

void unmap_txbuf(void* buf)
{
    struct pmap_param_ param;
    int retv, fd = open(AV_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return;
    }
    param.u.appAddr = buf;
    retv = ioctl(fd, DIOC_UNMAP_BUF, &param);
    close(fd);
}

void unmap_rwbuf(void* buf)
{
    struct pmap_param_ param;
    int retv, fd = open(RW_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return;
    }
    param.u.appAddr = buf;
    retv = ioctl(fd, DIOC_UNMAP_BUF, &param);
    close(fd);
}

static int browGetPortNr(int browID)
{
    char    pname[16], nname[16];

    if (FindFullName(0, browID, nname, pname) <0)
    {
        return  0;
    }
    printf("browGetPortNr: %c\n", pname[7]);
    if (pname[7] == '1')
        return  1;
    return  0;
}

int browMonInit(int browID, int monID, int monSize)
{
    unsigned    i, rw_bufferId;

    printf("Enter browMonInit: %x %x %x\n", browID, monID, monSize);
    if (monID<0 || monID>7 || monSize<0 || monSize>3)
        return  -1;

    rw_bufferId = RW_FCAVRX0_OPCODE;
    if (browGetPortNr(browID))
    {
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }

    rw_write_reg(browID, RW_FCDEV_OPCODE, BROW_FB_SIZE_REG, (DDR_SIZE_MARCO | BROW_DISP_AREA));
    usleep(1000);
    rw_write_reg(browID, RW_FCDEV_OPCODE, BROW_COLORKEY_REG, 0x000000F8);
    usleep(1000);
    rw_write_reg(browID, rw_bufferId, BROW_PORT_SETCOLOR_REG, 0x3000000);
    usleep(3000); //taskDelay(2);
    rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_MON0_BASE + monID * 0x100/*ŒàÊÓÆ÷»ùµØÖ·*/ + BROW_MON_CTRL_REG, 0x80000000/*Ê¹ÄÜŒàÊÓÆ÷£¬ÉèÖÃÊôÐÔÎªLVDSœÓ¿Ú¡¢24Î»ÑÕÉ«*/);
    return  0x80000000;
}

unsigned browMonSet(int browID, int monID, int fbID, unsigned layerAddr, int openFlag, int posx, int posy)
{
    unsigned    portAddr, fbCtrlReg, fbAddrReg, fbPosReg, fbAddr, rw_bufferId, monFbAddr;
    
    printf("Enter browMonSet(%d %d)!\n", browID, fbID);
    if ((monID<0) || (monID>7))
    {
        printf("Invalid parameter.\n");
        return  -1;
    }
    portAddr = BROW_MON0_BASE + monID * 0x100; //»ñÈ¡ŒàÊÓÆ÷ŒÄŽæÆ÷»ùµØÖ·
    if (fbID == 1) //»ñÈ¡ÍŒ²ã¿ØÖÆ»ùµØÖ·
    {
        fbCtrlReg = portAddr + BROW_MON_BUFF0_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF0_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF0_POS;
        fbAddr = layerAddr;
    } else if (fbID == 2) {
        fbCtrlReg = portAddr + BROW_MON_BUFF1_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF1_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF1_POS;
        fbAddr = layerAddr;
    } else if (fbID == 3) {
        fbCtrlReg = portAddr + BROW_MON_BUFF2_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF2_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF2_POS;
        fbAddr = layerAddr;
    } else if (fbID == 4) {
        fbCtrlReg = portAddr + BROW_MON_BUFF3_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF3_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF3_POS;
        fbAddr = layerAddr;
    } else {
        printf("Invalid parameter.\n");
        return  -1;
    }
    rw_bufferId = RW_BROWSER_OPCODE;

    //ÉèÖÃÍŒ²ãÊôÐÔ
    if ((openFlag != 1) && (openFlag != 3))
    {
        rw_write_reg(browID, rw_bufferId, fbCtrlReg, 0); //¹Ø±ÕÍŒ²ã
        printf("close browser %1d, layer %1d!\n", browID, fbID);
        return  1;
    }
    monFbAddr = fbAddr;
    printf("browMonSet lap1\n");
    rw_write_reg(browID, rw_bufferId, fbCtrlReg, 0); //¹Ø±ÕÍŒ²ã
    usleep(90000);   //taskdelay(2)
    
    printf("browMonSet lap2\n");
    rw_write_reg(browID, rw_bufferId, fbAddrReg, monFbAddr); //ÉèÖÃŒàÊÓÆ÷µ±Ç°ÍŒ²ã»ùµØÖ·
    usleep(90000);   //taskdelay(2)

    printf("browMonSet lap3\n");
    rw_write_reg(browID, rw_bufferId, fbPosReg, ((posx & 0x3FFF) << 16) | (posy & 0x3FFF));
    usleep(90000);   //taskdelay(2)

    if (openFlag == 3) //ÍŒ²ãÊôÐÔÉèÎªŽò¿ª£¬¹ýÂË¹ŠÄÜŽò¿ª;
        rw_write_reg(browID, rw_bufferId, fbCtrlReg, 0xC0000000);
    else //ÍŒ²ãÊôÐÔÉèÎªŽò¿ª£¬¹ýÂË¹ŠÄÜ¹Ø±Õ;
        rw_write_reg(browID, rw_bufferId, fbCtrlReg, 0x80000000);
    printf("browser %1d layer %1d opened!\n", browID, fbID);
    return  1;
}

unsigned browMonSetPos(int browID, int monID, int fbID, int posx, int posy)
{
    unsigned    portAddr, fbCtrlReg, fbAddrReg, fbPosReg, rw_bufferId, posData;

    if ((monID<0) || (monID>7))
    {
        printf("Invalid parameter.\n");
        return  -1;
    }
    portAddr = BROW_MON0_BASE + monID * 0x100; //»ñÈ¡ŒàÊÓÆ÷ŒÄŽæÆ÷»ùµØÖ·
    if (fbID == 1) //»ñÈ¡ÍŒ²ã¿ØÖÆ»ùµØÖ·
    {
        fbCtrlReg = portAddr + BROW_MON_BUFF0_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF0_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF0_POS;
    }
    else if (fbID == 2) {
        fbCtrlReg = portAddr + BROW_MON_BUFF1_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF1_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF1_POS;
    }
    else if (fbID == 3) {
        fbCtrlReg = portAddr + BROW_MON_BUFF2_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF2_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF2_POS;
    }
    else if (fbID == 4) {
        fbCtrlReg = portAddr + BROW_MON_BUFF3_CTL;
        fbAddrReg = portAddr + BROW_MON_BUFF3_ADDR;
        fbPosReg = portAddr + BROW_MON_BUFF3_POS;
    }
    else {
        printf("Invalid parameter.\n");
        return  -1;
    }
    rw_bufferId = RW_BROWSER_OPCODE;

    //设置显示窗口相对图层的x，y坐标
    posData = (posx & 0x3FFF);
    posData = (posData << 16) | (posy & 0x3FFF);
    rw_write_reg(browID, rw_bufferId, fbPosReg, posData);
    return 1;
}

//设置光标打开关闭和闪烁频率
unsigned browSetCursor(int browID, int monID, int onoff, int tfreq)
{
    unsigned    portAddr = BROW_MON0_BASE + monID * 0x100;  //获取监视器寄存器基地址
    unsigned    tempData;
    unsigned    rw_bufferId = RW_BROWSER_OPCODE;

    if (monID<0 || monID>7)
    {
        printf("Invalid parameter.\n");
        return -1;
    }
    tempData = rw_read_reg(browID, rw_bufferId, portAddr + BROW_MON_CURCTL_REG);    //读取当前光标控制寄存器的值
    if (onoff == 1) //打开光标
        tempData = 0x80000000 | (tfreq & 0xFF);
    else
        tempData &= 0xFF;

    rw_write_reg(browID, rw_bufferId, portAddr + BROW_MON_CURCTL_REG, tempData);
    return  1;
}

int browAvTxCtrl(int browID, unsigned vid, unsigned tgtId, 
        unsigned camX, unsigned camY, unsigned cvPosX, unsigned cvPosY, unsigned cvWidth, unsigned cvHeight)
{
    unsigned tmp, rwID=RW_FCAVTX0_OPCODE;
    printf("Browser 0x%x CAM, %d x %d\n", browID, camX, camY);
    
    tmp=rw_read_reg(browID, rwID, BROW_AVTX_CTRL_REG);
    rw_write_reg(browID, rwID, BROW_AVTX_CTRL_REG, tmp & 0xFFFfff);
    rw_write_reg(browID, rwID, BROW_AVTX_VIDEO_ATTR_REG, 0x414);

    rw_write_reg(browID, rwID, BROW_AVTX_VIDEOID_REG, vid);
    tmp=((camX<<16)|camY);
    rw_write_reg(browID, rwID, BROW_AVTX_VIDEO_SIZE_REG, tmp);
    tmp=((cvPosX<<16)|cvPosY);
    rw_write_reg(browID, rwID, BROW_AVTX_WIN_POS_REG, tmp);
    tmp=((cvWidth<<16)|cvHeight);
    rw_write_reg(browID, rwID, BROW_AVTX_WIN_SIZE_REG, tmp);
    tmp=(0xA0000000|tgtId);
    rw_write_reg(browID, rwID, BROW_AVTX_CTRL_REG, tmp);
    usleep(800);
    printf("start CAM TX 0x%x video OK!\n",browID);
    return rw_read_reg(browID, rwID, BROW_AVTX_CTRL_REG);
}

int browSetVColor(int browID, unsigned red, unsigned green, unsigned blue)
{
    unsigned    portBaseAddr, rw_bufferId, tempData;

    //È±Ê¡Ê¹ÓÃPort0
    portBaseAddr = BROW_PORT0_BASE;
    rw_bufferId = RW_FCAVRX0_OPCODE;

    if (browGetPortNr(browID))
    {
        portBaseAddr = BROW_PORT1_BASE;
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }

    //ÐŽÈëÔ¶³Ìä¯ÀÀÆ÷ÉèÖÃ£šÇå³ý£©ÑÕÉ«ŒÄŽæÆ÷
    tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_SETCOLOR_REG);
    tempData &= 0x03000000;
    tempData |= (((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF));
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_SETCOLOR_REG, tempData);
    return  1;
}

int browSetKeyColor(int browID, int keyRGB)
{
    //设置浏览器透明色：蓝色
    rw_write_reg(browID, RW_FCDEV_OPCODE, BROW_COLORKEY_REG, (keyRGB & 0xFFFFFF));
}

int browClrLayer(int browID, unsigned layAddr)
{
    unsigned    portBaseAddr, rw_bufferId;

    //È±Ê¡Ê¹ÓÃPort0
    portBaseAddr = BROW_PORT0_BASE;
    rw_bufferId = RW_FCAVRX0_OPCODE;

    if (browGetPortNr(browID))
    {
        portBaseAddr = BROW_PORT1_BASE;
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }

    //ÉèÖÃÍŒ²ãµØÖ·
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_DISP_ADDR_REG, layAddr);
    //Çå³ýÍŒ²ã
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG, 0x00000400);
    return  1;
}

static int brow_av_playCtl(int browID, unsigned vid, int start)
{
    unsigned    tempData, loops;
    unsigned    portBaseAddr, rw_bufferId;

    if (vid == 0)
        return -1;

    //È±Ê¡Ê¹ÓÃPort0
    portBaseAddr = BROW_PORT0_BASE;
    rw_bufferId = RW_FCAVRX0_OPCODE;

    if (browGetPortNr(browID))
    {
        portBaseAddr = BROW_PORT1_BASE;
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }
    
    if (start == 0)
    {   //ÉèÖÃÊÓÆµœÓÊÕ¿ØÖÆŒÄŽæÆ÷£¬Æô¶¯FC-AVÐ­ÒéœÓÊÕŽ«Êä
        //ÐŽÈëÒªÍ£Ö¹œÓÊÕµÄÊÓÆµID
        rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_VIDEOID_REG, vid);
        usleep(800);
        //×éºÏ¿ØÖÆ×Ö, ÐŽÈëÔ¶³Ìä¯ÀÀÆ÷ÊÓÆµœÓÊÕÄ£¿é¿ØÖÆŒÄŽæÆ÷
        rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG, AV_RX_STOP_BIT);
        usleep(800);
        //ÅÐ¶ÏÖŽÐÐœá¹û
        tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG);
        loops = 0;
        while (tempData & AV_RX_STOP_BIT)
        {
            usleep(800);
            tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG);
            if (loops++ > 100)
            {
                printf("brow_av_playCtl:stop FC-AV RX videoID 0x%x fail!\n", vid);
                return  -1;
            }
        }
        return  tempData;
    }
    if (start == 1)
    {   //ÐŽÈëÒªÆô¶¯œÓÊÕµÄÊÓÆµID
        rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_VIDEOID_REG, vid);
        usleep(800);

        //ÐŽÈëÔ¶³Ìä¯ÀÀÆ÷ÊÓÆµœÓÊÕÄ£¿é¿ØÖÆŒÄŽæÆ÷
        rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG, AV_RX_START_BIT);
        usleep(800);
        //ÅÐ¶ÏÖŽÐÐœá¹û
        tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG);
        loops = 0;
        while (tempData & AV_RX_START_BIT)
        {
            usleep(800);
            tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG);
            if (loops++ > 100)
            {
                printf("brow_av_playCtl:start FC-AV RX videoID 0x%x fail!\n", vid);
                return  -1;
            }
        }
        return  tempData;
    }
    printf("browser %0xA FC-AV Rx start failed.\n", browID);
    return  -1;
}

FCBROW_VW_PARAM g_WinParam = { 0 };

static int browAvInit(int browID, unsigned dispBaseAddr, unsigned vid, int srcPortId, unsigned posx, unsigned posy, unsigned width, unsigned height)
{
    //³õÊŒ»¯Ô¶³Ìä¯ÀÀÆ÷ÏÔÊŸŽ°¿Ú²ÎÊý
    g_WinParam.videoID = vid;
    g_WinParam.frameBuffer = dispBaseAddr;
    g_WinParam.dispWinPos = posx*65536 + posy;
    g_WinParam.dispWinSize = width*65536 + height;
    if (srcPortId == 0)
        printf("browAvInit: invalid srcPortid!\n");

    g_WinParam.srcPortID = srcPortId;
    g_WinParam.audioFlag = 0;             //È±Ê¡²»œÓÊÕÒôÆµ
    g_WinParam.videoFlag = 1;             //È±Ê¡²»œÓÊÕÊÓÆµ
    return  1;
}

static int browWriteWinParam(int browID, int videoID)
{
    unsigned portBaseAddr, tempData, loops, rw_bufferId;
    unsigned align_buf[8], *pAlignBuf = &align_buf[0];

    if (videoID == 0)
        return -1;

    if ((size_t)pAlignBuf & 7)
        ++pAlignBuf;

    //È±Ê¡Ê¹ÓÃPort0
    portBaseAddr = BROW_PORT0_BASE;
    rw_bufferId = RW_FCAVRX0_OPCODE;

    if (browGetPortNr(browID))
    {
        portBaseAddr = BROW_PORT1_BASE;
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }
    
    memcpy(pAlignBuf, &g_WinParam.videoID, 20);
    printf("browWriteWinParam: writting %p(%x %x %x %x %x) to reg %x.\n", pAlignBuf, 
        pAlignBuf[0], pAlignBuf[1], pAlignBuf[2], pAlignBuf[3], pAlignBuf[4], portBaseAddr + BROW_PORT_VIDEOID_REG);
    rw_write_mem(browID, rw_bufferId, portBaseAddr + BROW_PORT_VIDEOID_REG, pAlignBuf, 20);
    
    usleep(2200);
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG, AV_RX_WINDATA_SET_BIT);

    loops = 0;
    do {
        usleep(2200);
        tempData = rw_read_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG);

        if (!(tempData & AV_RX_WINDATA_SET_BIT))
            return  tempData;
    } while (loops++ < 40);

    printf("browWriteWinParam:save av rx win data fail!BROW_PORT_CTRL_REG=0x%x\n", tempData);
    return -1;
}

int browPlayCtl(int browID, unsigned vid, int srcPortId, int start, unsigned layerAddr, unsigned posx, unsigned posy, unsigned width, unsigned height)
{
    if (vid == 0)
    {
        printf("Invalid parameter.\n");
        return  -1;
    }
    //ÏÈÍ£Ö¹ËùÓÐÊÓÆµœÓÊÕ
    brow_av_playCtl(browID, 0xFFFFFFFF, 0);

    if (start>3 || start<0)
    {
        printf("Invalid parameter.\n");
        return  -1;
    }
    if (start == 0)
    {
        printf("stopped!\n");
        return  1;
    }
    //¶ÔÔ¶³Ìä¯ÀÀÆ÷FC-AVÐ­ÒéœÓÊÕÄ£¿éœøÐÐ³õÊŒ»¯£¬Ê¹ÓÃŽ«ÈëµÄÍŒ²ã»ùÖ·
    browAvInit(browID, layerAddr, vid, srcPortId, posx, posy, width, height);
    //usleep(1500);

    //ÅÐ¶Ï¿ØÖÆÃüÁî
    if (start & 0x01)
    {   //œÓÊÕÊÓÆµ
        //ÉèÖÃÔ¶³Ìä¯ÀÀÆ÷FC_AVÐ­ÒéœÓÊÕÄ£¿éÏÔÊŸŽ°¿ÚÔÚÆÁÄ»ÉÏµÄÎ»ÖÃºÍŽóÐ¡
        //browSetRxDispVW(browID, vid, posx, posy, dvWdith, dvHeight);
        //usleep(1500);
        //ÉèÖÃÔ¶³Ìä¯ÀÀÆ÷FC_AVÐ­ÒéœÓÊÕÄ£¿éÊÓÆµ²ÉŒ¯Ž°¿ÚµÄÎ»ÖÃºÍŽóÐ¡
        //browSetRxCaptureVW(browID,winNo,0,0,dvWdith,dvHeight);
        //taskDelay(2);
    }
    if (start & 0x02)
    {
        //œÓÊÕÒôÆµ
        //browSetAudio(browID, vid);
    }
    //±£ŽæÔ¶³Ìä¯ÀÀÆ÷ÏÔÊŸŽ°¿Ú²ÎÊý
    browWriteWinParam(browID, vid);
    usleep(1500);
    //Æô¶¯ËùÓÐÔ¶³Ìä¯ÀÀÆ÷FC-AVÐ­ÒéœÓÊÕ
    brow_av_playCtl(browID, 0xFFFFFFFF, 1);
    usleep(1500);
    return  1;
}

int browClrRect(int browID, unsigned layAddr, int posx, int posy, int width, int height)
{
    unsigned tempData;
    unsigned portBaseAddr;
    unsigned rw_bufferId;

    portBaseAddr = BROW_PORT0_BASE;
    rw_bufferId = RW_FCAVRX0_OPCODE;

    if (browGetPortNr(browID))
    {
        portBaseAddr = BROW_PORT1_BASE;
        rw_bufferId = RW_FCAVRX1_OPCODE;
    }

    //设置图层地址
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_DISP_ADDR_REG, layAddr);

    //设置远程浏览器显示窗口位置
    tempData = (posx & 0x3FFF);
    tempData = (tempData << 16) | (posy & 0x3FFF);
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_WIN_POS_REG, tempData);

    //设置远程浏览器显示窗口尺寸
    tempData = (width & 0x3FFF);
    tempData = (tempData << 16) | (height & 0x3FFF);
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_WIN_SIZE_REG, tempData);

    //清除显示窗口区域
    rw_write_reg(browID, rw_bufferId, portBaseAddr + BROW_PORT_CTRL_REG, 0x00000200);
    return 1;
}

unsigned myatoi(char* p)
{
    if ((p[0] == '0') && (p[1] == 'x'))
    {
        unsigned i = 2, u, retv = 0;

        while (((p[i] >= '0') && (p[i] <= '9')) || ((p[i] >= 'a') && (p[i] <= 'z')) || ((p[i] >= 'A') && (p[i] <= 'Z')))
        {
            if (p[i] <= '9')
                u = p[i] - '0';
            else if (p[i] <= 'Z')
                u = p[i] - 'A' + 10;
            else
                u = p[i] - 'a' + 10;
            retv = retv * 16 + u;
            ++i;
        }
        return  retv;
    }
    return  atoi(p);
}

FCBROW_COMM_STRUCT g_browCommInfo[16] = { 0 };
FCBROW_IIC_STRUCT g_browIIcInfo[32] = { 0 };
FC_BROWTS_HANDLER g_fc_browTs_handler[8] = { 0 };

void recBrowCommMsg(unsigned char *msgbuf, unsigned buflen, int idx)
{
    int i;
    
    if (!msgbuf || !buflen || (idx >= sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0])))
    {
        printf("recBrowCommMsg: Invalid param!\n");
        return;
    }
    for (i = 0; i < sizeof(g_browCommInfo[idx].comBuf); i++)
    {   //将串口数据拷贝入对应的串口缓冲区
        g_browCommInfo[idx].comBuf[g_browCommInfo[idx].bufDataTail] = msgbuf[(i | 3) - (i & 3)];
        //维护缓冲区尾索引值
        g_browCommInfo[idx].bufDataTail = (g_browCommInfo[idx].bufDataTail + 1) % sizeof(g_browCommInfo[idx].comBuf);

        //维护缓冲区头索引值
        if (g_browCommInfo[idx].bufDataTail == g_browCommInfo[idx].bufDataHead)
            g_browCommInfo[idx].bufDataHead = (g_browCommInfo[idx].bufDataHead + 1) % sizeof(g_browCommInfo[idx].comBuf);
    }
}

// 初始化远程浏览器串口模块，进行热复位，
// 通过对远程浏览器音频模块进行初始化，创建串口数据接收缓冲区
// 设置 波特率，目标端口ID，消息ID基本参数，
int browSerialInit(int browID, int pno, int targetId, unsigned commMsgId)
{
    int rw_Id, comBaseAddr, idx;

    if ((browID <= 0) || (browID>0x00FFFFFF))
        return  -1;

    if (pno == 1)
    {
        comBaseAddr = BROW_COMM1_BASE;
        rw_Id = RW_FCCOM1_OPCODE;
    } else if (pno == 2) {
        comBaseAddr = BROW_COMM2_BASE;
        rw_Id = RW_FCCOM2_OPCODE;
    } else {
        return  -2;
    }

    //初始化远程浏览器串口数据结构
    for (idx = 0; idx < sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0]); idx++)
        if ((g_browCommInfo[idx].browID == browID) && (g_browCommInfo[idx].pno == pno))
            break;

    if (idx >= sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0]))
    {
        for (idx = 0; idx < sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0]); idx++)
            if (!g_browCommInfo[idx].browID)
                break;

        if (idx >= sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0]))
        {
            printf("brow comm buffer FULL!\n");
            return  -1;
        }
        //第一次初始化浏览器串口，结构赋值
        g_browCommInfo[idx].browID = browID;
        g_browCommInfo[idx].pno = pno;
    }
    //注册串口消息ID
    g_browCommInfo[idx].msgID = commMsgId;
    g_browCommInfo[idx].bufDataHead = 0;
    g_browCommInfo[idx].bufDataTail = 0;

    rw_write_reg(browID, rw_Id, comBaseAddr + BROW_COMM_CTRL_OFFSET, 0x40004000);   //复位串口模块发送逻辑和接收逻辑
    usleep(800);
    rw_write_reg(browID, rw_Id, comBaseAddr + BROW_COMM_CTRL_OFFSET, 0x00008000);   //按缺省参数配置串口ASM控制,3类服务,使能接收
    rw_write_reg(browID, rw_Id, comBaseAddr + BROW_COMM_CONFIG_OFFSET, 0xF0F00064); //按缺省参数配置串口属性，115200，无校验，接收消息间隔为100ms
    rw_write_reg(browID, rw_Id, comBaseAddr + BROW_COMM_MSGID_OFFSET, commMsgId);   //设置串口ASM消息标识
    rw_write_reg(browID, rw_Id, comBaseAddr + BROW_COMM_DSTPORT_OFFSET, targetId);  //设置串口ASM消息目标ID

    if (add_asm_cmdHandler(commMsgId, browID, recBrowCommMsg, idx) <0) 
    {   //将串口接收处理函数加入ASM消息处理队列,挂接触摸屏ASM消息处理函数
        printf("Add Brow comm msg ID err!\n");
        return  -1;
    }
    printf("Browser 0x%x start comm %d rec msg OK!\n", browID, pno);
    return  1;
}

// 从远程浏览器串口模块读取字符串函数
// 从本地建立的串口缓冲区中读取字符串，
// 缓冲区中的数据来自于远端浏览器串口模块
int browSerialRead(int browID, int pno, char *msgBuf, int maxLen)
{
    int i, j = 0;

    if (!msgBuf || !maxLen)
    {
        printf("browSerialRead: Invalid param!\n");
        return  -1;
    }
    for (i = 0; i < sizeof(g_browCommInfo) / sizeof(g_browCommInfo[0]); i++)
    {
        if ((g_browCommInfo[i].browID == browID) && (g_browCommInfo[i].pno == pno))
        {   //从对应的串口缓冲区拷贝串口数据
            while (g_browCommInfo[i].bufDataHead != g_browCommInfo[i].bufDataTail)
            {
                msgBuf[j] = g_browCommInfo[i].comBuf[g_browCommInfo[i].bufDataHead];
                g_browCommInfo[i].bufDataHead = (g_browCommInfo[i].bufDataHead + 1) % sizeof(g_browCommInfo[i].comBuf);
                if (++j >= maxLen)
                    break;
            }
            return j;
        }
    }
    printf("Not find indecate browser comm!\n");
    return  -1;
}

// 通过对远程浏览器发送来的IIC接口ASM消息，并放置到对应的接收缓冲区
void recBrowIIcMsg(ASM_HEADER *pShd, unsigned char *msgbuf, unsigned buflen, int idx)
{
    int     i;

    //printf("ENTER recBrowIIcMsg.\n");
    if (!msgbuf || (buflen < 0) || (buflen >= sizeof(g_browIIcInfo[0].rxBuf)) || (idx > sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0])))
    {
        printf("recBrowIIcMsg: Invalid param!\n");
        return;
    }
    g_browIIcInfo[idx].usrDef = pShd->UsrReg;

    for (i = 0; i<buflen; i += 4)
    {   //23.6寸、23.8寸触摸屏消息需要倒字节，14寸、17寸触摸屏消息不需要倒字节, ztp Edited at 20180530
        g_browIIcInfo[idx].rxBuf[i]     = msgbuf[i + 3];
        g_browIIcInfo[idx].rxBuf[i + 1] = msgbuf[i + 2];
        g_browIIcInfo[idx].rxBuf[i + 2] = msgbuf[i + 1];
        g_browIIcInfo[idx].rxBuf[i + 3] = msgbuf[i];
    }
    g_browIIcInfo[idx].dataLen = buflen;
    sem_post(&g_browIIcInfo[idx].rxSem);
}

// 从远程浏览器IIC模块读取IIC数据包函数
// 从本地建立的IIC设备缓冲区中读取字符串，
// 缓冲区中的数据来自于远端浏览器IIC模块
int browIIcRead(int browID, int pno, int *ctrlWord, char *msgBuf, unsigned maxLen)
{
    unsigned        i, j;
    struct timeval  now;
    struct timespec out_time;
    
    if (!msgBuf || !ctrlWord || !maxLen)
    {
        printf("browIIcRead : Invalid param!\n");
        return  -1;
    }
    for (i = 0; i < sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]); i++)
        if ((g_browIIcInfo[i].browID == browID) && (g_browIIcInfo[i].pno == pno))
            break;

    if (i >= sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]))
    {
        printf("browIIcRead: Not find indecate browser!\n");
        return  -1;
    }
    gettimeofday(&now, 0);
    out_time.tv_sec = now.tv_sec + BROW_IIC_WAITTIME;
    out_time.tv_nsec = now.tv_usec * 1000;
    if (sem_timedwait(&g_browIIcInfo[i].rxSem, &out_time))       
        return  0;  //等待超时返回

    if (0 == (*ctrlWord = g_browIIcInfo[i].usrDef))
        return  0;

    for (j = 0; (j < g_browIIcInfo[i].dataLen) && (j < maxLen); j++)
        msgBuf[j] = g_browIIcInfo[i].rxBuf[j];
    
    g_browIIcInfo[i].usrDef = 0;
    return  j;
}

// 初始化远程浏览器IIC接口模块，，
// 控制IIC接口使能或关闭
// 设置对应的ASM消息ID，目标端口ID，消息发送间隔等基本参数，
int browI2cInit(int browID, int I2cNo, int onoff, int targetId, int gap, unsigned txMsgId, unsigned rxMsgId)
{
    int i;

    if ((I2cNo != 1) && (I2cNo != 2))
        return  -2;

    //初始化远程浏览器IIC接口数据结构
    for (i = 0; i < sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]); i++)
        if ((g_browIIcInfo[i].browID == browID) && (g_browIIcInfo[i].pno == I2cNo))
            break;

    if (i >= sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]))
    {
        for (i = 0; i < sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]); i++)
            if (!g_browIIcInfo[i].browID)
                break;

        if (i >= sizeof(g_browIIcInfo) / sizeof(g_browIIcInfo[0]))
        {
            printf("brow comm buffer FULL!\n");
            return  -1;
        }
        //第一次初始化浏览器串口，结构赋值
        g_browIIcInfo[i].browID = browID;
        g_browIIcInfo[i].pno = I2cNo;
        g_browIIcInfo[i].dataLen = 0;

        sem_init(&g_browIIcInfo[i].rxSem, 0, 0);
        sem_init(&g_browIIcInfo[i].txSem, 0, 1);
    }
    //记录浏览器类型,注册IIC接口对应ASM消息ID
    g_browIIcInfo[i].browType = 0;
    g_browIIcInfo[i].txMsgID = txMsgId;
    g_browIIcInfo[i].rxMsgID = rxMsgId;

    onoff = onoff ? 0x80000000 : 0;

    if (I2cNo == 1)
    {   //设置IIC接口模块ASM消息标识
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC1_TX_MSGID_REG, txMsgId);
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC1_RX_MSGID_REG, rxMsgId);
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC1_CTRL_REG, onoff | ((gap & 0x7F) << 24) | (targetId & 0x00FFFFFF));
    } else {
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC2_TX_MSGID_REG, txMsgId);
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC2_RX_MSGID_REG, rxMsgId);
        rw_write_reg(browID, RW_BROWSER_OPCODE, BROW_IIC2_CTRL_REG, onoff | ((gap & 0x7F) << 24) | (targetId & 0x00FFFFFF));
    }
    if (add_asmhead_cmdHandler(rxMsgId, browID, recBrowIIcMsg, i) < 0)
    {   //将IIC消息接收处理函数加入ASM消息处理队列, 挂IIC接口ASM消息处理函数
        printf("Add Brow IIC msg ID err!\n");
        return  -1;
    }
    printf("Browser 0x%x start IIC %d rec msg OK!\n", browID, I2cNo);
    return  1;
}

// 读取触摸屏EXC80送来的数据，解析触摸屏协议数据，
// 转换为屏幕坐标和手指信息，
// 适用于23.8寸浏览器触摸屏的驱动. 返回0表示失败。
static int readEPG030TSMsg(FC_BROWTS_HANDLER *pBrowTs, unsigned char *msgbuf, int buflen, int param1)
{
    int i, j, fingernum;
    
    if (!msgbuf || !pBrowTs)
    {
        printf("readEPG030TSMsg: Invalid param!\n");
        return  0;
    }
    if ((buflen<3) || (buflen>70))
    {
        printf("readEPG030TSMsg: Invalid bufLen!\n");
        return  0;
    }
    if (msgbuf[0] != 0x18)
    {   //判断触摸屏消息头 ReportID
        printf("readEPG030TSMsg: Invalid msg Head!\n");
        return	0;
    }
    //更新手指状态
    fingernum = (msgbuf[1] & 0x0F);
    
    if (fingernum <= 0 || fingernum>5)
    {   //根据fingernum值判断触摸数据有几个包，以及当前是哪个包, 用fingerPkt存贮当前包内有几个手指的数据
        printf("fingernum=0x%x error!\n", fingernum);
        return  0;
    }
    for (i = 0; i < fingernum; i++)
    {
        int         fingerId, fignerStat;
        unsigned    tempX, tempY;

        j = 10 * i + 2;
        fignerStat = msgbuf[10 * i + 2];
        fingerId = msgbuf[10 * i + 3];

        if (fignerStat & 1)
        {   //第几个手指按下，取相应的手指信息
            tempX = (msgbuf[j + 3] & 0x3F);
            tempX = (tempX << 8) + msgbuf[j + 2];
            tempY = (msgbuf[j + 5] & 0x3F);
            tempY = (tempY << 8) + msgbuf[j + 4];

            pBrowTs->touchFinger[fingerId].touchx = tempX;
            pBrowTs->touchFinger[fingerId].touchy = tempY;

            //进行坐标转换, 23.8寸触摸屏原始分辨率为16384*16384，对应屏幕分辨率为2560*1440
            pBrowTs->touchFinger[fingerId].scrx = (tempX*5)/32;
            pBrowTs->touchFinger[fingerId].scry = (tempY*45)/512;
            if (pBrowTs->touchFinger[fingerId].touchdown != 3) //之前此手指的状态不是按下,状态消息计数复位
                pBrowTs->touchFinger[fingerId].msgCnt = 0;

            pBrowTs->touchFinger[fingerId].touchdown = 4;   //更新手指状态,为更新过按下数据的中间状态
        } else {   //第几个手指松开，松开只处理一次, 因为松开时坐标可能不准确，因此延用之前按下时的坐标
            if (pBrowTs->touchFinger[fingerId].touchdown == 3)
            {   //如果之前状态是按下，则更改手指状态为松开未处理。由触摸处理程序处理完后将此状态改为松开已处理
                pBrowTs->touchFinger[fingerId].touchdown = 1;
                pBrowTs->touchFinger[fingerId].msgCnt = 0;
            }
        }
        (pBrowTs->touchFinger[fingerId].msgCnt)++;
    }
    for (i = 0; i<10; i++)
    {   //完整的触摸数据包处理完毕，更新整个触摸数据包
        if (pBrowTs->touchFinger[i].touchdown == 3)
        {   //指头抬起消息丢失，需要特殊处理，将补充一个抬起状态
            pBrowTs->touchFinger[i].touchdown = 1;
            pBrowTs->touchFinger[i].msgCnt = 0;
        } else if (pBrowTs->touchFinger[i].touchdown == 4)
        {   //将正常的按下更新状态
            pBrowTs->touchFinger[i].touchdown = 3;

            //printf("Brow 0x%x Touch finger[%d] touchdown!posx=0x%x,posy=0x%x,scrx=0x%x,scry=0x%x\n",
            //    pBrowTs->browId, i, pBrowTs->touchFinger[i].touchx, pBrowTs->touchFinger[i].touchy,
            //    pBrowTs->touchFinger[i].scrx, pBrowTs->touchFinger[i].scry);
        }
    }
    (pBrowTs->browTsMsgCnt)++;
    return  1;
}

// 初始化远程浏览器触摸屏解析校正任务
#define BROW14_TS_ADDR      0x2A
#define BROW17_TS_ADDR      0x41
#define BROW23_TS_ADDR      0x10
#define BROW238_TS_ADDR     0x2A

static void* browTSHandle(void* ctx)
{
    FC_BROWTS_HANDLER*  pBrowTs = (FC_BROWTS_HANDLER*)ctx;
    int i = 0, msglen = 0, statWord = 0;
    unsigned char msgBuf[80];

    while (pBrowTs)
    {
        if (!pBrowTs->browId || (pBrowTs->browId > 0xFFFFF0))
        {   //未指定触摸屏所在浏览器
            printf("browTSHandle: Invalid browser ID 0x%x\n", pBrowTs->browId);
            usleep(8000);
        }        
        msglen = browIIcRead(pBrowTs->browId, 1, &statWord, msgBuf, 80);    //读取IIC消息

        if (msglen <= 6)
        {
            printf("msgLen too Short!\n");
            continue;
        }
        printf("brow 0x%x IIC1 statWord=0x%x\n", pBrowTs->browId, statWord);
        
        if ((statWord & 0xff) != 0x81)
        {
            printf("brow 0x%x IIC1 ACK not read ACK! statWord=0x%x\n", pBrowTs->browId, statWord);
            continue;
        }
        //解析读到的触摸屏IIC消息, 23.8寸触摸屏，跳过长度字节,略过后面补零的字节
        if (!readEPG030TSMsg(pBrowTs, &msgBuf[2], msglen - 4, 0))
            continue;

        //根据是否挂拉处理程序来判断是否调用处理函数
        //if (pBrowTs->handler != NULL)
        {   //目前只调用处理第一根手指的状态,
            //函数2019年9月26日改为处理最多5根手指，有几根手指按下调几次回调
            for (i = 0; i<5; i++)
            {
                if (pBrowTs->touchFinger[i].touchdown == 3)
                {
                    if (pBrowTs->handler)
                        pBrowTs->handler(pBrowTs->browId, i, 3, pBrowTs->touchFinger[i].scrx, pBrowTs->touchFinger[i].scry);
                    else
                        printf("pseudo_ts_handler: brow %d finger %d pressed @%d,%d\n", pBrowTs->browId, i, pBrowTs->touchFinger[i].scrx, pBrowTs->touchFinger[i].scry);
                } else if (pBrowTs->touchFinger[i].touchdown == 1)
                {
                    if (pBrowTs->handler)
                        pBrowTs->handler(pBrowTs->browId, i, 1, pBrowTs->touchFinger[i].scrx, pBrowTs->touchFinger[i].scry);
                    else
                        printf("pseudo_ts_handler: brow %d finger %d release @%d,%d\n", pBrowTs->browId, i, pBrowTs->touchFinger[i].scrx, pBrowTs->touchFinger[i].scry);
                    pBrowTs->touchFinger[i].touchdown = 0;
                }
            }
        }
        usleep(800);
    }
}

int inputInit(int srcPortID, int msgId)
{
    struct asm_ts_hook_ param;
    int retv, fd;

    param.browPortId = srcPortID;
    param.asmRxId1 = msgId;
    param.asmRxId2 = 0;

    fd = open_dev_by_idx(ASM_DEV_NAME, 0);
    if (fd < 0)
    {
        printf("open /dev/fcasm error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_ASM_TSHOOK, &param);
    close(fd);
    printf("input init OK!\n");
    return  0;
}

// 初始化远程浏览器触摸屏模块，使能触摸屏IIC消息输出，启动触摸屏IIC消息处理函数
int browTSInit(int browFcPortID, int onoff, int targetId, int gap, void(*handle)(int browId, int fingerNr, int stat, int x, int y))
{
    //int         i;
    //pthread_t   ntid;
    struct asm_ts_hook_ param;
    int retv, fd;

    param.browPortId = browFcPortID;
    param.asmRxId1 = 0x80002;
    param.asmRxId2 = 0x80004;

    if (onoff != 1)
    {   //停止远程浏览器IIC通道1消息发送
        rw_write_reg(browFcPortID, RW_BROWSER_OPCODE, BROW_IIC1_CTRL_REG, 0);
        printf("Browser 0x%x stop TOUCH SCREEN TX IIC OK!\n", browFcPortID);
        return  0;
    }
/*
    for (i = 0; i < sizeof(g_fc_browTs_handler) / sizeof(g_fc_browTs_handler[0]); i++)
        if (g_fc_browTs_handler[i].browId == browFcPortID)
            break;

    if (i >= sizeof(g_fc_browTs_handler) / sizeof(g_fc_browTs_handler[0]))
        for (i = 0; i < sizeof(g_fc_browTs_handler) / sizeof(g_fc_browTs_handler[0]); i++)
            if (!g_fc_browTs_handler[i].browId)
                break;

    if (i >= sizeof(g_fc_browTs_handler) / sizeof(g_fc_browTs_handler[0]))
    {
        printf("can't init brow TouchScreen driver!\n");
        return  -1;
    }
    g_fc_browTs_handler[i].browId = browFcPortID;
    g_fc_browTs_handler[i].handler = handle;
    g_fc_browTs_handler[i].param1 = 0;
    g_fc_browTs_handler[i].browTsMsgCnt = 0;
    g_fc_browTs_handler[i].browType = FC_BROW_238;
*/
    rw_write_reg(browFcPortID, RW_BROWSER_OPCODE, BROW_IIC1_CTRL_REG, (0x80000000 | ((gap & 0x7F) << 24) | (targetId & 0x00FFFFFF))); //设置远程浏览器IIC通道1，目的端口标识,消息间隔,启动触摸屏IIC消息转发
    //if (pthread_create(&ntid, 0, browTSHandle, &g_fc_browTs_handler[i]))
    //{
    //    printf("browTSinit: create thread fail!\n");
    //    return  -1;
    //}   
    fd = open_dev_by_idx(ASM_DEV_NAME, 0);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_ASM_TSHOOK, &param);
    close(fd);
    printf("Browser 0x%x Init TOUCH SCREEN TX OK!\n", browFcPortID);
    return  0;
}

int sata_Initial(void)
{
    int retv, fd = open(SATA_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_SATA_INIT, 0);
    close(fd);
    return  retv;
}

int sata_cmd(unsigned cmd, unsigned lba, unsigned lbaHi_secs, void* buf)
{
    int retv, fd = open(SATA_DEV_NAME, O_RDWR);
    struct sata_cmd_ param;

    param.cmd = cmd;
    param.lba = lba;
    param.lbaHi_secs = lbaHi_secs;
    param.prd_size = 32;
    param.rpt = 1;
    param.buf = buf;

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_SATA_CMD, &param);
    close(fd);
    return  retv;
}

int common_Initial(int rxSeqCnt, int rxSeqBytes, int cls, int txSeqCnt, int txSeqBytes)
{
    struct common_init_param_ param;
    int retv, fd = open(COMMON_DEV_NAME, O_RDWR);

    if(fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    param.rxSeqCnt = rxSeqCnt;
    param.rxSeqBytes = rxSeqBytes;
    param.txSeqCnt = txSeqCnt;
    param.txSeqBytes = txSeqBytes;
    param.cls = cls;
    retv = ioctl(fd, DIOC_CMN_INIT, &param);
    close(fd);
    return  retv;
}

int common_Transfer(CMN_HEADER *phd, const char* buff, int len)
{
    struct common_xfer_desc_ param;
    int retv, fd = open(COMMON_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    memcpy(&param.ah, phd, sizeof(*phd));
    param.buf = (void*)buff;
    param.bufBytes = len;
    retv = ioctl(fd, DIOC_CMN_XFER, &param);
    close(fd);
    return  retv;
}

struct common_protocol_thread_param_
{
    unsigned ctl_code;
    unsigned volatile retv;
    struct common_add_grp_ param;
};

static void* common_protocol_thread(void* ctx)
{
    int fd;
    struct common_protocol_thread_param_* param = (struct common_protocol_thread_param_*)ctx;

    pthread_detach(pthread_self());
    fd = open(COMMON_DEV_NAME, O_RDWR);
    if (fd >= 0)
    {
        param->retv = ioctl(fd, param->ctl_code, &param->param);
        close(fd);
    }
    return 0;
}

static void inthdl(int param)
{
    printf("SIGINT received on %lx!\n", pthread_self());
    pthread_exit(0);
}

static void install_sighdl(void)
{
   struct sigaction act;

    //g_main_tid = pthread_self();

    act.sa_handler = inthdl;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}

static int common_OpGrp(struct common_protocol_thread_param_* p, unsigned grpID, unsigned portID)
{
    int i = 100;
    pthread_t   ntid;

    p->retv = -1;
    p->param.grpID = grpID;
    p->param.portID = portID;

    if (0 != pthread_create(&ntid, 0, common_protocol_thread, p))
    {
        printf("ulib create thread error.\n");
        return -1;
    }
    while (i--)
    {
        if (p->retv != -1)
            return p->retv;
        usleep(10000);
    }
    printf("ulib timeout.\n");
    pthread_kill(ntid, SIGINT);
    return  -1;
}

int common_AddGrp(unsigned grpID, unsigned portID)
{
    static struct common_protocol_thread_param_ param = {DIOC_CMN_ADDGRP};

    return common_OpGrp(&param, grpID, portID);
}

int common_ExitGrp(unsigned grpID, unsigned portID)
{
    static struct common_protocol_thread_param_ param = {DIOC_CMN_EXTGRP};

    return common_OpGrp(&param, grpID, portID);
}

int common_QueryGrp(unsigned grpID)
{
    static struct common_protocol_thread_param_ param = {DIOC_CMN_QRYGRP};

    return common_OpGrp(&param, grpID, 0);
}

int common_waitio(unsigned char* buf, unsigned bufsize)
{   //return bytes received, <=0 if fail.
	struct common_wait_io_ param = {buf, bufsize};
    int retv, fd = open(COMMON_DEV_NAME, O_RDWR);

    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_CMN_WAITIO, &param);
    close(fd);

    if (retv >= 0)
        return param.bytes;

    return 0;
}

int ddr_init()
{
    char    dev_name[32];
    int     fd, retv, idx;

    for (idx = 0; idx < MAX_CARD; ++idx)
    {
        if ((fd = open_dev_by_idx(CARD_DEV_NAME, idx)) < 0)
        {
            if (!idx)
                printf("open dev error: %d\n", fd);
            return fd;
        }
        retv = ioctl(fd, DIOC_DDR_INIT, 0);
        close(fd);
    }
    return  retv;
}

int fcip_init(int mtu)
{
    int     retv, fd = open_dev_by_idx(CARD_DEV_NAME, 0);

    if (fd < 0)
    {
        printf("open dev error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_IP_INIT, &mtu);
    close(fd);
    return  retv;
}

//返回0表示没跨，非0表示跨了(此时返回值表示最多可访问的字节数)
static unsigned cross4G(unsigned addrLo, unsigned bytes)
{
    return (bytes <= -addrLo) ? 0 : -addrLo;
}

int axi_dtm(unsigned long long host_addr, unsigned long long axi_addr, unsigned bytes)
{
    struct   axi_dma_ param;
    char     dev_name[32];
    int      fd, retv;
    unsigned sub = cross4G((unsigned)host_addr, bytes), sub2 = 0;

    if (sub)                                       //sub不为0表示至少分成2段了
        sub2 = cross4G((unsigned)axi_addr, sub);   //sub2不为0表示分成3段了
    else
        sub = cross4G((unsigned)axi_addr, bytes);  

    sprintf(dev_name, CARD_DEV_NAME, 0);
    fd = open(dev_name, O_RDWR);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    param.host_addr = host_addr;
    param.axi_addr = axi_addr;
    if (sub2)
    {
        param.bytes = sub2;
        ioctl(fd, DIOC_AXI_DTM, &param);
        param.host_addr= host_addr+ (unsigned long long)sub2;
        param.axi_addr = axi_addr + (unsigned long long)sub2;
        sub -= sub2;
    }
    if (sub)
    {
        param.bytes = sub;
        ioctl(fd, DIOC_AXI_DTM, &param);
        param.host_addr= host_addr+ (unsigned long long)sub;
        param.axi_addr = axi_addr + (unsigned long long)sub;
        bytes -= sub;
    }
    param.bytes = bytes;
    retv = ioctl(fd, DIOC_AXI_DTM, &param);
    close(fd);
    return  retv;
}

int axi_mtd(unsigned long long host_addr, unsigned long long axi_addr, unsigned bytes)
{
    struct axi_dma_ param;
    char    dev_name[32];
    int     fd, retv;
    unsigned sub = cross4G((unsigned)host_addr, bytes), sub2 = 0;

    if (sub)                                       //sub不为0表示至少分成2段了
        sub2 = cross4G((unsigned)axi_addr, sub);   //sub2不为0表示分成3段了
    else
        sub = cross4G((unsigned)axi_addr, bytes);

    sprintf(dev_name, CARD_DEV_NAME, 0);
    fd = open(dev_name, O_RDWR);
    if (fd < 0)
    {
        printf("open file error: %d\n", fd);
        return  -1;
    }
    param.host_addr = host_addr;
    param.axi_addr = axi_addr;
    if (sub2)
    {
        param.bytes = sub2;
        ioctl(fd, DIOC_AXI_MTD, &param);
        param.host_addr= host_addr+ (unsigned long long)sub2;
        param.axi_addr = axi_addr + (unsigned long long)sub2;
        sub -= sub2;
    }
    if (sub)
    {
        param.bytes = sub;
        ioctl(fd, DIOC_AXI_MTD, &param);
        param.host_addr= host_addr+ (unsigned long long)sub;
        param.axi_addr = axi_addr + (unsigned long long)sub;
        bytes -= sub;
    }
    param.bytes = bytes;    
    retv = ioctl(fd, DIOC_AXI_MTD, &param);
    close(fd);
    return  retv;
}

unsigned get_usec_elapsed(struct timespec* pBgn)
{
    struct timespec end;
    unsigned long long b, e;
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    b = (unsigned long long)(pBgn->tv_sec) * 1000000000ULL + (unsigned long long)(pBgn->tv_nsec);
    e = (unsigned long long)(end.tv_sec) * 1000000000ULL + (unsigned long long)(end.tv_nsec);

    return (unsigned)((e - b)/1000ULL);
}

static char g_pci_reg_path[256];

static FILE* open_reg_by_name(const char* name, const char* mode)
{
    char  filename[288];
    FILE* fp;

    sprintf(filename, "%s/card_r%s", g_pci_reg_path, name);
    return fopen(filename, mode);
}

const char* nr_to_name(unsigned regNr)
{
    static char  name[16];
    sprintf(name, "%02x", regNr);
    return name;
}

unsigned read_reg(const char* name)
{
    char  buf[32] = { 0 }, *p;
    FILE* fp = open_reg_by_name(name, "rb");

    if (fp)
    {
        fread(buf, 1, sizeof(buf), fp);
        fclose(fp);

        if (0 != (p = strstr(buf, "= ")))
            return myatoi(p+2);
    }
    return  0;
}

void write_reg(const char* name, unsigned val)
{
    char  buf[16];
    FILE* fp = open_reg_by_name(name, "wb");

    if (fp)
    {
        sprintf(buf, "0x%08x", val);
        fwrite(buf, 10, sizeof(buf), fp);
        fclose(fp);
    }
}

unsigned read_reg_any(unsigned regNr)
{
    write_reg("Addr", regNr);
    return read_reg("Data");
}

unsigned write_reg_any(unsigned regNr, unsigned val)
{
    write_reg("Addr", regNr);
    write_reg("Data", val);
}

//return 0 if FC4000A, 1 if browser(asic_ver2).
static int is_asic_ver2(void)
{
    FILE* fp = open_reg_by_name("900", "rb");

    if (!fp)
        return 0;
    fclose(fp);
    return 1;
}

int get_fc_pci_reg_path(void)
{
	DIR*	        dir;
	struct dirent*	dir_ent;

    strcpy(g_pci_reg_path, "/sys/module/fcve/drivers/pci:myFc2V/");

	if (0 == (dir = opendir(g_pci_reg_path)))
    {
        printf("open directory fail!\n");
        return	-1;
    }
    while (0 != (dir_ent = readdir(dir)))
    {
        if (!memcmp(dir_ent->d_name, "0000:", 5))
        {
            strcpy(g_pci_reg_path + strlen(g_pci_reg_path), dir_ent->d_name);
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return -1;
}

int global_reset(void)
{
    int retv, fd = open_dev_by_idx(CARD_DEV_NAME, 0);

    if(fd < 0)
    {
        printf("open /dev/fcve0 error: %d\n", fd);
        return fd;
    }
    retv = ioctl(fd, DIOC_GLB_RST, 0);
    close(fd);
    return  retv;
}
