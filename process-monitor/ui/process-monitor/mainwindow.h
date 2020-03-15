#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "TaskInfo.h"
#include "killdialog.h"
#include "nicedialog.h"
#include "rundialog.h"
#include "searchdialog.h"
#include "global.h"

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QScrollBar>
#include <QKeyEvent>
#include <QSemaphore>
#include <QMessageBox>

#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>

#include <cstring>
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

static constexpr int COLUMN_NUM = 13;
static constexpr int PID_COLUMN = 0;
static constexpr int PPID_COLUMN = 1;
static constexpr int USER_COLUMN = 2;
static constexpr int COMMAND_COLUMN = 3;
static constexpr int PRI_COLUMN = 4;
static constexpr int NI_COLUMN = 5;
static constexpr int VIRT_COLUMN = 6;
static constexpr int RES_COLUMN = 7;
static constexpr int SHR_COLUMN = 8;
static constexpr int S_COLUMN = 9;
static constexpr int CPU_COLUMN = 10;
static constexpr int MEM_COLUMN = 11;
static constexpr int TIME_COLUMN = 12;

enum SortMethodType {PID_ASC, PID_DES,
                     PPID_ASC, PPID_DES,
                     USER_ASC, USER_DES,
                     COMMAND_ASC, COMMAND_DES,
                     PRI_ASC, PRI_DES,
                     NI_ASC, NI_DES,
                     VIRT_ASC, VIRT_DES,
                     RES_ASC, RES_DES,
                     SHR_ASC, SHR_DES,
                     S_ASC, S_DES,
                     CPU_ASC, CPU_DES,
                     MEM_ASC, MEM_DES,
                     TIME_ASC, TIME_DES};

static QSemaphore updateFree(1);

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
    std::unordered_map<int, TaskInfo *> taskInfoDict;
    unsigned long mem_total = 0, mem_used = 0, mem_free = 0, mem_shared = 0, mem_buff = 0, mem_cache = 0, mem_available = 0;
    unsigned long swap_total = 0, swap_used = 0, swap_free = 0;
    int taskTotal = 0, taskRunning = 0, taskSleeping = 0, taskStopped = 0, taskZombie = 0;
    int matchedTaskTotal = 0;
    DIR *dir_ptr;
    unsigned char sortMethod = S_ASC;
    const std::unordered_map<char, int> statePriority = {{'R', 0}, {'S', 1}, {'I', 2}, {'T', 3}, {'D', 4}, {'Z', 5}};

    // MemDialog *memDialog;
    // bool isMemDialogActive = false;
    KillDialog *killDialog;
    NiceDialog *niceDialog;
    RunDialog *runDialog;
    SearchDialog *searchDialog;

    int pid_max;

    int currSelectedRow = 0;
    int currVerticalScrollValue = 0;
    int currHorizontalScrollValue = 0;

    const int gcInterval = 10;
    int gcEra = 0;

    bool searchMode = false;
    bool searchSignal = false;
    std::string searchedCommand;

private slots:
    void on_sectionClicked(int index);
    // void on_memDetailButton_clicked();
    // void on_memDialog_closed();
    void on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_killButton_clicked();

    void on_niceButton_clicked();

    void on_runButton_clicked();

    void on_searchButton_clicked();

    void searchPid(int pid);
    void searchCommand(const std::string command);
    void on_searchDialog_closed();

    void keyPressEvent(QKeyEvent *event);

private:
    void initTableModel();
    void updateLoadAverage();
    void updateUptime();
    void updateMem();
    void updateTaskInfo();
    void updateTaskTable();
    static void formatCommand(char *src, char *dest);
    static void formatSize(unsigned long l_size, char *s_size);
    static void formatTime(unsigned long l_time, char *s_time);
    static bool isNumeric(const std::string &s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    }
    void sortTable();
    // void sendMemInfo();
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
            if ((other.column() >= VIRT_COLUMN && other.column() <= SHR_COLUMN) || other.column() == TIME_COLUMN) {
                return l.toUInt() < r.toUInt();
            } else if (other.column() == CPU_COLUMN || other.column() == MEM_COLUMN) {
                return l.toFloat() < r.toFloat();
            } else if (other.column() == S_COLUMN) {
                return l.toInt() < r.toInt();
            }
        }
        return QStandardItem::operator<(other);
    }
};
#endif // MAINWINDOW_H
