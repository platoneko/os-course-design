#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define MAXLINE 1024

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    dir_ptr = opendir("/proc");
    chdir("/proc");

    ui->setupUi(this);
    this->setWindowTitle("Process Monitor");

    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    model = new QStandardItemModel();
    ui->taskTable->setModel(model);
    initTableModel();

    taskTotal = taskRunning = taskSleeping = taskStopped = taskZombie = 0;
    sortMethod = S_A;

    connect(ui->taskTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(on_sectionClicked(int)));
    connect(ui->memDetailButton, SIGNAL(clicked()), this, SLOT(on_memDetailButton_clicked()));
    connect(ui->taskTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::on_selectionChanged);
    update();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::update);
    timer->start(3000);
}

MainWindow::~MainWindow()
{
    closedir(dir_ptr);
    delete ui;
}

void MainWindow::update() {
    updateLoadAverage();
    updateUptime();
    updateMem();
    updateTaskInfo();
    displayTaskInfo();
}

void MainWindow::initTableModel() {
    model->setColumnCount(12);
    model->setHeaderData(0, Qt::Horizontal, "PID");
    model->setHeaderData(1, Qt::Horizontal, "USER");
    model->setHeaderData(2, Qt::Horizontal, "Command");
    model->setHeaderData(3, Qt::Horizontal, "PRI");
    model->setHeaderData(4, Qt::Horizontal, "NI");
    model->setHeaderData(5, Qt::Horizontal, "VIRT");
    model->setHeaderData(6, Qt::Horizontal, "RES");
    model->setHeaderData(7, Qt::Horizontal, "SHR");
    model->setHeaderData(8, Qt::Horizontal, "S");
    model->setHeaderData(9, Qt::Horizontal, "CPU%");
    model->setHeaderData(10, Qt::Horizontal, "MEM%");
    model->setHeaderData(11, Qt::Horizontal, "TIME+");

    ui->taskTable->verticalHeader()->hide();
    ui->taskTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->taskTable->setSortingEnabled(false);

    ui->taskTable->setColumnWidth(0,60);
    ui->taskTable->setColumnWidth(1,120);
    ui->taskTable->setColumnWidth(2,200);
    ui->taskTable->setColumnWidth(3,60);
    ui->taskTable->setColumnWidth(4,60);
    ui->taskTable->setColumnWidth(5,80);
    ui->taskTable->setColumnWidth(6,80);
    ui->taskTable->setColumnWidth(7,80);
    ui->taskTable->setColumnWidth(8,30);
    ui->taskTable->setColumnWidth(9,80);
    ui->taskTable->setColumnWidth(10,80);
    ui->taskTable->setColumnWidth(11,150);
}

void MainWindow::updateLoadAverage() {
    char buf[BUFSIZ];
    float load_1, load_5, load_15;
    FILE *fp = fopen("loadavg", "r");
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    sscanf(buf, "%f %f %f", &load_1, &load_5, &load_15);
    sprintf(buf, "Load average: %.2f %.2f %.2f", load_1, load_5, load_15);
    ui->loadAverage->setText(buf);
}

void MainWindow::updateUptime() {
    char buf[BUFSIZ];
    int uptime, hs, ms, ss;
    FILE *fp = fopen("uptime", "r");
    fgets(buf, BUFSIZ, fp);
    fclose(fp);
    sscanf(buf, "%d", &uptime);
    hs = uptime/3600;
    ms = (uptime%3600)/60;
    ss = uptime%60;
    sprintf(buf, "Uptime: %2d:%02d:%02d", hs, ms, ss);
    ui->uptime->setText(buf);
}

void MainWindow::updateMem() {
    char buf[BUFSIZ], line[MAXLINE];

    FILE *fp = fopen("meminfo", "r");
    fgets(buf, BUFSIZ, fp);  // MemTotal
    sscanf(buf, "%s%lu", line, &mem_total);
    fgets(buf, BUFSIZ, fp);  // MemFree
    sscanf(buf, "%s%lu", line, &mem_free);
    fgets(buf, BUFSIZ, fp);  // MemAvailable
    sscanf(buf, "%s%lu", line, &mem_available);
    fgets(buf, BUFSIZ, fp);  // Buffers
    sscanf(buf, "%s%lu", line, &mem_buff);
    fgets(buf, BUFSIZ, fp);  // Cached
    sscanf(buf, "%s%lu", line, &mem_cache);
    fgets(buf, BUFSIZ, fp);  // SwapCached
    fgets(buf, BUFSIZ, fp);  // Active
    fgets(buf, BUFSIZ, fp);  // Inactive
    fgets(buf, BUFSIZ, fp);  // Active(anon)
    fgets(buf, BUFSIZ, fp);  // Inactive(anon)
    fgets(buf, BUFSIZ, fp);  // Active(file)
    fgets(buf, BUFSIZ, fp);  // Inactive(file)
    fgets(buf, BUFSIZ, fp);  // Unevictable
    fgets(buf, BUFSIZ, fp);  // Mlocked
    fgets(buf, BUFSIZ, fp);  // SwapTotal
    sscanf(buf, "%s%lu", line, &swap_total);
    fgets(buf, BUFSIZ, fp);  // SwapFree
    sscanf(buf, "%s%lu", line, &swap_free);
    fgets(buf, BUFSIZ, fp);  // Dirty
    fgets(buf, BUFSIZ, fp);  // Writeback
    fgets(buf, BUFSIZ, fp);  // AnonPages
    fgets(buf, BUFSIZ, fp);  // Mapped
    fgets(buf, BUFSIZ, fp);  // Shmem
    sscanf(buf, "%s%lu", line, &mem_shared);
    fclose(fp);

    mem_used = mem_total - mem_free - mem_buff - mem_cache;
    swap_used = swap_total - swap_free;
    ui->memBar->setValue((mem_total-mem_free)*100/mem_total);
    ui->swpBar->setValue(swap_used*100/swap_total);
}

void MainWindow::updateTaskInfo() {
    FILE *fp;
    struct dirent *dir_entry;
    char buf[BUFSIZ], line[MAXLINE];

    int uid;

    int pid, ppid, pgrp, session, tty_nr, tpgid, priority, nice;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
    long cutime, cstime, num_threads;
    unsigned int flags;
    char state, comm[MAXLINE];

    char command[MAXLINE];

    unsigned long virt, res, shr;

    float cpuUsed = 0.0;
    taskTotal = taskRunning = taskSleeping = taskStopped = taskZombie = 0;
    while ((dir_entry = readdir(dir_ptr))) {
        if (isNumeric(dir_entry->d_name)) {
            ++taskTotal;
            chdir(dir_entry->d_name);
            fp = fopen("status", "r");
            for (int j = 0; j < 9; ++j) {
                fgets(buf, BUFSIZ, fp);
            }
            fclose(fp);
            sscanf(buf, "%s%d", line, &uid);

            fp = fopen("stat", "r");
            fgets(buf, BUFSIZ, fp);
            sscanf(buf, "%d%s %c%d%d%d%d%d%u%lu%lu%lu%lu%lu%lu%ld%ld%d%d%ld",
            &pid, comm, &state,
            &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
            &minflt, &cminflt, &majflt, &cmajflt,
            &utime, &stime, &cutime, &cstime,
            &priority, &nice,
            &num_threads);
            fclose(fp);

            fp = fopen("statm", "r");
            fgets(buf, BUFSIZ, fp);
            sscanf(buf, "%lu%lu%lu", &virt, &res, &shr);
            fclose(fp);
            virt <<= 2;
            res <<= 2;
            shr <<= 2;

            fp = fopen("cmdline", "r");
            memset(buf, 0, BUFSIZ);
            fgets(buf, BUFSIZ, fp);
            fclose(fp);
            formatCommand(buf, command);

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
                TaskInfo &taskInfo = taskInfoDict[pid];
                taskInfo.uid = uid;
                taskInfo.pri = priority;
                taskInfo.ni = nice;
                taskInfo.virt = virt;
                taskInfo.res = res;
                taskInfo.shr = shr;
                taskInfo.s = state;
                taskInfo.cpu = float(utime+stime-taskInfo.time)/3;
                taskInfo.mem = float(res)/mem_total*100;
                taskInfo.time = utime+stime;
                taskInfo.command = command;
                taskInfo.comm = comm;
                taskInfo.dirty = 1;
                cpuUsed += taskInfo.cpu;
            } else {
                taskInfoDict[pid] = TaskInfo(pid, uid, priority, nice,
                                             virt, res, shr,
                                             state, float(res)/mem_total,
                                             utime+stime,
                                             command, comm);
            }
            chdir("..");
        }
    }
    rewinddir(dir_ptr);

    ++gcCount;
    if (gcCount == gcInterval) {
        auto it = taskInfoDict.begin();
        while (it != taskInfoDict.end()) {
            if (it->second.dirty == 0) {
                it = taskInfoDict.erase(it);
            } else {
                it++;
            }
        }
        gcCount = 0;
    }
    sprintf(buf, "Tasks: %4d total, %4d running, %4d sleeping, %4d stopped, %4d zombie",
            taskTotal, taskRunning, taskSleeping, taskStopped, taskZombie);
    ui->tasks->setText(buf);
    ui->cpuBar->setValue(int(cpuUsed));
}

void MainWindow::displayTaskInfo() {
    int rowCount = model->rowCount();
    if (rowCount < taskTotal) {
        for (int i = rowCount; i < taskTotal; ++i) {
            for (int j = 0; j < 12; ++j) {
                model->setItem(i, j, new TableItem());
                model->item(i, j)->setEditable(false);
            }
        }
    }

    model->setRowCount(taskTotal);
    int row = 0;
    char s_virt[MAXLINE], s_res[MAXLINE], s_shr[MAXLINE], s_time[MAXLINE];
    for (auto &taskInfo: taskInfoDict) {
        if (taskInfo.second.dirty) {
            model->item(row, 0)->setData(taskInfo.second.pid, Qt::DisplayRole);
            model->item(row, 1)->setData(getpwuid(uid_t(taskInfo.second.uid))->pw_name, Qt::DisplayRole);
            if (taskInfo.second.command.length() == 0) {
                model->item(row, 2)->setData(taskInfo.second.comm.c_str(), Qt::DisplayRole);
            } else {
                model->item(row, 2)->setData(taskInfo.second.command.c_str(), Qt::DisplayRole);
            }
            model->item(row, 3)->setData(taskInfo.second.pri, Qt::DisplayRole);
            model->item(row, 4)->setData(taskInfo.second.ni, Qt::DisplayRole);
            formatSize(taskInfo.second.virt, s_virt);
            model->item(row, 5)->setData(s_virt, Qt::DisplayRole);
            formatSize(taskInfo.second.res, s_res);
            model->item(row, 6)->setData(s_res, Qt::DisplayRole);
            formatSize(taskInfo.second.shr, s_shr);
            model->item(row, 7)->setData(s_shr, Qt::DisplayRole);
            model->item(row, 8)->setData(QChar(taskInfo.second.s), Qt::DisplayRole);
            model->item(row, 9)->setData(QString::number(taskInfo.second.cpu, 'f', 1), Qt::DisplayRole);
            model->item(row, 10)->setData(QString::number(taskInfo.second.mem, 'f', 1), Qt::DisplayRole);
            formatTime(taskInfo.second.time, s_time);
            model->item(row, 11)->setData(s_time, Qt::DisplayRole);
            model->item(row, 5)->setData(qlonglong(taskInfo.second.virt), Qt::UserRole);
            model->item(row, 6)->setData(qlonglong(taskInfo.second.res), Qt::UserRole);
            model->item(row, 7)->setData(qlonglong(taskInfo.second.shr), Qt::UserRole);
            model->item(row, 11)->setData(qlonglong(taskInfo.second.time), Qt::UserRole);
            model->item(row, 9)->setData(taskInfo.second.cpu, Qt::UserRole);
            model->item(row, 10)->setData(taskInfo.second.mem, Qt::UserRole);
            model->item(row, 8)->setData(statePriority.at(taskInfo.second.s), Qt::UserRole);
            taskInfo.second.dirty = 0;
            ++row;
        }
    }
    sortTable();
    currScrollValue = ui->taskTable->verticalScrollBar()->value();
    ui->taskTable->selectRow(currSelectedRow);
    ui->taskTable->verticalScrollBar()->setValue(currScrollValue);
}

void MainWindow::formatCommand(char *src, char *dest) {
    int cnt = 0, i = 0;
    while (cnt != 2 && i < MAXLINE) {
        if (*src == 0) {
            ++cnt;
            *dest++ = ' ';
        } else {
            cnt = 0;
            *dest++ = *src++;
        }
        ++i;
    }
    *(dest-2) = 0;
}

void MainWindow::formatSize(unsigned long l_size, char *s_size) {
    if (l_size < 10000) {
        if (l_size == 0) {
            strcpy(s_size, "0");
        } else {
            sprintf(s_size, "%luK", l_size);
        }
    } else {
        l_size >>= 10;
        if (l_size < 10000) {
            sprintf(s_size, "%luM", l_size);
        } else {
            l_size >>= 10;
            sprintf(s_size, "%luG", l_size);
        }
    }
}

void MainWindow::formatTime(unsigned long l_time, char *s_time) {
    unsigned long m;
    float s;
    m = l_time/6000;
    s = (float)l_time/100 - m*60;
    if (m >= 60) {
        unsigned long h = m/60;
        m = m%60;
        sprintf(s_time, "%ldh%02ld:%d", h, m, int(s));
    } else {
        sprintf(s_time, "%ld:%05.2f", m, s);
    }
}

void MainWindow::on_sectionClicked(int index) {
    switch (index) {
    case 0:
        if (sortMethod == PID_A) {
            sortMethod = PID_D;
        } else {
            sortMethod = PID_A;
        }
        break;
    case 1:
        if (sortMethod == USER_A) {
            sortMethod = USER_D;
        } else {
            sortMethod = USER_A;
        }
        break;
    case 2:
        if (sortMethod == COMMAND_A) {
            sortMethod = COMMAND_D;
        } else {
            sortMethod = COMMAND_A;
        }
        break;
    case 3:
        if (sortMethod == PRI_A) {
            sortMethod = PRI_D;
        } else {
            sortMethod = PRI_A;
        }
        break;
    case 4:
        if (sortMethod == NI_A) {
            sortMethod = NI_D;
        } else {
            sortMethod = NI_A;
        }
        break;
    case 5:
        if (sortMethod == VIRT_D) {
            sortMethod = VIRT_A;
        } else {
            sortMethod = VIRT_D;
        }
        break;
    case 6:
        if (sortMethod == RES_D) {
            sortMethod = RES_A;
        } else {
            sortMethod = RES_D;
        }
        break;
    case 7:
        if (sortMethod == SHR_D) {
            sortMethod = SHR_A;
        } else {
            sortMethod = SHR_D;
        }
        break;
    case 8:
        if (sortMethod == S_A) {
            sortMethod = S_D;
        } else {
            sortMethod = S_A;
        }
        break;
    case 9:
        if (sortMethod == CPU_D) {
            sortMethod = CPU_A;
        } else {
            sortMethod = CPU_D;
        }
        break;
    case 10:
        if (sortMethod == MEM_D) {
            sortMethod = MEM_A;
        } else {
            sortMethod = MEM_D;
        }
        break;
    case 11:
        if (sortMethod == TIME_D) {
            sortMethod = TIME_A;
        } else {
            sortMethod = TIME_D;
        }
        break;
    default:
        break;
    }
    sortTable();
    currScrollValue = ui->taskTable->verticalScrollBar()->value();
    ui->taskTable->selectRow(currSelectedRow);
    ui->taskTable->verticalScrollBar()->setValue(currScrollValue);
}

void MainWindow::sortTable() {
    switch (sortMethod) {
    case PID_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        break;
    case PID_D:
        ui->taskTable->sortByColumn(0, Qt::DescendingOrder);
        break;
    case USER_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(1, Qt::AscendingOrder);
        break;
    case USER_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(1, Qt::DescendingOrder);
        break;
    case COMMAND_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(2, Qt::AscendingOrder);
        break;
    case COMMAND_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(2, Qt::DescendingOrder);
        break;
    case PRI_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(3, Qt::AscendingOrder);
        break;
    case PRI_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(3, Qt::DescendingOrder);
        break;
    case NI_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(4, Qt::AscendingOrder);
        break;
    case NI_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(4, Qt::DescendingOrder);
        break;
    case VIRT_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(5, Qt::AscendingOrder);
        break;
    case VIRT_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(5, Qt::DescendingOrder);
        break;
    case RES_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(6, Qt::AscendingOrder);
        break;
    case RES_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(6, Qt::DescendingOrder);
        break;
    case SHR_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(7, Qt::AscendingOrder);
        break;
    case SHR_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(7, Qt::DescendingOrder);
        break;
    case S_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(8, Qt::AscendingOrder);
        break;
    case S_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(8, Qt::DescendingOrder);
        break;
    case CPU_A:
        ui->taskTable->sortByColumn(9, Qt::AscendingOrder);
        break;
    case CPU_D:
        ui->taskTable->sortByColumn(9, Qt::DescendingOrder);
        break;
    case MEM_A:
        ui->taskTable->sortByColumn(10, Qt::AscendingOrder);
        break;
    case MEM_D:
        ui->taskTable->sortByColumn(10, Qt::DescendingOrder);
        break;
    case TIME_A:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(11, Qt::AscendingOrder);
        break;
    case TIME_D:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(11, Qt::DescendingOrder);
        break;
    default:
        ui->taskTable->sortByColumn(0, Qt::AscendingOrder);
        ui->taskTable->sortByColumn(8, Qt::AscendingOrder);
        break;
    }
}

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

void MainWindow::sendMemInfo() {
    memDialog->update(mem_total, mem_free, mem_used, mem_buff, mem_cache, mem_available, swap_total, swap_free, swap_used);
}

void MainWindow::on_memDialog_closed() {
    disconnect(timer, &QTimer::timeout, this, &MainWindow::sendMemInfo);
    disconnect(memDialog, &MemDialog::closeSignal, this, &MainWindow::on_memDialog_closed);
    isMemDialogActive = false;
}

void MainWindow::on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    currSelectedRow = selected.indexes().first().row();
}

void MainWindow::on_killButton_clicked() {
    int pid = model->item(currSelectedRow, 0)->data(Qt::DisplayRole).toInt();
    killDialog = new KillDialog(pid, taskInfoDict[pid].comm, this);
    killDialog->setModal(true);
    killDialog->show();
}

void MainWindow::on_niceButton_clicked() {
    int pid = model->item(currSelectedRow, 0)->data(Qt::DisplayRole).toInt();
    niceDialog = new NiceDialog(pid, taskInfoDict[pid].comm, taskInfoDict[pid].ni, this);
    niceDialog->setModal(true);
    niceDialog->show();
}
