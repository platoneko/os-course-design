#ifndef MEMDIALOG_H
#define MEMDIALOG_H

#include <QDialog>


namespace Ui {
class MemDialog;
}

class MemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MemDialog(QWidget *parent = nullptr);
    ~MemDialog();
    void update(unsigned long mem_total,
                unsigned long mem_free,
                unsigned long mem_used,
                unsigned long mem_buff,
                unsigned long mem_cache,
                unsigned long mem_available,
                unsigned long swap_total,
                unsigned long swap_free,
                unsigned long swap_used);

private:
    Ui::MemDialog *ui;
    void closeEvent(QCloseEvent *event);

signals:
    void closeSignal();
};

#endif // MEMDIALOG_H
