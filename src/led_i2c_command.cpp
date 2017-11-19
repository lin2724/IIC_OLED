//
// Created by lin27 on 2016/12/25.
//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <string>

#include "log.h"

using namespace std;

#define SLAVE_ADDR  0x3f



#define         CMD_clear         0x01             // 清除屏幕
#define         CMD_back          0x02             // DDRAM回零位
#define         CMD_dec1          0x04             // 读入后AC（指针）减1，向左写
#define         CMD_add1          0x06             // 读入后AC（指针）加1，向右写
#define         CMD_dis_gb1     0x0f             // 开显示_开光标_开光标闪烁
#define         CMD_dis_gb2     0x0e             // 开显示_开光标_关光标闪烁
#define         CMD_dis_gb3     0x0c             // 开显示_关光标_关光标闪烁
#define         CMD_OFF_dis     0x08             // 关显示_关光标_关光标闪烁
#define         CMD_set82         0x38             // 8位总线_2行显示
#define         CMD_set81         0x30             // 8位总线_1行显示（上边行）
#define         CMD_set42         0x28             // 4位总线_2行显示
#define         CMD_set41         0x20             // 4位总线_1行显示（上边行）
#define         lin_1               0x80             // 4位总线_1行显示（上边行）
#define         lin_2               0xc0             // 4位总线_1行显示（上边行）


enum{LCD_LINE_1,LCD_LINE_2,LCD_LINE_BUTT};
#define LCD_1602_TOTAL_COLUMN 16
#define LCD_DISPLAY_STR_MAX (LCD_1602_TOTAL_COLUMN*2)
#define LCD_LINE_DISPLAY_BUF_LEN (LCD_1602_TOTAL_COLUMN*3)
#define LCD_DISPLAY_LIST_MAX 20
#define  ULONG_32 int
#define IN
#define OUT
#define INOUT


void* thread_DisplayBufFresh(void* arg);
void DisplayVirBufWrite(int line, signed int offset, char* buf);


typedef struct TagLcdDisplayInfo
{
    ULONG_32 ulId;
    ULONG_32 ulLine;
    char szBuf[LCD_DISPLAY_STR_MAX];
}StuLcdDisplayInfo;

typedef struct TagLcdDisplayList
{
    ULONG_32 ulListCount;
    ULONG_32 ulCurDisplay;
    ULONG_32 ulListNumMax;
    pthread_mutex_t mutex;
    StuLcdDisplayInfo* pstListHead[LCD_DISPLAY_LIST_MAX];
}StuLcdDisplayList;

StuLcdDisplayList gstLcdDisplayMng;
int G_I2cFd;


typedef struct TagLcdDisplayBufMng
{
    pthread_mutex_t mutex;
    char pcNeedReFlag[LCD_LINE_BUTT];
    char* pszLcdDisplayBuf[LCD_LINE_BUTT];
}StuLcdDisplayBufMng;

StuLcdDisplayBufMng G_stDisplayBufMng;

void GetAllPin(void);
void SetAllPin(void);
int i2c_init(char* dev_file)
{

}


void JustTest(void)
{
    int i2c_fd;
    int slave_addr = SLAVE_ADDR;
    int res, ret;
    char buf[2];
    const char* device_name[2] = {"/dev/i2c-0", "/dev/i2c-1"};

    for(int count=0; count<2; count++ )
    {
        i2c_fd = open(device_name[count], O_RDWR);
        LOG_1("open device %s\n", device_name[count]);
        if(i2c_fd < 0)
        {
            LOG_1("ERROR: fail to open i2c device %s\n", device_name[count]);
            continue;
        }
        for(int addr_poll=0; addr_poll<=0x80; addr_poll++)
        {
            if(ioctl(i2c_fd, I2C_SLAVE, addr_poll) < 0)
            {
                LOG_1("ERROR: fail set slave addr 0x%x\n", addr_poll);
                usleep(200*1000);
                continue;
            }
            ret = read(i2c_fd, buf, 2);
            if(ret < 1)
            {
                if((addr_poll%10)==0)
                {
                    LOG_1("read addr fail ret: 0x%x\n" , addr_poll);
                }else
                {
                    LOG_0(".");
                }

            }else
            {
                LOG_3("read succeed with device %s ,and addr 0x%x ,total read %d!\n", device_name[count], addr_poll, ret);
            }
            usleep(10*1000);
        }
        close(i2c_fd);
    }


    return;
#if 0
    res = i2c_smbus_read_word_data(i2c_fd, reg);
    if (res < 0) {
        /* ERROR HANDLING: i2c transaction failed */
        printf("transaction fail\n");
    } else {
        /* res contains the read word */
        printf("transaction ok\n");
    }
#endif

}

#define BIT_1602_RS (1<<0)
#define BIT_1602_RW (1<<1)
#define BIT_1602_CS (1<<2)
#define BIT_1602_LIGHT (1<<3)

#define BIT_1602_DB4 (1<<4)
#define BIT_1602_DB5 (1<<5)
#define BIT_1602_DB6 (1<<6)
#define BIT_1602_DB7 (1<<7)

unsigned char gcWord_W;
unsigned char gcWord_R;

#define set_1602_RS(bit) if(bit){gcWord_W|=BIT_1602_RS;}else{gcWord_W&=~BIT_1602_RS;}
#define set_1602_RW(bit) if(bit){gcWord_W|=BIT_1602_RW;}else{gcWord_W&=~BIT_1602_RW;}
#define set_1602_CS(bit) if(bit){gcWord_W|=BIT_1602_CS;}else{gcWord_W&=~BIT_1602_CS;}
#define set_1602_Light(bit) if(bit){gcWord_W|=BIT_1602_LIGHT;}else{gcWord_W&=~BIT_1602_LIGHT;}

void setHalfWordHigh(unsigned char dat)
{
    gcWord_W &= 0x0f;
    gcWord_W |= (dat/16)<<4;
}
void setHalfWordLow(unsigned char dat)
{
    gcWord_W &= 0x0f;
    gcWord_W |= (dat%16)<<4;
    //printf("set data 0x%x\n", gcWord_W>>4);
}
void wait_1602_busy(void)
{
    //printf("wait busy\n");
    usleep(500);
    return;
    while(1)
    {
        setHalfWordLow(0xff);
        set_1602_RS(0);
        set_1602_RW(1);
        set_1602_CS(0);
        SetAllPin();
        GetAllPin();
        if(!(gcWord_R & BIT_1602_DB7))
        {
            set_1602_CS(0);
            SetAllPin();
            break;
        }
        usleep(50*1000);
        LOG_0("busy..\n");
    }
    LOG_0("wait done\n");
}



void LCD1602_EnableOnce(unsigned char usec)
{
    set_1602_CS(1);
    SetAllPin();
    usleep(usec);
    set_1602_CS(0);
    SetAllPin();
}

void LCD_write_L4bit_command(unsigned char data)
{
    wait_1602_busy();
    set_1602_RS(0);
    set_1602_RW(0);
    setHalfWordLow(data);
    //SetAllPin();
    LCD1602_EnableOnce(10);
}
void LCD1602_WriteData(unsigned char data)
{
    wait_1602_busy();
    set_1602_RS(1);
    set_1602_RW(0);
    setHalfWordHigh(data);
    LCD1602_EnableOnce(10);

    setHalfWordLow(data);
    LCD1602_EnableOnce(10);
}
void LCD1602_WriteCMD(unsigned char cmd)
{
    //printf("write cmd 0x%x\n", cmd);
    wait_1602_busy();
    set_1602_RS(0);
    set_1602_RW(0);
    setHalfWordHigh(cmd);
    //SetAllPin();
    LCD1602_EnableOnce(10);

    setHalfWordLow(cmd);
    //SetAllPin();
    LCD1602_EnableOnce(10);
    usleep(10*1000);
}

int Oled_I2c_Init(void)
{
    char buf[2];
    const char* device_name[2] = {"/dev/i2c-0", "/dev/i2c-1"};
    int i2c_fd = open(device_name[0], O_RDWR);
    LOG_1("open device %s\n", device_name[0]);
    if(i2c_fd < 0)
    {
        LOG_1("ERROR: fail to open i2c device %s\n", device_name[0]);
        return -1;
    }
    if(ioctl(i2c_fd, I2C_SLAVE, SLAVE_ADDR) < 0)
    {
        LOG_1("ERROR: fail set slave addr 0x%x\n", SLAVE_ADDR);
        return -1;
    }
    buf[0] = 0;
    int ret = write(i2c_fd, buf, 1);
    ret = read(i2c_fd, buf, 2);
    if(ret < 1)
    {
        LOG_1("read test addr fail ret: 0x%x\n" , SLAVE_ADDR);
    }else
    {
        LOG_3("read succeed with device %s ,and addr %d ,total read %d!\n", device_name[0], SLAVE_ADDR, ret);
    }
    return i2c_fd;
}


void I2C_WriteByte(unsigned char addr,unsigned char data)
{
    int ret = 0;
    unsigned char buf[2];
    buf[0] = addr;
    buf[1] = data;
    ret = write(G_I2cFd, (void*)&buf[0], 2);
    if(ret != 2){LOG_1("write byte fail %d\n", ret);}
    usleep(100*1000);
    return;
    ret = write(G_I2cFd, (void*)&buf[1], 1);
    if(ret != 1){LOG_1("write byte fail %d\n", ret);}
    usleep(1*1000);
}


unsigned char Xword[]={
        0x18,0x18,0x07,0x08,0x08,0x08,0x07,0x00,  //℃，代码 0x00
        0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,  //一，代码 0x01
        0x00,0x00,0x00,0x0e,0x00,0xff,0x00,0x00,  //二，代码 0x02
        0x00,0x00,0xff,0x00,0x0e,0x00,0xff,0x00,  //三，代码 0x03
        0x00,0x00,0xff,0xf5,0xfb,0xf1,0xff,0x00,  //四，代码 0x04
        0x00,0xfe,0x08,0xfe,0x0a,0x0a,0xff,0x00,  //五，代码 0x05
        0x00,0x04,0x00,0xff,0x00,0x0a,0x11,0x00,  //六，代码 0x06
        0x00,0x1f,0x11,0x1f,0x11,0x11,0x1f,0x00,  //日，代码 0x07
};
void CgramWrite(void) { // 装入CGRAM //
    unsigned char i;
    LCD1602_WriteCMD(0x06);   // CGRAM地址自动加1
    LCD1602_WriteCMD(0x40);   // CGRAM地址设为00处
    for(i=0;i<64;i++) {
        LCD1602_WriteData(Xword[i]);// 按数组写入数据
    }
}

void print(unsigned char a,char *str){
    LCD1602_WriteCMD(a | 0x80);
    while(*str != '\0'){
        LCD1602_WriteData(*str++);
    }
    //*str = 0;
}
void print2(unsigned char a,unsigned char t){
    LCD1602_WriteCMD(a | 0x80);
    LCD1602_WriteData(t);
}
void LCD1602_LightCmd(int cmd)
{
    if(cmd){LOG_0("light on\n");}
    else{LOG_0("light off\n");}
    set_1602_Light(cmd);
    SetAllPin();
}
void OLED_Init(void)
{
    LCD1602_LightCmd(1);
    usleep(100*1000); //这里的延时很重要


    LCD_write_L4bit_command(0x03);
    usleep(100);
    LCD_write_L4bit_command(0x03);
    usleep(100);
    LCD_write_L4bit_command(0x03);
    usleep(100);
    LCD_write_L4bit_command(0x02);
    usleep(100);
    LCD1602_WriteCMD(0x28);
    usleep(100);
    LCD1602_WriteCMD(CMD_dis_gb1);
    usleep(100);
    LCD1602_WriteCMD(0x06);
    usleep(100);
    LCD1602_WriteCMD(0x01);
    usleep(100);



    LCD1602_WriteCMD(CMD_set42); //* 显示模式设置：4位数据显示2行，
    LCD1602_WriteCMD(CMD_set42); //* 必须再运行一次！不然就出错
    LCD1602_WriteCMD(CMD_set42); //* 必须再运行一次！不然就出错
    LCD1602_WriteCMD(CMD_set42); //* 必须再运行一次！不然就出错
    LCD1602_WriteCMD(CMD_clear); //  显示清屏
    LCD1602_WriteCMD(CMD_back);  //* 数据指针指向第1行第1个字符位置
    LCD1602_WriteCMD(CMD_add1);  //  显示光标移动设置：文字不动，光标右移
    LCD1602_WriteCMD(CMD_dis_gb1);  //  显示开及光标设置：显示开，光标开，闪烁开
    CgramWrite();
    LOG_0("led init done\n");
}



void OLED_DrawBMP(void)
{

}
void ShowPin(char data)
{
    int i, j;
    LOG_0("#### ####\n");
    for(i=7; i>=0; i--)
    {
        if(data & (1<<i)){LOG_0("1");}
        else{LOG_0("0");}
        if(i==4){LOG_0(" ");}
    }
    LOG_0("\n");
}
void GetAllPin(void)
{
    //unsigned char buf[2];
    int ret;
    ret = read(G_I2cFd, &gcWord_R, 1);
    if(ret != 1){LOG_1("read fail %d\n", ret);}
    //printf("\nread\n");
    //ShowPin(buf[0]);
    //gcWord_R = buf[0];
}
void SetAllPin(void)
{
    unsigned char buf[2];
    //buf[0] = gcWord_W;
    int ret = write(G_I2cFd, &gcWord_W, 1);
    if(ret != 1){LOG_1("write fail %d\n", ret);}
    //printf("\nset\n");
    //ShowPin(gcWord_W);

}

void LcdSetPos(int line, int offset)
{
    if(offset >LCD_1602_TOTAL_COLUMN)
    {
        LOG_2("warning:offset [%d] out of maximum [%d]", offset, LCD_1602_TOTAL_COLUMN);
        return;
    }
    switch(line)
    {
        case LCD_LINE_1:LCD1602_WriteCMD((offset+0x80) | 0x80);break;
        case LCD_LINE_2:LCD1602_WriteCMD((offset+0x40) | 0x80);break;
        default:break;
    }
}

void ShowStringAtPos(int line, int offset, char* str)
{
    int count = 0;
    LcdSetPos(line, offset);
    while(1)
    {
        for(count = offset; count<LCD_1602_TOTAL_COLUMN; count++)
        {
            if(*str == '\0'){break;}
            LCD1602_WriteData(*str);
            str++;
        }
        if(*str != '\0')
        {
            switch(line)
            {
                case LCD_LINE_1:LcdSetPos(LCD_LINE_2, 0);offset = 0;break;
                case LCD_LINE_2:LcdSetPos(LCD_LINE_1, 0);offset = 0;break;
                default:break;
            }
        }else
        {
            break;
        }
    }
}

void ShowLineStringAtPos(int line, int offset, char* str)
{
    int count = 0;
    LcdSetPos(line, offset);

    for(count = offset; count<LCD_1602_TOTAL_COLUMN; count++)
    {
        if(*str == '\0'){break;}
        LCD1602_WriteData(*str);
        str++;
    }

}
void ClearLine(int line)
{
    char szBuf[LCD_1602_TOTAL_COLUMN];
    memset(szBuf, ' ', LCD_1602_TOTAL_COLUMN);
    ShowStringAtPos(line, 0, szBuf);
}




int DisplayListInit(void)
{
    int ret = 0, i=0;
    ret = pthread_mutex_init(&gstLcdDisplayMng.mutex,NULL);
    if(ret)
    {
        LOG_1("ERROR:mutex init fail %d\n", ret);
        return ret;
    }
    ret = pthread_mutex_init(&G_stDisplayBufMng.mutex,NULL);
    if(ret)
    {
        LOG_1("ERROR:mutex init fail %d\n", ret);
        return ret;
    }
    memset((void*)&gstLcdDisplayMng, 0, sizeof(StuLcdDisplayList));
    gstLcdDisplayMng.ulListNumMax = LCD_DISPLAY_LIST_MAX;
    memset(&G_stDisplayBufMng,0,sizeof(StuLcdDisplayBufMng));
    for(i=0; i< LCD_LINE_BUTT; i++)
    {
        G_stDisplayBufMng.pszLcdDisplayBuf[i] = (char*)malloc(LCD_LINE_DISPLAY_BUF_LEN);
        if(NULL == G_stDisplayBufMng.pszLcdDisplayBuf[i])
        {
            LOG_0("malloc fail\n");
            _exit(1);
        }
        memset(G_stDisplayBufMng.pszLcdDisplayBuf[i],0,LCD_LINE_DISPLAY_BUF_LEN);
    }
}

int DisplayAddList(INOUT StuLcdDisplayInfo* pstNewDisplayInfo)
{
    int i = 0;
    ULONG_32 id = 0;
    pthread_mutex_lock(&gstLcdDisplayMng.mutex);
    StuLcdDisplayInfo* pstTmpDisplayInfo = (StuLcdDisplayInfo*)malloc(sizeof(StuLcdDisplayInfo));
    if(NULL == pstTmpDisplayInfo)
    {
        LOG_0("ERROR: malloc fail\n");
        pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
        return -1;
    }
    if(gstLcdDisplayMng.ulListCount >= gstLcdDisplayMng.ulListNumMax)
    {
        LOG_1("ERROR: display list full [%d]\n", gstLcdDisplayMng.ulListCount);
        pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
        free(pstTmpDisplayInfo);
        return -1;
    }
    for(i=0; i<gstLcdDisplayMng.ulListCount; i++)
    {
        if(gstLcdDisplayMng.pstListHead[i]->ulId >= id)
        {
            id = gstLcdDisplayMng.pstListHead[i]->ulId + 1;
        }
    }
    pstNewDisplayInfo->ulId = id;
    memcpy((void*)pstTmpDisplayInfo, (void*)pstNewDisplayInfo, sizeof(StuLcdDisplayInfo));
    gstLcdDisplayMng.pstListHead[gstLcdDisplayMng.ulListCount] = pstTmpDisplayInfo;
    gstLcdDisplayMng.ulListCount ++;
    pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
    return id;
}
int DisplayDellListById(ULONG_32 id)
{
    int i = 0, j = 0;
    int FindFlag = 0;
    pthread_mutex_lock(&gstLcdDisplayMng.mutex);
    for(i=0; i< gstLcdDisplayMng.ulListCount; i++)
    {
        if(gstLcdDisplayMng.pstListHead[i]->ulId == id)
        {
            FindFlag = 1;
            free(gstLcdDisplayMng.pstListHead[i]);
            for(j=i+1; j< gstLcdDisplayMng.ulListCount; j++)
            {
                gstLcdDisplayMng.pstListHead[j-1] = gstLcdDisplayMng.pstListHead[j];
            }
            gstLcdDisplayMng.ulListCount--;
            break;
        }
    }
    pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
    if(1 != FindFlag)
    {
        LOG_1("ERROR: display info id [%d] not found in list, delete fail\n", id);
        return -1;
    }
    return 0;
}


StuLcdDisplayInfo* getNextDisplay(int line)
{
    static int line_rec_1 = -1;
    static int line_rec_2 = -1;
    int i = 0;
    int* pline_rec_tmp = NULL;
    int succeed_flag = 0;
    StuLcdDisplayInfo* pstTmpLcdDisplayInfo = NULL;
    pstTmpLcdDisplayInfo = (StuLcdDisplayInfo* )malloc(sizeof(StuLcdDisplayInfo));
    if(NULL == pstTmpLcdDisplayInfo)
    {
        LOG_0("malloc fail");
        return NULL;
    }
    switch(line)
    {
        case LCD_LINE_1:pline_rec_tmp = &line_rec_1;break;
        case LCD_LINE_2:pline_rec_tmp = &line_rec_2;break;
        default:LOG_0("ERROR:wrong line specific!");return NULL;
    }

    
    pthread_mutex_lock(&gstLcdDisplayMng.mutex);
    do{
        for(i=0; i<gstLcdDisplayMng.ulListCount; i++)
        {
            if(i>*pline_rec_tmp && gstLcdDisplayMng.pstListHead[i]->ulLine == line)
            {
                memcpy(pstTmpLcdDisplayInfo, gstLcdDisplayMng.pstListHead[i],
                       sizeof(StuLcdDisplayInfo));
                *pline_rec_tmp = i;
                succeed_flag = 1;
                break;
            }
        }
        if(0 == succeed_flag)
        {
            if(-1 == *pline_rec_tmp)
            {
                pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
                free(pstTmpLcdDisplayInfo);
                return NULL;
            }else
            {
                *pline_rec_tmp = -1;
                continue;
            }
        }
    }while(0);
    pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
    //printf("get next line [%d] [%d]\n", line, pstTmpLcdDisplayInfo->ulId);
    return pstTmpLcdDisplayInfo;
}


typedef struct DisplayMngRec
{
    StuLcdDisplayInfo* pstLcdDisplayInfo;
    int offset;
    int timeUse;
    int roundUse;
    int done;
    int changeDelay;
    int moveDir;
}stDisplayMngRec;

#define DISPLAY_TIME_MAX 5
#define DISPLAY_ROUND_MAX 4
#define DISPLAY_CHANGE_DLY 2
void* thread_DisplayHandle(void* arg)
{
    int i = 0, j = 0;
    int MissionFlag = 0;
    char szBuf[LCD_DISPLAY_STR_MAX];
    int display_len = 0;
    StuLcdDisplayInfo stTmpLcdDisplayInfo;
    stDisplayMngRec pDisplayMngRec[LCD_LINE_BUTT] = {0};
    while(1)
    {
        for(i=0; i<LCD_LINE_BUTT; i++)
        {
            if(NULL == pDisplayMngRec[i].pstLcdDisplayInfo || pDisplayMngRec[i].done)
            {
                free(pDisplayMngRec[i].pstLcdDisplayInfo);
                pDisplayMngRec[i].pstLcdDisplayInfo = getNextDisplay(i);
                pDisplayMngRec[i].offset = 0;
                pDisplayMngRec[i].done = 0;
                pDisplayMngRec[i].roundUse = 0;
                pDisplayMngRec[i].timeUse = 0;
                pDisplayMngRec[i].changeDelay = 0;
                pDisplayMngRec[i].moveDir = -1;
            }else
            {
                display_len = strlen(pDisplayMngRec[i].pstLcdDisplayInfo->szBuf);
                if(display_len > LCD_1602_TOTAL_COLUMN)
                {
                    if((pDisplayMngRec[i].offset + display_len) == LCD_1602_TOTAL_COLUMN)
                    {
                        pDisplayMngRec[i].moveDir = 1;
                        if(pDisplayMngRec[i].changeDelay > DISPLAY_CHANGE_DLY)
                        {
                            pDisplayMngRec[i].moveDir = 1;
                            pDisplayMngRec[i].changeDelay = 0;
                        }else
                        {
                            pDisplayMngRec[i].moveDir = 0;
                            pDisplayMngRec[i].changeDelay ++;
                        } 
                    }
   
                    pDisplayMngRec[i].offset += pDisplayMngRec[i].moveDir;
                    if (pDisplayMngRec[i].offset == 0)
                    {
                        pDisplayMngRec[i].moveDir = -1;
                        if(pDisplayMngRec[i].roundUse >= DISPLAY_ROUND_MAX)
                        {
                            pDisplayMngRec[i].done = 1;
                        }else
                        {
                            pDisplayMngRec[i].roundUse += 1;
                        }
                        if(pDisplayMngRec[i].changeDelay > DISPLAY_CHANGE_DLY)
                        {
                            pDisplayMngRec[i].moveDir = -1;
                            pDisplayMngRec[i].changeDelay = 0;
                        }else
                        {
                            pDisplayMngRec[i].moveDir = 0;
                            pDisplayMngRec[i].changeDelay ++;
                        } 
                    }
                    //printf("offset [%d]\n", pDisplayMngRec[i].offset);

                }else
                {
                    pDisplayMngRec[i].timeUse += 1;
                    if(pDisplayMngRec[i].timeUse >= DISPLAY_TIME_MAX)
                    {
                        pDisplayMngRec[i].done = 1;
                    }
                }
            }
        }

        for(i=0; i<LCD_LINE_BUTT; i++)
        {
            if(NULL == pDisplayMngRec[i].pstLcdDisplayInfo){continue;}
            DisplayVirBufWrite(i, pDisplayMngRec[i].offset, pDisplayMngRec[i].pstLcdDisplayInfo->szBuf);
        }
       usleep(500*1000);
    }

    while(1)
    {
        pthread_mutex_lock(&gstLcdDisplayMng.mutex);
        if(gstLcdDisplayMng.ulCurDisplay < gstLcdDisplayMng.ulListCount)
        {
            memcpy(&stTmpLcdDisplayInfo, gstLcdDisplayMng.pstListHead[gstLcdDisplayMng.ulCurDisplay],
                   sizeof(StuLcdDisplayInfo));
            MissionFlag = 1;
            gstLcdDisplayMng.ulCurDisplay++;
            if(gstLcdDisplayMng.ulCurDisplay >= gstLcdDisplayMng.ulListCount)
            {
                gstLcdDisplayMng.ulCurDisplay = 0;
            }
        }else
        {
            gstLcdDisplayMng.ulCurDisplay = 0;
        }
        pthread_mutex_unlock(&gstLcdDisplayMng.mutex);
        if(MissionFlag)
        {
            MissionFlag = 0;
            ClearLine(stTmpLcdDisplayInfo.ulLine);
            ShowStringAtPos(stTmpLcdDisplayInfo.ulLine, 0, stTmpLcdDisplayInfo.szBuf);
        }else
        {
            LOG_0("no display info in list\n");
        }
        sleep(2);
    }
}




void* thread_DisplayBufFresh(void* arg)
{
    int i = 0;
    StuLcdDisplayInfo stTmpLcdDisplayInfo;
    char* TmpStr = NULL;
    char szBufZero[1] = {0};
    for(i=30; i< LCD_LINE_BUTT; i++)
    {
        G_stDisplayBufMng.pszLcdDisplayBuf[i] = (char*)malloc(LCD_LINE_DISPLAY_BUF_LEN);
        if(NULL == G_stDisplayBufMng.pszLcdDisplayBuf[i])
        {
            LOG_0("malloc fail\n");
            _exit(1);
        }
        memset(G_stDisplayBufMng.pszLcdDisplayBuf[i],0,LCD_LINE_DISPLAY_BUF_LEN);
    }
    //memset(&G_stDisplayBufMng,0,sizeof(StuLcdDisplayBufMng));
    while(1)
    {
        pthread_mutex_lock(&G_stDisplayBufMng.mutex);
        if(G_stDisplayBufMng.pcNeedReFlag[LCD_LINE_1])
        {
            G_stDisplayBufMng.pcNeedReFlag[0] = 0;

            TmpStr = &G_stDisplayBufMng.pszLcdDisplayBuf[LCD_LINE_1][LCD_1602_TOTAL_COLUMN];
            //ClearLine(LCD_LINE_1);
            LcdSetPos(LCD_LINE_1, 0);
            for(i=0; i<LCD_1602_TOTAL_COLUMN; i++)
            {
                if(*TmpStr)
                {
                    LCD1602_WriteData(*TmpStr);
                    TmpStr++;
                }else
                {
                    LCD1602_WriteData(' ');
                } 
            }
        }
        if(G_stDisplayBufMng.pcNeedReFlag[LCD_LINE_2])
        {
            G_stDisplayBufMng.pcNeedReFlag[LCD_LINE_2] = 0;

            TmpStr = &G_stDisplayBufMng.pszLcdDisplayBuf[LCD_LINE_2][LCD_1602_TOTAL_COLUMN];
            //ClearLine(LCD_LINE_2);
            LcdSetPos(LCD_LINE_2, 0);
            for(i=0; i<LCD_1602_TOTAL_COLUMN; i++)
            {
                if(*TmpStr)
                {
                    LCD1602_WriteData(*TmpStr);
                    TmpStr++;
                }else
                {
                    LCD1602_WriteData(' ');
                }
                
            }
        }
        pthread_mutex_unlock(&G_stDisplayBufMng.mutex);
        usleep(20*1000);
    }
}


void DisplayVirBufWrite(int line, signed int offset, char* buf)
{
    int absOffset = 0;
    int actualoffset=LCD_1602_TOTAL_COLUMN + offset;
    if(NULL == buf)
    {
        LOG_0("ERROR:NULL ptr in DisplayStrVirtual\n");
        return;
    }
    if(line>=LCD_LINE_BUTT || line<0)
    {
       LOG_1("ERROR:Wrong line %d\n", line);
       return; 
    }
    absOffset = offset>0?offset:-offset;
    if(absOffset>LCD_1602_TOTAL_COLUMN)
    {
        LOG_0("ERROR:out of display window\n");
        return;
    }
    pthread_mutex_lock(&G_stDisplayBufMng.mutex);
    for(actualoffset=LCD_1602_TOTAL_COLUMN + offset; actualoffset<LCD_LINE_DISPLAY_BUF_LEN; actualoffset++)
    {
        if(*buf)
        {
            G_stDisplayBufMng.pszLcdDisplayBuf[line][actualoffset] = *buf;
            buf++;
        }else
        {
            G_stDisplayBufMng.pszLcdDisplayBuf[line][actualoffset] = '\0';
            break;
        }
    }
    G_stDisplayBufMng.pcNeedReFlag[line] = 1;
    pthread_mutex_unlock(&G_stDisplayBufMng.mutex);
}

void DisplayThreadStart(void)
{
    pthread_t tid;
    int err = 0;
    DisplayListInit();
    err = pthread_create(&tid, NULL, thread_DisplayBufFresh, NULL);
    if(err)
    {
        LOG_1("ERROR: create thread fail [%d]\n", err);
    }
    err = pthread_create(&tid, NULL, thread_DisplayHandle, NULL);
    if(err)
    {
        LOG_1("ERROR: create thread fail [%d]\n", err);
    }
    return;
}

void LedPwr(char cmd)
{
    int ret=0;
    unsigned char buf[2];
    read(G_I2cFd, buf, 1);
    if(cmd)
    {
        buf[0] |= 1<<3;
        ret = write(G_I2cFd, (void*)&buf[0], 1);
        LOG_0("on\n");
    }else
    {
        buf[0] &= ~(1<<3);
        ret = write(G_I2cFd, (void*)&buf[0], 1);
        LOG_0("off\n");
    }
}


int check_char_is_valid_display(char c)
{
	if(c >= ' ' && c <= '~')
	{
		return 1;
	}
	return 0;
}

void* thread_LoadDisplayFile(void* arg)
{
	char line_1_file_name[] = "lcd_line_1";
	char line_2_file_name[] = "lcd_line_2";
	char line_1_buf[LCD_DISPLAY_STR_MAX] = {0};
	char line_2_buf[LCD_DISPLAY_STR_MAX] = {0};
	StuLcdDisplayInfo TmpDisplayInfo;
	int buf_pos = 0;
	int fd_list[512] = {0};
	int fd_list_pos = 0;
	char tmpC = 0;
	int ret = 0;
	int i;

	int set_load_period = 1;
	
	int fd_1=-1, fd_2 = -1;
	fd_1 = open(line_1_file_name, O_RDONLY|O_CREAT);
	fd_2 = open(line_2_file_name, O_RDONLY|O_CREAT);

	if(0>fd_1 || 0>fd_2)
	{
		LOG_0("Failed to open\n");
		return NULL;
	}

	for(;;)
	{

		for(i=0; i< fd_list_pos; i++)
		{
			DisplayDellListById(fd_list[i]);
		}
		fd_list_pos = 0;
		
		lseek(fd_1, 0, SEEK_SET);
		for(i=0, buf_pos = 0; i< LCD_DISPLAY_STR_MAX; i++)
		{
			ret = read(fd_1, (void*)&tmpC, 1);
			if(1 == ret && check_char_is_valid_display(tmpC))
			{	
				line_1_buf[buf_pos] = tmpC;
				buf_pos += 1;
			}
			else
			{
				break;
			}
		}
		if(0 != buf_pos)
		{
			line_1_buf[buf_pos] = '\0';
	        TmpDisplayInfo.ulLine = LCD_LINE_1;
	        sprintf(TmpDisplayInfo.szBuf, "%s", line_1_buf);
	        fd_list[fd_list_pos] = DisplayAddList(&TmpDisplayInfo);
	        fd_list_pos++;
	        
		}

		lseek(fd_2, 0, SEEK_SET);
		for(i=0, buf_pos = 0; i< LCD_DISPLAY_STR_MAX; i++)
		{
			ret = read(fd_2, (void*)&tmpC, 1);
			if(1 == ret && check_char_is_valid_display(tmpC))
			{	
				line_2_buf[buf_pos] = tmpC;
				buf_pos += 1;
			}
			else
			{
				break;
			}
		}
		if(0 != buf_pos)
		{
			line_2_buf[buf_pos] = '\0';
	        TmpDisplayInfo.ulLine = LCD_LINE_2;
	        sprintf(TmpDisplayInfo.szBuf, "%s", line_2_buf);
	        fd_list[fd_list_pos] = DisplayAddList(&TmpDisplayInfo);
	        fd_list_pos++;
		}
		sleep(set_load_period);

	}
	

	
}


void MainThreadStart(void)
{
    pthread_t tid;
    int err = 0;
    DisplayListInit();
    err = pthread_create(&tid, NULL, thread_LoadDisplayFile, NULL);
    if(err)
    {
        LOG_1("ERROR: create thread fail [%d]\n", err);
    }

    return;
}

int do_daemon(void)
{
	int fd = 0;
	fd = fork();
	if(0 <= fd)
	{
		if(0 == fd)
		{
			return 0;
		}
		_exit(0);
		return 0;
	}
	else
	{
		LOG_0("Failed to fork\n");
	}
}



int main(int argc, char* argv[])
{
    int ret;
    int count = 5;
    char buf[2];
    StuLcdDisplayInfo TmpDisplayInfo;

	if(log_init("/var/log/iic_led.log"))
	{
		printf("Failed to inti log\n");
		exit(1);
	}

    G_I2cFd = Oled_I2c_Init();
    if(G_I2cFd <= 0)
    {
        LOG_0("fail to open i2c device exit");
        _exit(-1);
    }
    OLED_Init();
    /*
    print2(0x80,'z');   // 在第1行第1位显示数字1 
    print2(0x40,'5');   // 在第1行第1位显示数字5
    print2(0x85,0x05);     // 在第1行第5位显示自定义字符
    print2(0x88,0xE4);     // 在第1行第9位 ASCII 中的upeer 4bit 1110  lower 4bit 0100对应的 μ
    print(0x42,"www.51cto.com");
    */

    do_daemon();
    DisplayThreadStart();
    
    #if 1
	MainThreadStart();

    for(;;)
    {
    	sleep(5);;
    }
#endif
    for(count=0; count<10; count++)
    {
        TmpDisplayInfo.ulLine = LCD_LINE_1;
        sprintf(TmpDisplayInfo.szBuf, "Line info [%d]", count);
        DisplayAddList(&TmpDisplayInfo);
        LOG_1("line1 add id[%d]\n", TmpDisplayInfo.ulId);
    }
    for(count=0; count<10; count++)
    {
        TmpDisplayInfo.ulLine = LCD_LINE_2;
        sprintf(TmpDisplayInfo.szBuf, "Line 2, info [%d] hello", count);
        DisplayAddList(&TmpDisplayInfo);
        LOG_1("line2 add id[%d]\n", TmpDisplayInfo.ulId);
    }
    while(1)
    {
        sleep(5);
    }
    return 0;
}
