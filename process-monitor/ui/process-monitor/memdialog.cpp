#include "memdialog.h"
#include "ui_memdialog.h"

#include <QDebug>

MemDialog::MemDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MemDialog)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose,true);
}

MemDialog::~MemDialog()
{
    delete ui;
}

void MemDialog::update(unsigned long mem_total,
                       unsigned long mem_free,
                       unsigned long mem_used,
                       unsigned long mem_buff,
                       unsigned long mem_cache,
                       unsigned long mem_available,
                       unsigned long swap_total,
                       unsigned long swap_free,
                       unsigned long swap_used) {
    char buf[BUFSIZ];
    sprintf(buf, "MiB Mem : %8.1f total, %8.1f free, %8.1f used, %8.1f buff/cache",
            double(mem_total)/1024,
            double(mem_free)/1024,
            double(mem_used)/1024,
            double(mem_buff+mem_cache)/1024);
    ui->memInfo->setText(buf);
    sprintf(buf, "MiB Swap : %8.1f total, %8.1f free, %8.1f used. %8.1f avail Mem",
            double(swap_total)/1024,
            double(swap_free)/1024,
            double(swap_used)/1024,
            double(mem_available)/1024);
    ui->swpInfo->setText(buf);
}

void MemDialog::closeEvent(QCloseEvent *event) {
    emit closeSignal();
}
