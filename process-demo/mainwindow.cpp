#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "pv.h"

#include <unistd.h>


extern int srcfd, tgtfd;
extern int readMu, writeMu;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    copy();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::copy() {
    int n;
    char buf[128], msg[128];
    while (1) {
        P(readMu, 0);
        n = read(srcfd, buf, sizeof(buf));
        P(writeMu, 0);
        V(readMu, 0);
        if (n <= 0) {
            V(writeMu, 0);
            break;
        }
        write(tgtfd, buf, n);
        V(writeMu, 0);
        sprintf(msg, "copy %d bytes\n", n);
        ui->textBrowser->insertPlainText(QString(msg));
        if (n != sizeof(buf)) {
            break;
        }
    }
}
