#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qtftp_client.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void getDone();
    void putPross(uint8_t);
private:
    Qtftp *tftp;
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
