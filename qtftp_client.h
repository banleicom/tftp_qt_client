#ifndef QTFTP_H
#define QTFTP_H

#include <QUdpSocket>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <QFile>

#define OPCODE_RRQ (1)
#define OPCODE_WRQ (2)
#define OPCODE_DATA (3)
#define OPCODE_ACK (4)
#define OPCODE_ERR (5)
#define OPCODE_OPACK (6)
#define BLOCKSIZE (512)

#define BIG2LITTLE16(A)   ((((A) & 0xFF00) >> 8) | (((A) & 0x00FF) << 8))

struct TFTPHeader{
    short opcode;
}__attribute__((packed));


struct TFTPWRRQ{
    struct TFTPHeader header;
    char *filename;	// Terminal as \0x00
    char *mode;	// Terminal as \0x00
}__attribute__((packed));

struct TFTPData{
    struct TFTPHeader header;
    short block;
    char data[];
}__attribute__((packed));

struct TFTPACK{
    struct TFTPHeader header;
    short block;
}__attribute__((packed));

struct TFTPERR{
    struct TFTPHeader header;
    short errcode;
    char *errmsg;	// Terminal as \0x00
}__attribute__((packed));


class Qtftp : public QUdpSocket
{
    Q_OBJECT
public:
    Qtftp();
    ~Qtftp();
    void QtftpInit(QHostAddress,quint16);
    void QtftpGet(const QString &pFilename);
    void QtftpPut(const QString &pFilename);
private:
    void getReadReq(const QString &pFilename);
    void putReadReq(const QString &pFilename);
    void sendData(struct TFTPData *Data,char *line, quint16 senderPort,int);
    void sendDataAck(struct TFTPData *pData,quint16 senderPort);

signals:
    void tftpPutProgress(uint8_t);
    void tftpPutDone();
    void tftpGetDone();
private slots:
    void readPendingDatagrams();


protected:
    bool tftp_on_off;
    bool station;                    //  false 读    true 写
    long long w_File_size;           //文件总大小
    long long w_time_size;           //文件剩余大小
    int r_block_size;
    int Block_current;               //当前包数

    QDataStream *inout;
    QHostAddress host_ip;
    quint16 host_port;
    QFile * pFile=NULL;
    char RecvData[2048];
};

#endif // QTFTP_H
