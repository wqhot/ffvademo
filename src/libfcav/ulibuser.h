#include <time.h>
#include "ulib.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int rawIpSend(unsigned srcPortNr, unsigned portId, unsigned char* buf, unsigned bytes);
int ddr_init(void);
int fcip_init(int mtu);
int axi_dtm(unsigned long long host_addr, unsigned long long axi_addr, unsigned bytes);
int axi_mtd(unsigned long long host_addr, unsigned long long axi_addr, unsigned bytes);
int getNPortId(int pno); //��ȡ�˿ڱ�ʶ
void port_broadcast_enable(unsigned pno);
void port_broadcast_disable(unsigned pno);
int FindFullName(int pno, unsigned pid, char *nname, char *pname);
unsigned FindPortID2(int pno, char *pPortName, char *pNodeName);
int setNNodeName(int pno, char *pNodeName);
int setNPortName(int pno, char *pPortName);

int asm_Initial(int rxSeqCnt, int rxSeqBytes, int cls, int txSeqCnt, int txSeqBytes);
int asm_Port_Set(int pno);
int asm3_Transfer(ASM_HEADER *phd, const char* buff, int len);
int asm2_Transfer(ASM_HEADER *phd, const char* buff, int len);
int del_asm_cmdHandler(unsigned asmId, unsigned srcPortId);
int add_asm_cmdHandler(unsigned asmId, unsigned srcPortId, void(*handler)(unsigned char*, unsigned, int), int param1);
int add_asmhead_cmdHandler(unsigned asmId, unsigned srcPortId, void(*handler)(ASM_HEADER*, unsigned char*, unsigned, int), int param1);
unsigned char* map_asm_dma_buf(int card_idx, unsigned bytes);

int rw_Initial(void);
int rw_free(void);
int rw_Add_Buff(char* buff_addr, int buff_size, unsigned buff_id, int buff_spec);
int rw_Add_BuffOnDev(unsigned long long addr_ondev, int buff_size, unsigned buff_id, int buff_spec);
int rw_PortRead(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len, int intFlag);
int rw_PortWrite(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len, int intFlag);
int rw_read_mem(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len);
int rw_read_reg(unsigned port_id, unsigned buff_id, unsigned ofs);
int rw_write_mem(unsigned port_id, unsigned buff_id, unsigned ofs, void* buf, int len);
int rw_write_reg(unsigned port_id, unsigned buff_id, unsigned ofs, unsigned reg_val);
int rw_cmd_arr(unsigned port_id, unsigned buff_id, RW_CMD* cmds);
int rw_lock(unsigned port_id, unsigned buff_id);
int rw_unlock(unsigned port_id, unsigned buff_id);

int fc1553_Initial(int sub_cnt, int share_bytes);
int fc1553_add_sub(unsigned sub_id, void* buf, unsigned sub_size, unsigned sub_spec);
int fc1553_del_sub(unsigned sub_id);
int fc1553_msg_send(int nt1, int nt1Sub, int nt2, int nt2Sub, int ctrl, int bytes);
int fc1553_lst_send(struct b1553_cmd_tbl_* tbl);
int fc1553_blk_send(struct b1553_cmd_tbl_* tbl);

int av_tx_init(void);
int av_tx_reset(void);
int av_tx_ctrl(unsigned vid, unsigned start, unsigned freq, unsigned targetId, unsigned colorAtr, unsigned posx, unsigned posy, unsigned width, unsigned height, void* memAddr);
int av_rx_ctrl(unsigned vid, unsigned start, unsigned srcId, unsigned posx, unsigned posy, unsigned width, unsigned height, unsigned clrBits, void* memAddr);
int av_cp_ctrl(unsigned vid, unsigned start, unsigned freq, unsigned targetId, unsigned colorAtr, unsigned posx, unsigned posy, unsigned width, unsigned height, void* memAddr);
unsigned char* map_txbuf(unsigned bytes);
unsigned char* map_videobuf(unsigned phyAddr, unsigned bytes);
unsigned char* map_rwbuf(unsigned bytes);
unsigned get_rwbuf_phy(unsigned char* buf);
void unmap_txbuf(void* buf);
void unmap_rwbuf(void* buf);
int add_avrx_handler(void (*handler)(void*), unsigned vid);
int del_avrx_handler(int);

int browAvTxCtrl(int browID, unsigned vid, unsigned tgtId, unsigned camX, unsigned camY, unsigned cvPosX, unsigned cvPosY, unsigned cvWidth, unsigned cvHeight);
int browSetKeyColor(int browID, int keyRGB);
int browMonInit(int browID, int monID, int monSize);
int browMonSet(int browID, int monID, int fbID, unsigned layerAddr, int openFlag, int posx, int posy);
unsigned browMonSetPos(int browID, int monID, int fbID, int posx, int posy);
unsigned browSetCursor(int browID, int monID, int onoff, int tfreq);
int browSetVColor(int browID, unsigned red, unsigned green, unsigned blue);
int browClrLayer(int browID, unsigned layAddr);
int browClrRect(int browID, unsigned layAddr, int posx, int posy, int width, int height);
int browPlayCtl(int browID, unsigned vid, int srcPortId, int start, unsigned layerAddr, unsigned posx, unsigned posy, unsigned dvWdith, unsigned dvHeight);
unsigned myatoi(char* p);
unsigned get_usec_elapsed(struct timespec* pBgn);

int browI2cInit(int browID, int I2cNo, int onoff, int targetId, int gap, unsigned txMsgId, unsigned rxMsgId);
int browTSInit(int browID, int onoff, int targetId, int gap, void (*hdl)(int browId, int fingerNr, int stat, int x, int y));
int inputInit(int srcPortID, int msgId);

int sata_Initial(void);
int sata_cmd(unsigned cmd, unsigned lba, unsigned lbaHi_secs, void* buf);

int common_Initial(int rxSeqCnt, int rxSeqBytes, int cls, int txSeqCnt, int txSeqBytes);
int common_Transfer(CMN_HEADER *phd, const char* buff, int len);
int common_AddGrp(unsigned grpID, unsigned portID);
int common_ExitGrp(unsigned grpID, unsigned portID);
int common_QueryGrp(unsigned grpID);
int common_waitio(unsigned char* buf, unsigned bufsize);
int global_reset(void);

int get_fc_pci_reg_path(void);
const char* nr_to_name(unsigned regNr);
unsigned read_reg(const char* name);
void write_reg(const char* name, unsigned val);
unsigned read_reg_any(unsigned regNr);
unsigned write_reg_any(unsigned regNr, unsigned val);
#ifdef __cplusplus
}
#endif  /* __cplusplus */
