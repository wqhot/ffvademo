//视频接收控制寄存器AV_RX_CTRL_REG位定义
#define AV_RX_START_BIT                         0x80000000  //启动接收控制位
#define AV_RX_STOP_BIT                          0x40000000  //停止接收控制位
#define AV_RX_WINDATA_SET_BIT                   0x20000000  //设置显示参数存储器控制位
#define AV_RX_WINDATA_CLR_BIT                   0x10000000  //清除显示参数存储器控制位
#define AV_RX_WINDATA_RESORT_BIT                0x08000000  //初始化读参数顺序控制位
#define AV_RX_WINDATA_READ_BIT                  0x04000000  //读显示窗口参数控制位
#define AV_RX_WINDATA_OK_BIT                    0x02000000  //读参数就绪标志

#define RW_FCPORT0_OPCODE                        0xFFFF0000      //FC端口0存储器标识
#define RW_FCPORT1_OPCODE                        0xFFFF0001      //FC端口1存储器标识
#define RW_FCDEV_OPCODE                          0xFFFF0100      //FC设备公共参数寄存器存储器标识
#define RW_FCCOM1_OPCODE                         0xFFFF0110      //串口1存储器标识
#define RW_FCCOM2_OPCODE                         0xFFFF0111      //串口2存储器标识
#define RW_FCIIC_OPCODE                          0xFFFF0120      //IIC存储器标识
#define RW_FCSPI_OPCODE                          0xFFFF0130      //SPI存储器标识
#define RW_FCAC97_OPCODE                         0xFFFF0140      //AC97存储器标识
#define RW_FCCAN_OPCODE                          0xFFFF0150      //CAN存储器标识
#define RW_FCFLEXRAY_OPCODE                      0xFFFF0160      //FlexRay存储器标识
#define RW_FCMIC_OPCODE                          0xFFFF0170      //MIC存储器标识
#define RW_FCETHERNET_OPCODE                     0xFFFF0180      //以太网存储器标识
#define RW_FCUSB_OPCODE                          0xFFFF0190      //USB存储器标识
#define RW_FCIO_OPCODE                           0xFFFF01A0      //IO存储器标识
#define RW_BROWSER_VIDEOSENSE_OPCODE             0xFFFF01B0      //浏览器视频传感器存储器标识       
#define RW_FCAVRX0_OPCODE                        0xFFFF0400      //FC-AV接收模块存储器标识
#define RW_FCAVRX1_OPCODE                        0xFFFF0401      //FC-AV接收模块存储器标识
#define RW_FCAVTX_OPCODE                         0xFFFF0410      //FC-AV发送模块存储器标识
#define RW_FCAVTX0_OPCODE                        0xFFFF0410
#define RW_FCDSTX_OPCODE                         0xFFFF0420      //数据流发送存储器标识
#define RW_FCDSRX_OPCODE                         0xFFFF0421      //数据流接收存储器标识
#define RW_FCCAM_OPCODE                          0xFFFF0422      //摄像机采集模块存储器标识
#define RW_FCSATA_OPCODE                         0xFFFF0423      //SATA接口模块存储器标识
#define RW_BROWSER_OPCODE                        0xFFFF0800      //浏览器（视频接口）远程存储器标识
#define RW_FCRAM_OPCODE                          0xFFFF1000      //共享存贮器存储器标识



//定义远程浏览器各图层基地址
#define BROW_FB0_BUFFER       0x00000000
#define BROW_FB1_BUFFER       0x04000000
#define BROW_FB2_BUFFER       0x80000000
#define BROW_FB3_BUFFER       0x84000000
#define BROW_FB_SIZE          0x10000000
#define BROW_LAYER_SIZE       0x04000000                  //每个图层占用64M ,

#define BROW_CURSOR_BUFFER    0x58000000
#define BROW_CURSOR_SIZE      0x00010000

#define DDR1_ADDR_MARCO     0x00000000
#define DDR2_ADDR_MARCO     0x80000000
#define DDR_SIZE_MARCO      0x40000000                  //每条DDR2芯片1024M ,

#define BROW_DISP_AREA      0x1E0010E0                  //7680X4320


//浏览器端口0寄存器RW地址
#define BROW_PORT0_BASE                 0x0
//浏览器端口1寄存器RW地址
#define BROW_PORT1_BASE                 0x0
//浏览器端口2寄存器RW地址
#define BROW_PORT2_BASE                 0x0
//浏览器端口3寄存器RW地址
#define BROW_PORT3_BASE                 0x0

//浏览器端口接收模块寄存器RW地址偏移
#define BROW_PORT_VIDEOID_REG                0x00              //浏览器端口FC_AV接收视频接收标识寄存器
#define BROW_PORT_DISP_ADDR_REG              0x04              //浏览器端口FC_AV接收图层地址寄存器
#define BROW_PORT_WIN_POS_REG                0x08              //浏览器端口FC_AV接收图形窗口位置寄存器
#define BROW_PORT_WIN_SIZE_REG               0x0c              //浏览器端口FC_AV接收图形窗口尺寸寄存器
//#define BROW_PORT_SAMPLEVD_POS_REG           0x10              //浏览器端口视频接收采集窗口位置寄存器
//#define BROW_PORT_SAMPLEVD_SIZE_REG          0x14              //浏览器端口视频接收采集窗口尺寸寄存器
#define BROW_PORT_AVATTR_REG                 0x10              //浏览器端口FC_AV接收AV参数寄存器
#define BROW_PORT_CTRL_REG                   0x14              //浏览器端口FC_AV接收控制寄存器
#define BROW_PORT_SETCOLOR_REG               0x18              //浏览器端口FC_AV接收清除设置颜色寄存器
#define BROW_PORT_VFRAMECNT_REG              0x1C              //浏览器端口FC_AV接收AV容器计数器
#define BROW_PORT_VFRAMELOST_REG             0x20              //浏览器端口FC_AV接收AV容器丢失计数器

#define BROW_PORT_LINKERR_CNT                0x40              //浏览器端口链路失败计数器
#define BROW_PORT_SYNLOST_CNT                0x44              //浏览器端口同步丢失计数器
#define BROW_PORT_SIGNALLOST_CNT             0x48              //浏览器端口信号丢失计数器
#define BROW_PORT_PRIMERR_CNT                0x4C              //浏览器端口原语错误计数器
#define BROW_PORT_INVAWORD_CNT               0x50              //浏览器端口无效传输字计数器
#define BROW_PORT_CRCERR_CNT                 0x4C              //浏览器端口CRC错误计数器


//定义浏览器尺寸类型
typedef enum
{
    FC_BROW_140,
    FC_BROW_170,
    FC_BROW_238,
    FC_BROW_236
}FC_BROWSER_TYPE;
//定义浏览器属性结构
typedef struct{

    int isBrow;              	//远程浏览器的FC端口ID
    FC_BROWSER_TYPE   browType;     //远程浏览器型号
    int init_flag;
}FCBROW_INFO_STRUCT;

#define FCBROW_MAX_NUM		32		//最大初始化操作浏览器数量
////////////////////////////////////////
//      浏览器视频接收窗口参数结构
////////////////////////////////////////
#define FCBROW_VW_NUM   512
//定义浏览器图层结构
typedef struct{

    unsigned   portID;              //远程浏览器的FC端口ID
    unsigned   frameBuffer;            //图层基地址
    unsigned   memSize;          //图层所用内存大小
    unsigned   fbSize;          //图层长宽属性，此值为所有图层必须一致           
    unsigned   fbAttr : 24;              //图层属性,包括显示不显示,是否激活等属性
    unsigned   fbLayer : 8;              //图层显示层次，光标层为最高层0层，其下依次1层、2层、3层、4层，5层以下为不显示层        
    unsigned   monPos;             //图层显示窗口左上角座标,应小于图层大小，在不显示时此字段无效
    unsigned   monSize;             //图层显示窗口长宽属性，应小于图层大小，在不显示时此字段无效
    //struct list_head  fbList;      //显示图层链表        
}FCBROW_FB_STRUCT;
//定义远程浏览器显示窗口结构
typedef struct{

    unsigned   portID;                 //远程浏览器端口ID        
    unsigned   videoID;                //显示窗口接收的视频标识           
    unsigned   frameBuffer;            //图层基地址
    unsigned   dispWinPos;             //相对于图层基地址显示窗口左上角座标
    unsigned   dispWinSize;          //显示窗口长宽
    unsigned   srcPortID : 24;           //发送视频标识的源端口ID
    unsigned   reserv1 : 6;
    unsigned   audioFlag : 1;            //接收音频
    unsigned   videoFlag : 1;            //接收视频        
    FCBROW_FB_STRUCT *pFb;          //显示窗口所在的图层指针
    //struct list_head  winList;      //显示窗口链表 
}FCBROW_VW_PARAM;

//定义远程浏览器视频发送控制数据结构
typedef struct{

    unsigned   videoID;                //显示窗口发送的视频标识           
    unsigned   scrSize;                //视频信号尺寸
    unsigned   captWinPos;             //从视频数据流中采集窗口左上角座标相
    unsigned   captWinSize;          //采集窗口长宽
    unsigned   dstPortID : 24;           //发送视频标识的目的端口ID
    unsigned   reserv1 : 2;
    unsigned   audioFlag : 1;            //接收音频
    unsigned   videoFlag : 1;            //接收视频        
    unsigned   reserv2 : 3;
    unsigned   videoStart : 1;            //AV启动传输
    unsigned   audioID;                //单音频发送时的音频标识         
    unsigned   audioPortID : 24;           //发送视频标识的目的端口ID
    unsigned   reserv3 : 7;
    unsigned   audioStart : 1;            //音频启动传输
    //struct list_head  winList;      //显示窗口链表
    unsigned   browPortID;             //远程浏览器端口ID       
}FCBROW_AVTX_PARAM;

//定义远程浏览器光标数据结构
typedef struct{
    int width;                   //图像宽度1-256，以像素为单位           
    int height;                   //图像高度1-256，以像素为单位           
    int xhotspot;      //热点x坐标
    int yhotspot;      //热点y坐标        
    unsigned   colorIndex[16];         //光标颜色索引表           
    char    *pDatabuf;              //光标图形缓冲区指针 ， 要求8字节对齐
    int     bufLen;                 //光标图形缓冲区长度

}STRUCT_FCBROW_CURSOR;

//浏览器键盘模块寄存器RW地址
//#define BROW_KEYB_BASE                 0x0400
#define BROW_KEYB_BASE                 0x0

#define BROW_KEYB_MSGID_REG_            BROW_KEYB_BASE + 0x00              //浏览器键盘按键ASM消息标识寄存器
#define BROW_KEYB_DSTPORT_REG           BROW_KEYB_BASE + 0x04              //浏览器键盘消息目的端口标识寄存器
#define BROW_KEYB_CTRL_REG              BROW_KEYB_BASE + 0x08              //浏览器键盘模块控制寄存器
#define BROW_KEYB_LED_REG               BROW_KEYB_BASE + 0x10              //浏览器键盘指示灯寄存器


//浏览器串口模块寄存器RW地址
//#define BROW_COMM1_BASE                0x0420
//#define BROW_COMM2_BASE                0x0430

#define BROW_COMM1_BASE                0x00
#define BROW_COMM2_BASE                0x00
#define BROW_COMM_MSGID_OFFSET         0x00              //浏览器串口ASM消息标识寄存器偏移
#define BROW_COMM_DSTPORT_OFFSET       0x04              //浏览器串口接收消息目的端口标识寄存器偏移
#define BROW_COMM_CONFIG_OFFSET        0x08              //浏览器串口配置寄存器偏移
#define BROW_COMM_CTRL_OFFSET          0x0C              //浏览器串口控制寄存器偏移

#define BROW_COMM_SETBAUDRATE           0x10001
#define BROW_COMM_SETPARITY             0x10002
#define BROW_COMM_SETPERIOD             0x10003
#define BROW_COMM_SETFCTYPE             0x20001
//定义远程浏览器串口发送控制数据结构
typedef struct {
    unsigned    browID;                 //远程浏览器端口号           
    int         pno;                    //远程浏览器串口号       
    unsigned long long browNodeName;    //远程浏览器节点名
    unsigned long long browPortName;    //远程浏览器端口名
    unsigned    msgID;                  //远程浏览器串口1ASM消息ID
    char        comBuf[512];
    int         bufDataHead;
    int         bufDataTail;
}FCBROW_COMM_STRUCT;

//浏览器FC-AV发送模块寄存器RW地址偏移 
//#define BROW_AVTX_BASE                0x0440
#define BROW_AVTX_VIDEOID_REG         0x00              //浏览器FC_AV发送输出AV标识寄存器
#define BROW_AVTX_VIDEO_SIZE_REG      0x04              //浏览器FC_AV发送信号尺寸址寄存器
#define BROW_AVTX_WIN_POS_REG         0x08              //浏览器FC_AV发送输出窗口位置寄存器
#define BROW_AVTX_WIN_SIZE_REG        0x0c              //浏览器FC_AV发送输出窗口尺寸寄存器
#define BROW_AVTX_CTRL_REG            0x10              //浏览器FC_AV发送控制寄存器
#define BROW_AVTX_AUDIO_ID_REG        0x14              //浏览器FC_AV发送输出音频标识寄存器
#define BROW_AVTX_AUDIO_CTRL_REG      0x18              //浏览器FC_AV音频发送控制寄存器
#define BROW_AVTX_AUDIO_LEN_REG       0x1C              //浏览器FC_AV音频发送长度寄存器
#define BROW_AVTX_VIDEO_ATTR_REG      0x20

#define BROW_COMMSET_BASE               0x0
#define BROW_FB_SIZE_REG              BROW_COMMSET_BASE+0x0            //显示区尺寸寄存器
#define BROW_COLORKEY_REG             BROW_COMMSET_BASE+0x04            //透明色编码寄存器
#define BROW_LINKSTAT_REG             BROW_COMMSET_BASE+0x08            //浏览器状态寄存器
#define BROW_ASSERT_MSGID_REG         BROW_COMMSET_BASE+0x0C            //通告消息标识寄存器
#define BROW_ASSERT_TGTID_REG         BROW_COMMSET_BASE+0x010            //通告消息接收端ID寄存器

//浏览器音频控制模块寄存器RW基地址
//#define BROW_AUDIO_BASE                0x0500
#define BROW_AUDIO_BASE                0x0000
#define BROW_AUDIO_CTRL_REG              BROW_AUDIO_BASE + 0x80                 //浏览器音频输入FC发送控制寄存器
#define BROW_AUDIO_CODECRESET_REG        BROW_AUDIO_BASE + 0x00              //远程浏览器音频编解码器（Codec）复位寄存器
#define BROW_AUDIO_VOLUME_REG            BROW_AUDIO_BASE + 0x04              //远程浏览器输出主音量寄存器
#define BROW_AUDIO_MONOVOLUME_REG        BROW_AUDIO_BASE + 0x0C              //远程浏览器MONO输出音量寄存器
#define BROW_AUDIO_BEEPVOLUME_REG        BROW_AUDIO_BASE + 0x14              //远程浏览器蜂鸣器输入音量寄存器

#define BROW_AUDIO_PHONEVOLUME_REG       BROW_AUDIO_BASE + 0x18              //远程浏览器电话输入音量寄存器
#define BROW_AUDIO_MICVOLUME_REG         BROW_AUDIO_BASE + 0x1C              //远程浏览器麦克风输入音量寄存器
#define BROW_AUDIO_LINEINVOLUME_REG      BROW_AUDIO_BASE + 0x20              //远程浏览器线性输入音量（Line In Volume）寄存器
#define BROW_AUDIO_CDINVOLUME_REG        BROW_AUDIO_BASE + 0x24              //远程浏览器CD输入音量（CD Volume）寄存器
#define BROW_AUDIO_PCMOUTVOLUME_REG      BROW_AUDIO_BASE + 0x30              //远程浏览器PCM Out Volume寄存器
#define BROW_AUDIO_RECORDSEL_REG         BROW_AUDIO_BASE + 0x34              //远程浏览器录音选择（Record Select）寄存器
#define BROW_AUDIO_RECORDGAIN_REG        BROW_AUDIO_BASE + 0x38              //远程浏览器录音增益（Record Gain）寄存器
#define BROW_AUDIO_GENERAL_REG           BROW_AUDIO_BASE + 0x40              //远程浏览器General Purpose寄存器
#define BROW_AUDIO_3DCTRL_REG            BROW_AUDIO_BASE + 0x44              //远程浏览器3D Control寄存器
#define BROW_AUDIO_CODECRES_REG          BROW_AUDIO_BASE + 0x48              //远程浏览器Codec保留位寄存器
#define BROW_AUDIO_POWERDNCS_REG         BROW_AUDIO_BASE + 0x4C              //远程浏览器Powerdown Control/Status寄存器
#define BROW_AUDIO_EXTAUDIOID_REG        BROW_AUDIO_BASE + 0x50              //远程浏览器Extended Audio ID寄存器
#define BROW_AUDIO_EXTAUDIOCS_REG        BROW_AUDIO_BASE + 0x54              //远程浏览器Extended Audio Control/Status寄存器
#define BROW_AUDIO_PCMDACRATE_REG        BROW_AUDIO_BASE + 0x58              //远程浏览器PCM DAC Rate寄存器
#define BROW_AUDIO_PCMADCRATE_REG        BROW_AUDIO_BASE + 0x64             //远程浏览器PCM ADC Rate寄存器

#define BROW_AUDIO_VENDORRES1_REG        BROW_AUDIO_BASE + 0xB4              //远程浏览器Vendor Reserved 1寄存器
#define BROW_AUDIO_VENDORRES2_REG        BROW_AUDIO_BASE + 0xE8              //远程浏览器Vendor Reserved 2寄存器
#define BROW_AUDIO_VENDORRES3_REG        BROW_AUDIO_BASE + 0xF4              //远程浏览器Vendor Reserved 3寄存器
#define BROW_AUDIO_VENDORID1_REG         BROW_AUDIO_BASE + 0xF8              //远程浏览器Vendor ID1寄存器
#define BROW_AUDIO_VENDORID2_REG         BROW_AUDIO_BASE + 0xFC              //远程浏览器Vendor ID2寄存器

#define BROW_AUDIO_COLDRESET            0x80000000
#define BROW_AUDIO_RESET                0x40000000
#define BROW_AUDIO_RECORDEN             0x20000000
#define BROW_AUDIO_PLAYEN               0x10000000

#define BROW_AUDIO_SETVOLUME            0x10001
#define BROW_AUDIO_SETPCMVOL            0x10002
#define BROW_AUDIO_SETADC               0x10010
#define BROW_AUDIO_SETDAC               0x10011
#define BROW_AUDIO_SELRECORD_CHAN       0x10020
#define BROW_AUDIO_SETRECORD_GAIN       0x10021


#define CODEC_READ_TIMEOUT             BROW_AUDIO_BASE +  10             //音频编解码器（Codec）读寄存器等待延时
#define BROW_AUDIO_CODEC_PLAY          BROW_AUDIO_BASE +  0x08           //控制远程浏览器音频编解码器（Codec）播放控制字
#define BROW_AUDIO_CODEC_RECORD        BROW_AUDIO_BASE +  0x04           //控制远程浏览器音频编解码器（Codec）录制控制字
#define BROW_AUDIO_CODEC_START         BROW_AUDIO_BASE +  0x02            //控制远程浏览器音频编解码器启动
#define BROW_AUDIO_CODEC_STOP          BROW_AUDIO_BASE +  0x01            //控制远程浏览器音频编解码器停止

#define BROW_AUDIO_MSGID_REG           BROW_AUDIO_BASE + 0x80              //浏览器输入音频FC-AV标识寄存器
#define BROW_AUDIO_FCTX_CTRL           BROW_AUDIO_BASE + 0x84              //浏览器音频输入FC发送控制寄存器
#define BROW_AUDIO_CODEC_CTRL          BROW_AUDIO_BASE + 0x88              //浏览器音频编解码器（Codec）控制寄存器
#define BROW_AUDIO_CODEC_DATA          BROW_AUDIO_BASE + 0x8C              //浏览器音频编解码器（Codec）数据端口寄存器

//浏览器视频传感器模块寄存器地址
#define BROW_CAM_CAPPOS_REG             0x00              //浏览器视频传感器采集窗口位置寄存器
#define BROW_CAM_CAPSIZE_REG            0x04              //浏览器视频传感器采集窗口尺寸寄存器
#define BROW_CAM_DISPPOS_REG            0x08              //浏览器视频传感器显示窗口位置寄存器
#define BROW_CAM_DISPSIZE_REG           0x0C              //浏览器视频传感器显示窗口尺寸寄存器
#define BROW_CAM_DISPADDR_REG           0x10              //浏览器视频传感器显示区地址寄存器
#define BROW_CAM_DISPCTRL_REG           0x14              //浏览器视频传感器控制寄存器


//浏览器监视器0配置寄存器RW地址
#define BROW_MON0_BASE                0x0800
//浏览器监视器1配置寄存器RW地址
#define BROW_MON1_BASE                0x0900
//浏览器监视器2配置寄存器RW地址
#define BROW_MON2_BASE                0x0A00
//浏览器监视器3配置寄存器RW地址
#define BROW_MON3_BASE                0x0B00
//浏览器监视器4配置寄存器RW地址
#define BROW_MON4_BASE                0x0C00
//浏览器监视器5配置寄存器RW地址
#define BROW_MON5_BASE                0x0D00
//浏览器监视器62配置寄存器RW地址
#define BROW_MON6_BASE                0x0E00
//浏览器监视器7配置寄存器RW地址
#define BROW_MON7_BASE                0x0F00



#define BROW_MON_ID_REG                0x00         //浏览器监视器标识寄存器
#define BROW_MON_CTRL_REG              0x04         //浏览器监视器控制寄存器
#define BROW_MON_LUMA_REG              0x08         //浏览器背光控制寄存器
#define BROW_MON_CTRL3_REG              0x0C         //浏览器监视器控制寄存器2

#define BROW_MON_CURCTL_REG            0x10         //浏览器监视器光标控制寄存器
#define BROW_MON_CURPARA_REG           0x14         //浏览器监视器光标参数寄存器
#define BROW_MON_CURDATA_REG           0x18         //浏览器监视器光标文件寄存器
#define BROW_MON_CURPOS_REG            0x1C         //浏览器监视器光标显示坐标寄存器
#define BROW_MON_CURCOLOR_REG          0x20         //浏览器监视器光标颜色索引表偏移

#define BROW_MON_BUFF0_CTL            0x80         //浏览器监视器显示窗口0控制寄存器
#define BROW_MON_BUFF0_ADDR           0x84         //浏览器监视器显示窗口0所使用图层的基地址
#define BROW_MON_BUFF0_POS            0x88         //浏览器监视器显示窗口0相对于图层的坐标

#define BROW_MON_BUFF1_CTL            0x90         //浏览器监视器显示窗口1控制寄存器
#define BROW_MON_BUFF1_ADDR           0x94         //浏览器监视器显示窗口1所使用图层的基地址
#define BROW_MON_BUFF1_POS            0x98         //浏览器监视器显示窗口1相对于图层的坐标

#define BROW_MON_BUFF2_CTL            0xA0         //浏览器监视器显示窗口2控制寄存器
#define BROW_MON_BUFF2_ADDR           0xA4         //浏览器监视器显示窗口2所使用图层的基地址
#define BROW_MON_BUFF2_POS            0xA8         //浏览器监视器显示窗口2相对于图层的坐标

#define BROW_MON_BUFF3_CTL            0xB0         //浏览器监视器显示窗口2控制寄存器
#define BROW_MON_BUFF3_ADDR           0xB4         //浏览器监视器显示窗口2所使用图层的基地址
#define BROW_MON_BUFF3_POS            0xB8         //浏览器监视器显示窗口2相对于图层的坐标


//浏览器IIC接口定义
#define BROW_IIC1_TX_MSGID_REG         0x000E0000    //浏览器I2C接口1接收ASM消息ID寄存器
#define BROW_IIC1_RX_MSGID_REG         0x000E0004    //浏览器I2C接口1发送ASM消息ID寄存器
#define BROW_IIC1_CTRL_REG             0x000E0008    //浏览器I2C接口1控制寄存器

#define BROW_IIC2_TX_MSGID_REG         0x000E0010    //浏览器I2C接口2接收ASM消息ID寄存器
#define BROW_IIC2_RX_MSGID_REG         0x000E0014    //浏览器I2C接口2发送ASM消息ID寄存器
#define BROW_IIC2_CTRL_REG             0x000E0018    //浏览器I2C接口2控制寄存器

#define BROW_IIC_WAITTIME               100            //浏览器I2C接口读IIC消息ACK等待时间

typedef struct {                    //定义远程浏览IIC发送接收控制数据结构
    unsigned        browID;         //远程浏览器端口号           
    int             pno;            //远程浏览器IIC接口编号       
    long long       browNodeName;   //远程浏览器节点名
    long long       browPortName;   //远程浏览器端口名
    FC_BROWSER_TYPE browType;       //远程浏览器类型
    unsigned        txMsgID;        //远程浏览器IIC接口发送ASM消息ID
    unsigned        rxMsgID;        //远程浏览器IIC接口接收ASM消息ID    
    char            rxBuf[256];
    unsigned        usrDef;
    int             dataLen;
    sem_t           rxSem;
    sem_t           txSem;
} FCBROW_IIC_STRUCT;

//浏览器监视器触摸屏模块RW地址偏移
#define BROW_TOUCH_READ_CTRLWORD        0x10202301   //浏览器逻辑设计人员定义的读浏览器触摸屏消息控制字
#define BROW_TOUCH_BASE                0x000E0000    //浏览器监视器触摸屏模块基地址
#define BROW_TOUCH_ASMID_REG           BROW_TOUCH_BASE+0x00    //浏览器监视器触摸屏ASM标识定义寄存器
#define BROW_TOUCH_TGTID_REG           BROW_TOUCH_BASE+0x04    //浏览器监视器触摸屏目的端口寄存器
#define BROW_TOUCH_CTRL_REG           BROW_TOUCH_BASE+0x08    //浏览器监视器触摸屏控制寄存器

//浏览器监视器摄像头模块RW地址偏移
#define BROW_CAM_BASE                0x000F0000    //浏览器监视器摄像头模块基地址
#define BROW_CAM_CONFIG0_REG         BROW_CAM_BASE+0x00    //浏览器监视器摄像头接口寄存器0
#define BROW_CAM_CONFIG1_REG         BROW_CAM_BASE+0x04    //浏览器监视器摄像头接口寄存器1
#define BROW_CAM_CONFIG2_REG         BROW_CAM_BASE+0x08    //浏览器监视器摄像头接口寄存器2
#define BROW_CAM_CONFIG3_REG         BROW_CAM_BASE+0x0C    //浏览器监视器摄像头接口寄存器3
#define BROW_CAM_CONFIG4_REG         BROW_CAM_BASE+0x10    //浏览器监视器摄像头接口寄存器4
#define BROW_CAM_CONFIG5_REG         BROW_CAM_BASE+0x14    //浏览器监视器摄像头接口寄存器5
#define BROW_CAM_CONFIG6_REG         BROW_CAM_BASE+0x18    //浏览器监视器摄像头接口寄存器6
#define BROW_CAM_CONFIG7_REG         BROW_CAM_BASE+0x1C    //浏览器监视器摄像头接口寄存器7
#define BROW_CAM_CONFIG_BUFFER         BROW_CAM_BASE+0x1000    //浏览器监视器摄像头接口配置缓冲

//浏览器监视器摄像头模块控制指令
#define BROW_CAM_SETBRIGHT         0x10001    //设置浏览器摄像头亮度
#define BROW_CAM_SETCONTRAST       0x10002    //设置浏览器摄像头对比度
#define BROW_CAM_SETGAIN           0x10003    //设置浏览器摄像头增益
#define BROW_CAM_SETFLIP           0x10004    //设置浏览器摄像头翻转
#define BROW_CAM_SETMIRROR         0x10005    //设置浏览器摄像头镜像

#define BROW_CAM_ISPTEST           0x20001    //设置浏览器摄像头测试图案
#define BROW_CAM_DVPTEST           0x20002    //设置浏览器摄像头测试图案

//DDR2 SDRAM地址
#define  BROW_MEM_BASE_ADDR                0x00000000           //浏览器DDR内存基地址
#define  BROW_MEM_SIZE                     0x40000000           //浏览器DDR内存大小

typedef struct {    //触摸手指消息结构
    int touchdown;  //0=松开已处理，1=松开未处理,3=按下,4=更新过按下数据的中间状态
    int msgCnt;		//每次按下或抬起时接收到的消息计数，当按下或抬起状态改变时清零
    int touchx;
    int touchy;
    int scrx;
    int scry;
} BROW_TOUCH_FINGER;

typedef struct {    //浏览器触摸屏设备结构
    int                 browId;
    int                 browTsMsgCnt;
    BROW_TOUCH_FINGER   touchFinger[10];
    FC_BROWSER_TYPE     browType;
    void (*handler)(int browId, int fingerNr, int stat, int x, int y);
    int                 param1;
} FC_BROWTS_HANDLER;
