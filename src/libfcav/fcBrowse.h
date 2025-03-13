//��Ƶ���տ��ƼĴ���AV_RX_CTRL_REGλ����
#define AV_RX_START_BIT                         0x80000000  //�������տ���λ
#define AV_RX_STOP_BIT                          0x40000000  //ֹͣ���տ���λ
#define AV_RX_WINDATA_SET_BIT                   0x20000000  //������ʾ�����洢������λ
#define AV_RX_WINDATA_CLR_BIT                   0x10000000  //�����ʾ�����洢������λ
#define AV_RX_WINDATA_RESORT_BIT                0x08000000  //��ʼ��������˳�����λ
#define AV_RX_WINDATA_READ_BIT                  0x04000000  //����ʾ���ڲ�������λ
#define AV_RX_WINDATA_OK_BIT                    0x02000000  //������������־

#define RW_FCPORT0_OPCODE                        0xFFFF0000      //FC�˿�0�洢����ʶ
#define RW_FCPORT1_OPCODE                        0xFFFF0001      //FC�˿�1�洢����ʶ
#define RW_FCDEV_OPCODE                          0xFFFF0100      //FC�豸���������Ĵ����洢����ʶ
#define RW_FCCOM1_OPCODE                         0xFFFF0110      //����1�洢����ʶ
#define RW_FCCOM2_OPCODE                         0xFFFF0111      //����2�洢����ʶ
#define RW_FCIIC_OPCODE                          0xFFFF0120      //IIC�洢����ʶ
#define RW_FCSPI_OPCODE                          0xFFFF0130      //SPI�洢����ʶ
#define RW_FCAC97_OPCODE                         0xFFFF0140      //AC97�洢����ʶ
#define RW_FCCAN_OPCODE                          0xFFFF0150      //CAN�洢����ʶ
#define RW_FCFLEXRAY_OPCODE                      0xFFFF0160      //FlexRay�洢����ʶ
#define RW_FCMIC_OPCODE                          0xFFFF0170      //MIC�洢����ʶ
#define RW_FCETHERNET_OPCODE                     0xFFFF0180      //��̫���洢����ʶ
#define RW_FCUSB_OPCODE                          0xFFFF0190      //USB�洢����ʶ
#define RW_FCIO_OPCODE                           0xFFFF01A0      //IO�洢����ʶ
#define RW_BROWSER_VIDEOSENSE_OPCODE             0xFFFF01B0      //�������Ƶ�������洢����ʶ       
#define RW_FCAVRX0_OPCODE                        0xFFFF0400      //FC-AV����ģ��洢����ʶ
#define RW_FCAVRX1_OPCODE                        0xFFFF0401      //FC-AV����ģ��洢����ʶ
#define RW_FCAVTX_OPCODE                         0xFFFF0410      //FC-AV����ģ��洢����ʶ
#define RW_FCAVTX0_OPCODE                        0xFFFF0410
#define RW_FCDSTX_OPCODE                         0xFFFF0420      //���������ʹ洢����ʶ
#define RW_FCDSRX_OPCODE                         0xFFFF0421      //���������մ洢����ʶ
#define RW_FCCAM_OPCODE                          0xFFFF0422      //������ɼ�ģ��洢����ʶ
#define RW_FCSATA_OPCODE                         0xFFFF0423      //SATA�ӿ�ģ��洢����ʶ
#define RW_BROWSER_OPCODE                        0xFFFF0800      //���������Ƶ�ӿڣ�Զ�̴洢����ʶ
#define RW_FCRAM_OPCODE                          0xFFFF1000      //����������洢����ʶ



//����Զ���������ͼ�����ַ
#define BROW_FB0_BUFFER       0x00000000
#define BROW_FB1_BUFFER       0x04000000
#define BROW_FB2_BUFFER       0x80000000
#define BROW_FB3_BUFFER       0x84000000
#define BROW_FB_SIZE          0x10000000
#define BROW_LAYER_SIZE       0x04000000                  //ÿ��ͼ��ռ��64M ,

#define BROW_CURSOR_BUFFER    0x58000000
#define BROW_CURSOR_SIZE      0x00010000

#define DDR1_ADDR_MARCO     0x00000000
#define DDR2_ADDR_MARCO     0x80000000
#define DDR_SIZE_MARCO      0x40000000                  //ÿ��DDR2оƬ1024M ,

#define BROW_DISP_AREA      0x1E0010E0                  //7680X4320


//������˿�0�Ĵ���RW��ַ
#define BROW_PORT0_BASE                 0x0
//������˿�1�Ĵ���RW��ַ
#define BROW_PORT1_BASE                 0x0
//������˿�2�Ĵ���RW��ַ
#define BROW_PORT2_BASE                 0x0
//������˿�3�Ĵ���RW��ַ
#define BROW_PORT3_BASE                 0x0

//������˿ڽ���ģ��Ĵ���RW��ַƫ��
#define BROW_PORT_VIDEOID_REG                0x00              //������˿�FC_AV������Ƶ���ձ�ʶ�Ĵ���
#define BROW_PORT_DISP_ADDR_REG              0x04              //������˿�FC_AV����ͼ���ַ�Ĵ���
#define BROW_PORT_WIN_POS_REG                0x08              //������˿�FC_AV����ͼ�δ���λ�üĴ���
#define BROW_PORT_WIN_SIZE_REG               0x0c              //������˿�FC_AV����ͼ�δ��ڳߴ�Ĵ���
//#define BROW_PORT_SAMPLEVD_POS_REG           0x10              //������˿���Ƶ���ղɼ�����λ�üĴ���
//#define BROW_PORT_SAMPLEVD_SIZE_REG          0x14              //������˿���Ƶ���ղɼ����ڳߴ�Ĵ���
#define BROW_PORT_AVATTR_REG                 0x10              //������˿�FC_AV����AV�����Ĵ���
#define BROW_PORT_CTRL_REG                   0x14              //������˿�FC_AV���տ��ƼĴ���
#define BROW_PORT_SETCOLOR_REG               0x18              //������˿�FC_AV�������������ɫ�Ĵ���
#define BROW_PORT_VFRAMECNT_REG              0x1C              //������˿�FC_AV����AV����������
#define BROW_PORT_VFRAMELOST_REG             0x20              //������˿�FC_AV����AV������ʧ������

#define BROW_PORT_LINKERR_CNT                0x40              //������˿���·ʧ�ܼ�����
#define BROW_PORT_SYNLOST_CNT                0x44              //������˿�ͬ����ʧ������
#define BROW_PORT_SIGNALLOST_CNT             0x48              //������˿��źŶ�ʧ������
#define BROW_PORT_PRIMERR_CNT                0x4C              //������˿�ԭ����������
#define BROW_PORT_INVAWORD_CNT               0x50              //������˿���Ч�����ּ�����
#define BROW_PORT_CRCERR_CNT                 0x4C              //������˿�CRC���������


//����������ߴ�����
typedef enum
{
    FC_BROW_140,
    FC_BROW_170,
    FC_BROW_238,
    FC_BROW_236
}FC_BROWSER_TYPE;
//������������Խṹ
typedef struct{

    int isBrow;              	//Զ���������FC�˿�ID
    FC_BROWSER_TYPE   browType;     //Զ��������ͺ�
    int init_flag;
}FCBROW_INFO_STRUCT;

#define FCBROW_MAX_NUM		32		//����ʼ���������������
////////////////////////////////////////
//      �������Ƶ���մ��ڲ����ṹ
////////////////////////////////////////
#define FCBROW_VW_NUM   512
//���������ͼ��ṹ
typedef struct{

    unsigned   portID;              //Զ���������FC�˿�ID
    unsigned   frameBuffer;            //ͼ�����ַ
    unsigned   memSize;          //ͼ�������ڴ��С
    unsigned   fbSize;          //ͼ�㳤�����ԣ���ֵΪ����ͼ�����һ��           
    unsigned   fbAttr : 24;              //ͼ������,������ʾ����ʾ,�Ƿ񼤻������
    unsigned   fbLayer : 8;              //ͼ����ʾ��Σ�����Ϊ��߲�0�㣬��������1�㡢2�㡢3�㡢4�㣬5������Ϊ����ʾ��        
    unsigned   monPos;             //ͼ����ʾ�������Ͻ�����,ӦС��ͼ���С���ڲ���ʾʱ���ֶ���Ч
    unsigned   monSize;             //ͼ����ʾ���ڳ������ԣ�ӦС��ͼ���С���ڲ���ʾʱ���ֶ���Ч
    //struct list_head  fbList;      //��ʾͼ������        
}FCBROW_FB_STRUCT;
//����Զ���������ʾ���ڽṹ
typedef struct{

    unsigned   portID;                 //Զ��������˿�ID        
    unsigned   videoID;                //��ʾ���ڽ��յ���Ƶ��ʶ           
    unsigned   frameBuffer;            //ͼ�����ַ
    unsigned   dispWinPos;             //�����ͼ�����ַ��ʾ�������Ͻ�����
    unsigned   dispWinSize;          //��ʾ���ڳ���
    unsigned   srcPortID : 24;           //������Ƶ��ʶ��Դ�˿�ID
    unsigned   reserv1 : 6;
    unsigned   audioFlag : 1;            //������Ƶ
    unsigned   videoFlag : 1;            //������Ƶ        
    FCBROW_FB_STRUCT *pFb;          //��ʾ�������ڵ�ͼ��ָ��
    //struct list_head  winList;      //��ʾ�������� 
}FCBROW_VW_PARAM;

//����Զ���������Ƶ���Ϳ������ݽṹ
typedef struct{

    unsigned   videoID;                //��ʾ���ڷ��͵���Ƶ��ʶ           
    unsigned   scrSize;                //��Ƶ�źųߴ�
    unsigned   captWinPos;             //����Ƶ�������вɼ��������Ͻ�������
    unsigned   captWinSize;          //�ɼ����ڳ���
    unsigned   dstPortID : 24;           //������Ƶ��ʶ��Ŀ�Ķ˿�ID
    unsigned   reserv1 : 2;
    unsigned   audioFlag : 1;            //������Ƶ
    unsigned   videoFlag : 1;            //������Ƶ        
    unsigned   reserv2 : 3;
    unsigned   videoStart : 1;            //AV��������
    unsigned   audioID;                //����Ƶ����ʱ����Ƶ��ʶ         
    unsigned   audioPortID : 24;           //������Ƶ��ʶ��Ŀ�Ķ˿�ID
    unsigned   reserv3 : 7;
    unsigned   audioStart : 1;            //��Ƶ��������
    //struct list_head  winList;      //��ʾ��������
    unsigned   browPortID;             //Զ��������˿�ID       
}FCBROW_AVTX_PARAM;

//����Զ�������������ݽṹ
typedef struct{
    int width;                   //ͼ����1-256��������Ϊ��λ           
    int height;                   //ͼ��߶�1-256��������Ϊ��λ           
    int xhotspot;      //�ȵ�x����
    int yhotspot;      //�ȵ�y����        
    unsigned   colorIndex[16];         //�����ɫ������           
    char    *pDatabuf;              //���ͼ�λ�����ָ�� �� Ҫ��8�ֽڶ���
    int     bufLen;                 //���ͼ�λ���������

}STRUCT_FCBROW_CURSOR;

//���������ģ��Ĵ���RW��ַ
//#define BROW_KEYB_BASE                 0x0400
#define BROW_KEYB_BASE                 0x0

#define BROW_KEYB_MSGID_REG_            BROW_KEYB_BASE + 0x00              //��������̰���ASM��Ϣ��ʶ�Ĵ���
#define BROW_KEYB_DSTPORT_REG           BROW_KEYB_BASE + 0x04              //�����������ϢĿ�Ķ˿ڱ�ʶ�Ĵ���
#define BROW_KEYB_CTRL_REG              BROW_KEYB_BASE + 0x08              //���������ģ����ƼĴ���
#define BROW_KEYB_LED_REG               BROW_KEYB_BASE + 0x10              //���������ָʾ�ƼĴ���


//���������ģ��Ĵ���RW��ַ
//#define BROW_COMM1_BASE                0x0420
//#define BROW_COMM2_BASE                0x0430

#define BROW_COMM1_BASE                0x00
#define BROW_COMM2_BASE                0x00
#define BROW_COMM_MSGID_OFFSET         0x00              //���������ASM��Ϣ��ʶ�Ĵ���ƫ��
#define BROW_COMM_DSTPORT_OFFSET       0x04              //��������ڽ�����ϢĿ�Ķ˿ڱ�ʶ�Ĵ���ƫ��
#define BROW_COMM_CONFIG_OFFSET        0x08              //������������üĴ���ƫ��
#define BROW_COMM_CTRL_OFFSET          0x0C              //��������ڿ��ƼĴ���ƫ��

#define BROW_COMM_SETBAUDRATE           0x10001
#define BROW_COMM_SETPARITY             0x10002
#define BROW_COMM_SETPERIOD             0x10003
#define BROW_COMM_SETFCTYPE             0x20001
//����Զ����������ڷ��Ϳ������ݽṹ
typedef struct {
    unsigned    browID;                 //Զ��������˿ں�           
    int         pno;                    //Զ����������ں�       
    unsigned long long browNodeName;    //Զ��������ڵ���
    unsigned long long browPortName;    //Զ��������˿���
    unsigned    msgID;                  //Զ�����������1ASM��ϢID
    char        comBuf[512];
    int         bufDataHead;
    int         bufDataTail;
}FCBROW_COMM_STRUCT;

//�����FC-AV����ģ��Ĵ���RW��ַƫ�� 
//#define BROW_AVTX_BASE                0x0440
#define BROW_AVTX_VIDEOID_REG         0x00              //�����FC_AV�������AV��ʶ�Ĵ���
#define BROW_AVTX_VIDEO_SIZE_REG      0x04              //�����FC_AV�����źųߴ�ַ�Ĵ���
#define BROW_AVTX_WIN_POS_REG         0x08              //�����FC_AV�����������λ�üĴ���
#define BROW_AVTX_WIN_SIZE_REG        0x0c              //�����FC_AV����������ڳߴ�Ĵ���
#define BROW_AVTX_CTRL_REG            0x10              //�����FC_AV���Ϳ��ƼĴ���
#define BROW_AVTX_AUDIO_ID_REG        0x14              //�����FC_AV���������Ƶ��ʶ�Ĵ���
#define BROW_AVTX_AUDIO_CTRL_REG      0x18              //�����FC_AV��Ƶ���Ϳ��ƼĴ���
#define BROW_AVTX_AUDIO_LEN_REG       0x1C              //�����FC_AV��Ƶ���ͳ��ȼĴ���
#define BROW_AVTX_VIDEO_ATTR_REG      0x20

#define BROW_COMMSET_BASE               0x0
#define BROW_FB_SIZE_REG              BROW_COMMSET_BASE+0x0            //��ʾ���ߴ�Ĵ���
#define BROW_COLORKEY_REG             BROW_COMMSET_BASE+0x04            //͸��ɫ����Ĵ���
#define BROW_LINKSTAT_REG             BROW_COMMSET_BASE+0x08            //�����״̬�Ĵ���
#define BROW_ASSERT_MSGID_REG         BROW_COMMSET_BASE+0x0C            //ͨ����Ϣ��ʶ�Ĵ���
#define BROW_ASSERT_TGTID_REG         BROW_COMMSET_BASE+0x010            //ͨ����Ϣ���ն�ID�Ĵ���

//�������Ƶ����ģ��Ĵ���RW����ַ
//#define BROW_AUDIO_BASE                0x0500
#define BROW_AUDIO_BASE                0x0000
#define BROW_AUDIO_CTRL_REG              BROW_AUDIO_BASE + 0x80                 //�������Ƶ����FC���Ϳ��ƼĴ���
#define BROW_AUDIO_CODECRESET_REG        BROW_AUDIO_BASE + 0x00              //Զ���������Ƶ���������Codec����λ�Ĵ���
#define BROW_AUDIO_VOLUME_REG            BROW_AUDIO_BASE + 0x04              //Զ�����������������Ĵ���
#define BROW_AUDIO_MONOVOLUME_REG        BROW_AUDIO_BASE + 0x0C              //Զ�������MONO��������Ĵ���
#define BROW_AUDIO_BEEPVOLUME_REG        BROW_AUDIO_BASE + 0x14              //Զ����������������������Ĵ���

#define BROW_AUDIO_PHONEVOLUME_REG       BROW_AUDIO_BASE + 0x18              //Զ��������绰���������Ĵ���
#define BROW_AUDIO_MICVOLUME_REG         BROW_AUDIO_BASE + 0x1C              //Զ���������˷����������Ĵ���
#define BROW_AUDIO_LINEINVOLUME_REG      BROW_AUDIO_BASE + 0x20              //Զ���������������������Line In Volume���Ĵ���
#define BROW_AUDIO_CDINVOLUME_REG        BROW_AUDIO_BASE + 0x24              //Զ�������CD����������CD Volume���Ĵ���
#define BROW_AUDIO_PCMOUTVOLUME_REG      BROW_AUDIO_BASE + 0x30              //Զ�������PCM Out Volume�Ĵ���
#define BROW_AUDIO_RECORDSEL_REG         BROW_AUDIO_BASE + 0x34              //Զ�������¼��ѡ��Record Select���Ĵ���
#define BROW_AUDIO_RECORDGAIN_REG        BROW_AUDIO_BASE + 0x38              //Զ�������¼�����棨Record Gain���Ĵ���
#define BROW_AUDIO_GENERAL_REG           BROW_AUDIO_BASE + 0x40              //Զ�������General Purpose�Ĵ���
#define BROW_AUDIO_3DCTRL_REG            BROW_AUDIO_BASE + 0x44              //Զ�������3D Control�Ĵ���
#define BROW_AUDIO_CODECRES_REG          BROW_AUDIO_BASE + 0x48              //Զ�������Codec����λ�Ĵ���
#define BROW_AUDIO_POWERDNCS_REG         BROW_AUDIO_BASE + 0x4C              //Զ�������Powerdown Control/Status�Ĵ���
#define BROW_AUDIO_EXTAUDIOID_REG        BROW_AUDIO_BASE + 0x50              //Զ�������Extended Audio ID�Ĵ���
#define BROW_AUDIO_EXTAUDIOCS_REG        BROW_AUDIO_BASE + 0x54              //Զ�������Extended Audio Control/Status�Ĵ���
#define BROW_AUDIO_PCMDACRATE_REG        BROW_AUDIO_BASE + 0x58              //Զ�������PCM DAC Rate�Ĵ���
#define BROW_AUDIO_PCMADCRATE_REG        BROW_AUDIO_BASE + 0x64             //Զ�������PCM ADC Rate�Ĵ���

#define BROW_AUDIO_VENDORRES1_REG        BROW_AUDIO_BASE + 0xB4              //Զ�������Vendor Reserved 1�Ĵ���
#define BROW_AUDIO_VENDORRES2_REG        BROW_AUDIO_BASE + 0xE8              //Զ�������Vendor Reserved 2�Ĵ���
#define BROW_AUDIO_VENDORRES3_REG        BROW_AUDIO_BASE + 0xF4              //Զ�������Vendor Reserved 3�Ĵ���
#define BROW_AUDIO_VENDORID1_REG         BROW_AUDIO_BASE + 0xF8              //Զ�������Vendor ID1�Ĵ���
#define BROW_AUDIO_VENDORID2_REG         BROW_AUDIO_BASE + 0xFC              //Զ�������Vendor ID2�Ĵ���

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


#define CODEC_READ_TIMEOUT             BROW_AUDIO_BASE +  10             //��Ƶ���������Codec�����Ĵ����ȴ���ʱ
#define BROW_AUDIO_CODEC_PLAY          BROW_AUDIO_BASE +  0x08           //����Զ���������Ƶ���������Codec�����ſ�����
#define BROW_AUDIO_CODEC_RECORD        BROW_AUDIO_BASE +  0x04           //����Զ���������Ƶ���������Codec��¼�ƿ�����
#define BROW_AUDIO_CODEC_START         BROW_AUDIO_BASE +  0x02            //����Զ���������Ƶ�����������
#define BROW_AUDIO_CODEC_STOP          BROW_AUDIO_BASE +  0x01            //����Զ���������Ƶ�������ֹͣ

#define BROW_AUDIO_MSGID_REG           BROW_AUDIO_BASE + 0x80              //�����������ƵFC-AV��ʶ�Ĵ���
#define BROW_AUDIO_FCTX_CTRL           BROW_AUDIO_BASE + 0x84              //�������Ƶ����FC���Ϳ��ƼĴ���
#define BROW_AUDIO_CODEC_CTRL          BROW_AUDIO_BASE + 0x88              //�������Ƶ���������Codec�����ƼĴ���
#define BROW_AUDIO_CODEC_DATA          BROW_AUDIO_BASE + 0x8C              //�������Ƶ���������Codec�����ݶ˿ڼĴ���

//�������Ƶ������ģ��Ĵ�����ַ
#define BROW_CAM_CAPPOS_REG             0x00              //�������Ƶ�������ɼ�����λ�üĴ���
#define BROW_CAM_CAPSIZE_REG            0x04              //�������Ƶ�������ɼ����ڳߴ�Ĵ���
#define BROW_CAM_DISPPOS_REG            0x08              //�������Ƶ��������ʾ����λ�üĴ���
#define BROW_CAM_DISPSIZE_REG           0x0C              //�������Ƶ��������ʾ���ڳߴ�Ĵ���
#define BROW_CAM_DISPADDR_REG           0x10              //�������Ƶ��������ʾ����ַ�Ĵ���
#define BROW_CAM_DISPCTRL_REG           0x14              //�������Ƶ���������ƼĴ���


//�����������0���üĴ���RW��ַ
#define BROW_MON0_BASE                0x0800
//�����������1���üĴ���RW��ַ
#define BROW_MON1_BASE                0x0900
//�����������2���üĴ���RW��ַ
#define BROW_MON2_BASE                0x0A00
//�����������3���üĴ���RW��ַ
#define BROW_MON3_BASE                0x0B00
//�����������4���üĴ���RW��ַ
#define BROW_MON4_BASE                0x0C00
//�����������5���üĴ���RW��ַ
#define BROW_MON5_BASE                0x0D00
//�����������62���üĴ���RW��ַ
#define BROW_MON6_BASE                0x0E00
//�����������7���üĴ���RW��ַ
#define BROW_MON7_BASE                0x0F00



#define BROW_MON_ID_REG                0x00         //�������������ʶ�Ĵ���
#define BROW_MON_CTRL_REG              0x04         //��������������ƼĴ���
#define BROW_MON_LUMA_REG              0x08         //�����������ƼĴ���
#define BROW_MON_CTRL3_REG              0x0C         //��������������ƼĴ���2

#define BROW_MON_CURCTL_REG            0x10         //����������������ƼĴ���
#define BROW_MON_CURPARA_REG           0x14         //������������������Ĵ���
#define BROW_MON_CURDATA_REG           0x18         //���������������ļ��Ĵ���
#define BROW_MON_CURPOS_REG            0x1C         //����������������ʾ����Ĵ���
#define BROW_MON_CURCOLOR_REG          0x20         //����������������ɫ������ƫ��

#define BROW_MON_BUFF0_CTL            0x80         //�������������ʾ����0���ƼĴ���
#define BROW_MON_BUFF0_ADDR           0x84         //�������������ʾ����0��ʹ��ͼ��Ļ���ַ
#define BROW_MON_BUFF0_POS            0x88         //�������������ʾ����0�����ͼ�������

#define BROW_MON_BUFF1_CTL            0x90         //�������������ʾ����1���ƼĴ���
#define BROW_MON_BUFF1_ADDR           0x94         //�������������ʾ����1��ʹ��ͼ��Ļ���ַ
#define BROW_MON_BUFF1_POS            0x98         //�������������ʾ����1�����ͼ�������

#define BROW_MON_BUFF2_CTL            0xA0         //�������������ʾ����2���ƼĴ���
#define BROW_MON_BUFF2_ADDR           0xA4         //�������������ʾ����2��ʹ��ͼ��Ļ���ַ
#define BROW_MON_BUFF2_POS            0xA8         //�������������ʾ����2�����ͼ�������

#define BROW_MON_BUFF3_CTL            0xB0         //�������������ʾ����2���ƼĴ���
#define BROW_MON_BUFF3_ADDR           0xB4         //�������������ʾ����2��ʹ��ͼ��Ļ���ַ
#define BROW_MON_BUFF3_POS            0xB8         //�������������ʾ����2�����ͼ�������


//�����IIC�ӿڶ���
#define BROW_IIC1_TX_MSGID_REG         0x000E0000    //�����I2C�ӿ�1����ASM��ϢID�Ĵ���
#define BROW_IIC1_RX_MSGID_REG         0x000E0004    //�����I2C�ӿ�1����ASM��ϢID�Ĵ���
#define BROW_IIC1_CTRL_REG             0x000E0008    //�����I2C�ӿ�1���ƼĴ���

#define BROW_IIC2_TX_MSGID_REG         0x000E0010    //�����I2C�ӿ�2����ASM��ϢID�Ĵ���
#define BROW_IIC2_RX_MSGID_REG         0x000E0014    //�����I2C�ӿ�2����ASM��ϢID�Ĵ���
#define BROW_IIC2_CTRL_REG             0x000E0018    //�����I2C�ӿ�2���ƼĴ���

#define BROW_IIC_WAITTIME               100            //�����I2C�ӿڶ�IIC��ϢACK�ȴ�ʱ��

typedef struct {                    //����Զ�����IIC���ͽ��տ������ݽṹ
    unsigned        browID;         //Զ��������˿ں�           
    int             pno;            //Զ�������IIC�ӿڱ��       
    long long       browNodeName;   //Զ��������ڵ���
    long long       browPortName;   //Զ��������˿���
    FC_BROWSER_TYPE browType;       //Զ�����������
    unsigned        txMsgID;        //Զ�������IIC�ӿڷ���ASM��ϢID
    unsigned        rxMsgID;        //Զ�������IIC�ӿڽ���ASM��ϢID    
    char            rxBuf[256];
    unsigned        usrDef;
    int             dataLen;
    sem_t           rxSem;
    sem_t           txSem;
} FCBROW_IIC_STRUCT;

//�����������������ģ��RW��ַƫ��
#define BROW_TOUCH_READ_CTRLWORD        0x10202301   //������߼������Ա����Ķ��������������Ϣ������
#define BROW_TOUCH_BASE                0x000E0000    //�����������������ģ�����ַ
#define BROW_TOUCH_ASMID_REG           BROW_TOUCH_BASE+0x00    //�����������������ASM��ʶ����Ĵ���
#define BROW_TOUCH_TGTID_REG           BROW_TOUCH_BASE+0x04    //�����������������Ŀ�Ķ˿ڼĴ���
#define BROW_TOUCH_CTRL_REG           BROW_TOUCH_BASE+0x08    //��������������������ƼĴ���

//���������������ͷģ��RW��ַƫ��
#define BROW_CAM_BASE                0x000F0000    //���������������ͷģ�����ַ
#define BROW_CAM_CONFIG0_REG         BROW_CAM_BASE+0x00    //���������������ͷ�ӿڼĴ���0
#define BROW_CAM_CONFIG1_REG         BROW_CAM_BASE+0x04    //���������������ͷ�ӿڼĴ���1
#define BROW_CAM_CONFIG2_REG         BROW_CAM_BASE+0x08    //���������������ͷ�ӿڼĴ���2
#define BROW_CAM_CONFIG3_REG         BROW_CAM_BASE+0x0C    //���������������ͷ�ӿڼĴ���3
#define BROW_CAM_CONFIG4_REG         BROW_CAM_BASE+0x10    //���������������ͷ�ӿڼĴ���4
#define BROW_CAM_CONFIG5_REG         BROW_CAM_BASE+0x14    //���������������ͷ�ӿڼĴ���5
#define BROW_CAM_CONFIG6_REG         BROW_CAM_BASE+0x18    //���������������ͷ�ӿڼĴ���6
#define BROW_CAM_CONFIG7_REG         BROW_CAM_BASE+0x1C    //���������������ͷ�ӿڼĴ���7
#define BROW_CAM_CONFIG_BUFFER         BROW_CAM_BASE+0x1000    //���������������ͷ�ӿ����û���

//���������������ͷģ�����ָ��
#define BROW_CAM_SETBRIGHT         0x10001    //�������������ͷ����
#define BROW_CAM_SETCONTRAST       0x10002    //�������������ͷ�Աȶ�
#define BROW_CAM_SETGAIN           0x10003    //�������������ͷ����
#define BROW_CAM_SETFLIP           0x10004    //�������������ͷ��ת
#define BROW_CAM_SETMIRROR         0x10005    //�������������ͷ����

#define BROW_CAM_ISPTEST           0x20001    //�������������ͷ����ͼ��
#define BROW_CAM_DVPTEST           0x20002    //�������������ͷ����ͼ��

//DDR2 SDRAM��ַ
#define  BROW_MEM_BASE_ADDR                0x00000000           //�����DDR�ڴ����ַ
#define  BROW_MEM_SIZE                     0x40000000           //�����DDR�ڴ��С

typedef struct {    //������ָ��Ϣ�ṹ
    int touchdown;  //0=�ɿ��Ѵ���1=�ɿ�δ����,3=����,4=���¹��������ݵ��м�״̬
    int msgCnt;		//ÿ�ΰ��»�̧��ʱ���յ�����Ϣ�����������»�̧��״̬�ı�ʱ����
    int touchx;
    int touchy;
    int scrx;
    int scry;
} BROW_TOUCH_FINGER;

typedef struct {    //������������豸�ṹ
    int                 browId;
    int                 browTsMsgCnt;
    BROW_TOUCH_FINGER   touchFinger[10];
    FC_BROWSER_TYPE     browType;
    void (*handler)(int browId, int fingerNr, int stat, int x, int y);
    int                 param1;
} FC_BROWTS_HANDLER;
