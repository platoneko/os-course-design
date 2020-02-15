#include "killdialog.h"
#include "ui_killdialog.h"

#include <QMessageBox>

#include <sys/wait.h>
#include <sys/stat.h>

#define MAXLINE 1024

KillDialog::KillDialog(const int pid, const std::string comm, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KillDialog),
    pid(pid),
    comm(comm)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose,true);
    this->setWindowTitle("Kill Task");
    char line[MAXLINE];
    sprintf(line, "Sure to kill task with pid %d ?", pid);
    ui->promptLabel->setText(line);
    ui->commLabel->setText(comm.c_str());
}

KillDialog::~KillDialog()
{
    delete ui;
}

void KillDialog::on_cancelButton_clicked()
{
    this->close();
}

void KillDialog::on_confirmButton_clicked()
{
    if (kill(pid, SIGKILL) < 0) {
        struct stat statBuf;
        char s_pid[MAXLINE];
        sprintf(s_pid, "%d", pid);
        if (stat(s_pid, &statBuf) < 0) {
            QMessageBox::warning(this, "Task Terminated", "Task has terminated!");
        } else {
            QMessageBox::warning(this, "Permission Denied", "Permission Denied!");
        }
    }
    this->close();
}
