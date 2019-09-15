#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qtftp_client.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //ui->setupUi(this);
    tftp=new Qtftp();
    connect(tftp,SIGNAL(tftpGetDone()),this,SLOT(getDone()));
    connect(tftp,SIGNAL(tftpPutProgress(uint8_t)),this,SLOT(putPross(uint8_t)));
    tftp->QtftpInit(QHostAddress("10.1.131.19"),(quint16)69);
    //tftp->QtftpGet("tftp.cpp");
    tftp->QtftpPut("tftp.cpp");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getDone()
{
    qDebug()<<"get done";
}

void MainWindow::putPross(uint8_t num)
{
    qDebug()<<num;
}
