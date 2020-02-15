#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "TaskInfo.h"
#include "memdialog.h"
#include "killdialog.h"
#include "nicedialog.h"

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QScrollBar>

#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>

#include <cstring>
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void update();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QStandardItemModel *model;
    std::unordered_map<int, TaskInfo> taskInfoDict;
    unsigned long mem_total, mem_used, mem_free, mem_shared, mem_buff, mem_cache, mem_available;
    unsigned long swap_total, swap_used, swap_free;
    int taskTotal, taskRunning, taskSleeping, taskStopped, taskZombie;
    DIR *dir_ptr;
    unsigned char sortMethod;
    enum sortMethodType {PID_A, PID_D,
                         USER_A, USER_D,
                         COMMAND_A, COMMAND_D,
                         PRI_A, PRI_D,
                         NI_A, NI_D,
                         VIRT_A, VIRT_D,
                         RES_A, RES_D,
                         SHR_A, SHR_D,
                         S_A, S_D,
                         CPU_A, CPU_D,
                         MEM_A, MEM_D,
                         TIME_A, TIME_D};
    const std::unordered_map<char, int> statePriority = {{'R', 0}, {'S', 1}, {'I', 2}, {'T', 3}, {'D', 4}, {'Z', 5}};

    MemDialog *memDialog;
    bool isMemDialogActive = false;
    KillDialog *killDialog;
    NiceDialog *niceDialog;

    int currSelectedRow = 0;
    int currScrollValue = 0;

    const int gcInterval = 10;
    int gcCount = 0;

private slots:
    void on_sectionClicked(int index);
    void on_memDetailButton_clicked();
    void on_memDialog_closed();
    void on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_killButton_clicked();

    void on_niceButton_clicked();

private:
    void initTableModel();
    void updateLoadAverage();
    void updateUptime();
    void updateMem();
    void updateTaskInfo();
    void displayTaskInfo();
    static void formatCommand(char *src, char *dest);
    static void formatSize(unsigned long l_size, char *s_size);
    static void formatTime(unsigned long l_time, char *s_time);
    static bool isNumeric(const std::string &s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    }
    void sortTable();
    void sendMemInfo();
};


class TableItem: public QStandardItem
{
public:
    TableItem(){}

    TableItem(const QString &text)
        :QStandardItem(text)
    {
    }

    TableItem(const TableItem &other)
        : QStandardItem(other)
    {
    }

    TableItem &operator=(const TableItem &other)
    {
        QStandardItem::operator=(other);
        return *this;
    }


    virtual bool operator<(const QStandardItem &other) const
    {
        const QVariant l = data(Qt::UserRole), r = other.data(Qt::UserRole);
        if (column() == other.column()) {
            if ((other.column() >= 5 && other.column() <= 7) || other.column() == 11) {
                return l.toUInt() < r.toUInt();
            } else if (other.column() == 9 || other.column() == 10) {
                return l.toFloat() < r.toFloat();
            } else if (other.column() == 8) {
                return l.toInt() < r.toInt();
            }
        }
        return QStandardItem::operator<(other);
    }
};
#endif // MAINWINDOW_H
