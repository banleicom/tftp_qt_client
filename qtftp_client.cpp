#include "qtftp_client.h"

#include <QMainWindow>
#include <QHostAddress>
#include <QtNetwork>
#include <QtWidgets>
#include <QUdpSocket>
#include <QFile>

Qtftp::Qtftp():pFile(NULL),inout(NULL)
{

}

Qtftp::~Qtftp()
{

}

void Qtftp::QtftpInit(QHostAddress ip,quint16 port)
{
    host_ip=ip;
    host_port=port;
    bind(QHostAddress::AnyIPv4,localPort());

    connect(this,SIGNAL(readyRead()),
            this,SLOT(readPendingDatagrams()));
    memset(RecvData,0,sizeof(RecvData));
}

void Qtftp::QtftpGet(const QString &pFilename)
{
    if(pFile != NULL)
    {
        delete pFile;
        pFile = new QFile(pFilename);
    }else{
        pFile = new QFile(pFilename);
    }

    if(!pFile->open(QIODevice::WriteOnly))
        return ;
    if(inout != NULL)
    {
       delete pFile;
       inout = new QDataStream(pFile);
    }else{
       inout = new QDataStream(pFile);
    }

    station=false;
    Block_current = 0;
    getReadReq(pFilename);
}

void Qtftp::QtftpPut(const QString &pFilename)
{
    if(pFile != NULL)
    {
        delete pFile;
        pFile = new QFile(pFilename);
    }else{
        pFile = new QFile(pFilename);
    }

    if(!pFile->open(QIODevice::ReadOnly))
        return ;
    if(inout != NULL)
    {
       delete pFile;
       inout = new QDataStream(pFile);
    }else{
       inout = new QDataStream(pFile);
    }

    w_File_size = pFile->size();
    w_time_size=w_File_size;

    station=true;        //写
    Block_current = 0;

    putReadReq(pFilename);
}

void Qtftp::getReadReq(const QString &pFilename)
{
    char*Filename = pFilename.toLatin1().data();
    struct TFTPHeader header;
    header.opcode = htons(OPCODE_RRQ);
    int filenamelen = strlen(Filename) + 1;
    qint64 packetsize = sizeof(header) + filenamelen + strlen("octet") + 1;
    char *packet = (char*)malloc(packetsize);	// mode = "octet"
    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), Filename, filenamelen);
    const char *mode = "octet";                                                  //
    memcpy(packet + sizeof(header) + filenamelen, mode, strlen(mode) + 1);
    int bytes = QUdpSocket::writeDatagram(packet,packetsize,host_ip,host_port);
    free(packet);
}

void Qtftp::putReadReq(const QString &pFilename)
{
    char*Filename = pFilename.toLatin1().data();
    // qDebug()<< "    file name"<< pFilename  ;
    struct TFTPHeader header;
    header.opcode = htons(OPCODE_WRQ);
    int filenamelen = strlen(Filename) + 1;
    qint16 packetsize = sizeof(header) + filenamelen + strlen("octet") + 1;
    char*packet = (char*)malloc(packetsize);	// mode = "octet"
    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), Filename, filenamelen);
    const char *mode = "octet";
    memcpy(packet + sizeof(header) + filenamelen, mode, strlen(mode) + 1);
    int bytes = QUdpSocket::writeDatagram((char*)packet,packetsize,host_ip,host_port);
    free(packet);
}

void Qtftp::sendData(TFTPData *Data, char *line, quint16 senderPort,int fileLen)
{
    int packetsize = sizeof(TFTPHeader)+sizeof(Data->block)+fileLen;
    char *packet = (char*)malloc(packetsize);
    memcpy(packet, &Data->header, sizeof(TFTPHeader));
    memcpy(packet+sizeof(Data->header), &Data->block,sizeof(Data->block));
    memcpy(packet+sizeof(Data->header)+sizeof(Data->block),line,fileLen);

    int bytes = QUdpSocket::writeDatagram((char*)packet,packetsize,host_ip,senderPort);
    free(packet);                //  malloc 需要释放
}

void Qtftp::sendDataAck(TFTPData *pData, quint16 senderPort)
{
    struct TFTPACK ack;
    ack.header.opcode = htons(OPCODE_ACK);
    ack.block = (pData->block);

    int packetsize = sizeof(TFTPHeader)+sizeof(ack.block);
    char *packet = (char*)malloc(packetsize);	// mode = "octet"
    memcpy(packet, &ack.header, sizeof(TFTPHeader));
    memcpy(packet+sizeof(TFTPHeader), &ack.block,sizeof(ack.block));

    int bytes = writeDatagram((char*)packet,packetsize,host_ip,senderPort);
    free(packet);
}

void Qtftp::readPendingDatagrams()
{

    while(hasPendingDatagrams())
    {
        QHostAddress sender;
        quint16 senderPort;
        int readbytes = readDatagram(RecvData,sizeof(RecvData),
                                                &sender,&senderPort);
        struct TFTPHeader *header = (struct TFTPHeader*)RecvData;

        switch(ntohs(header->opcode))
        {
        case OPCODE_DATA:
        {
            struct TFTPData *data = (struct TFTPData*)RecvData;
            if(readbytes-sizeof(struct TFTPHeader)-sizeof(short) == BLOCKSIZE)
            {
                inout->writeRawData(data->data,readbytes-sizeof(struct TFTPHeader)-sizeof(short));
                sendDataAck(data,senderPort);
                Block_current++;

            }else{

                inout->writeRawData(data->data,readbytes-sizeof(struct TFTPHeader)-sizeof(short));
                sendDataAck(data,senderPort);
                pFile->close();
                emit tftpGetDone();
            }

        } break;
        case OPCODE_ACK:
        {
            struct TFTPACK *ack = (struct TFTPACK*)RecvData;
            struct TFTPData Data;
            Data.header.opcode = htons(OPCODE_DATA);
            Data.block= htons(ntohs(ack->block)+1);
            char data[BLOCKSIZE];
            memset(data,1,BLOCKSIZE*sizeof(char));
            Block_current++;

            if(w_time_size>0)
            {
                int filelen=inout->readRawData(data,BLOCKSIZE);
                w_time_size=w_time_size-filelen;
                sendData(&Data,data,senderPort,filelen);
                emit tftpPutProgress(w_time_size*100/w_File_size);
            }else{
                 sendData(&Data,data,senderPort,0);
                 emit tftpPutDone();
                 pFile->close();
                 delete pFile;
                 delete inout;
                 pFile=NULL;
                 inout=NULL;
                 return;
            }
        } break;
        case OPCODE_OPACK:
        {
            if(station==false){                               //read  date ACK, the first ACK
                struct TFTPACK ack;
                ack.header.opcode = htons(OPCODE_ACK);
                ack.block= htons(0);                          //
                int packetsize = sizeof(TFTPHeader)+sizeof(ack.block);
                char *packet = (char*)malloc(packetsize);	// mode = "octet"
                memcpy(packet, &ack.header, sizeof(TFTPHeader));
                memcpy(packet+sizeof(TFTPHeader), &ack.block,sizeof(ack.block));
                int bytes = writeDatagram((char*)packet,packetsize,host_ip,senderPort);
                Block_current++;
                if(bytes<0){

                }
            }else{                                       //wirte date  ,the first  date
                struct TFTPData Data;
                char data[BLOCKSIZE];
                memset(data,1,BLOCKSIZE*sizeof(char));
                Data.header.opcode = htons(OPCODE_DATA);
                Data.block= htons(1);
                if(w_File_size>0){
                    int fileLen=inout->readRawData(data,BLOCKSIZE);
                    sendData(&Data,data,senderPort,fileLen);
                }else{
                    sendData(&Data,data,senderPort,0);
                    pFile->close();
                    delete pFile;
                    delete inout;
                    pFile=NULL;
                    inout=NULL;
                    return;
                }
            }
        }break;
        default:
            qDebug("      No match %d",ntohs(header->opcode));
        }
    }
}
