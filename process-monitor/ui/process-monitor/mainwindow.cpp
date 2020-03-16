#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    dir_ptr = opendir("/proc");
    chdir("/proc");

    FILE *fp;
    char pid_max[MAXLINE];
    fp = fopen("sys/kernel/pid_max", "r");
    fgets(pid_max, MAXLINE, fp);
    fclose(fp);
    this->pid_max = atoi(pid_max);

    ui->setupUi(this);
    this->setWindowTitle("Process Monitor");

    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    model = new QStandardItemModel();
    ui->taskTable->setModel(model);
    initTableModel();
    initSystemTab();

    connect(ui->taskTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(on_sectionClicked(int)));
    // connect(ui->memDetailButton, SIGNAL(clicked()), this, SLOT(on_memDetailButton_clicked()));
    connect(ui->taskTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_selectionChanged);

    update();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::update);
    timer->start(3000);

    ui->searchModeLabel->setVisible(false);
}

MainWindow::~MainWindow()
{
    closedir(dir_ptr);
    delete ui;
}

void MainWindow::update() {
    updateFree.acquire();
    updateSysinfo();
    updateTaskInfo();
    updateMemBar();
    updateCpuBar();
    updateLoadAverageLabel();
    updateUptimeLabel();
    updateTasksLabel();
    updateTaskTable();
    updateCurrentDatetimeLabel();
    updateFree.release();
}

void MainWindow::updateTaskInfo() {
    FILE *fp;
    struct dirent *dir_entry;
    char buf[MAXLINE], line[MAXLINE];

    unsigned short uid, pid, ppid;
    char priority, nice;
    int  pgrp, session, tty_nr, tpgid;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, num_threads;
    unsigned int flags;
    char state, comm[MAXLINE];

    // char command[MAXLINE];

    unsigned long virt, res, shr;

    cpuUsed = 0.0;
    taskTotal = taskRunning = taskSleeping = taskStopped = taskZombie = 0;
    matchedTaskTotal = 0;
    while ((dir_entry = readdir(dir_ptr))) {
        if (isNumeric(dir_entry->d_name)) {
            ++taskTotal;
            chdir(dir_entry->d_name);
            fp = fopen("status", "r");
            for (int j = 0; j < 9; ++j) {
                fgets(buf, MAXLINE, fp);
            }
            fclose(fp);
            sscanf(buf, "%s%hu", line, &uid);

            fp = fopen("stat", "r");
            fgets(buf, MAXLINE, fp);
            sscanf(buf, "%hu%s %c%hu%d%d%d%d%u%lu%lu%lu%lu%lu%lu%ld%ld%c%c%ld",
            &pid, comm, &state,
            &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
            &minflt, &cminflt, &majflt, &cmajflt,
            &utime, &stime, &cutime, &cstime,
            &priority, &nice,
            &num_threads);
            fclose(fp);

            fp = fopen("statm", "r");
            fgets(buf, MAXLINE, fp);
            sscanf(buf, "%lu%lu%lu", &virt, &res, &shr);
            fclose(fp);
            virt <<= 2;
            res <<= 2;
            shr <<= 2;

            /*
            fp = fopen("cmdline", "r");
            memset(buf, 0, MAXLINE);
            fgets(buf, MAXLINE, fp);
            fclose(fp);
            formatCommand(buf, command);
            */

            switch (state) {
            case 'R':
                ++taskRunning;
                break;
            case 'S':
                ++taskSleeping;
                break;
            case 'T':
                ++taskStopped;
                break;
            case 'Z':
                ++taskZombie;
                break;
            case 'D':
                ++taskSleeping;
                break;
            case 'I':
                ++taskSleeping;
                break;
            default:
                break;
            }

            if (taskInfoDict.count(pid)) {
                TaskInfo *taskInfo = taskInfoDict[pid];
                taskInfo->ppid = ppid;
                taskInfo->uid = uid;
                taskInfo->pri = priority;
                taskInfo->ni = nice;
                taskInfo->virt = virt;
                taskInfo->res = res;
                taskInfo->shr = shr;
                taskInfo->s = state;
                taskInfo->cpu = float(utime+stime-taskInfo->time)/3;
                taskInfo->mem = float(res)/currentSysinfo.totalram*100;
                taskInfo->time = utime+stime;
                taskInfo->comm = std::string(comm, 1, strlen(comm)-2);
                taskInfo->valid = true;
                cpuUsed += taskInfo->cpu;
                if (searchMode && strstr(comm, searchedCommand.c_str())) {
                    taskInfo->matched = true;
                    ++matchedTaskTotal;
                } else {
                    taskInfo->matched = false;
                }
            } else {
                taskInfoDict[pid] = new TaskInfo(pid, ppid, uid, priority, nice,
                                                 virt, res, shr,
                                                 state, float(res)/currentSysinfo.totalram*100,
                                                 utime+stime,
                                                 std::string(comm, 1, strlen(comm)-2));
                if (searchMode && strstr(comm, searchedCommand.c_str())) {
                    taskInfoDict[pid]->matched = true;
                    ++matchedTaskTotal;
                } else {
                    taskInfoDict[pid]->matched = false;
                }
            }
            chdir("..");
        }
    }
    rewinddir(dir_ptr);

    ++gcEra;
    /* garbage colletion */
    if (gcEra == gcInterval) {
        auto it = taskInfoDict.begin();
        while (it != taskInfoDict.end()) {
            if (!it->second->valid) {
                delete it->second;
                it = taskInfoDict.erase(it);
            } else {
                it++;
            }
        }
        gcEra = 0;
    }
}

/*
void MainWindow::on_memDetailButton_clicked() {
    if (!isMemDialogActive) {
        isMemDialogActive = true;
        memDialog = new MemDialog(this);
        memDialog->setModal(false);
        memDialog->show();
        memDialog->update(mem_total, mem_free, mem_used, mem_buff, mem_cache, mem_available, swap_total, swap_free, swap_used);
        connect(timer, &QTimer::timeout, this, &MainWindow::sendMemInfo);
        connect(memDialog, &MemDialog::closeSignal, this, &MainWindow::on_memDialog_closed);
    }
}
*/

/*
void MainWindow::sendMemInfo() {
    memDialog->update(mem_total, mem_free, mem_used, mem_buff, mem_cache, mem_available, swap_total, swap_free, swap_used);
}
*/

/*
void MainWindow::on_memDialog_closed() {
    disconnect(timer, &QTimer::timeout, this, &MainWindow::sendMemInfo);
    disconnect(memDialog, &MemDialog::closeSignal, this, &MainWindow::on_memDialog_closed);
    isMemDialogActive = false;
}
*/



void MainWindow::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Escape) {
        if (searchMode) {
            timer->stop();
            ui->searchModeLabel->setVisible(false);
            searchMode = false;
            update();
            timer->start(3000);
        }
    }

}
