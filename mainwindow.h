#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qtftp.h>
#include <QFile>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    void setupSignalsAndSlots();
    void upload();

private slots:
    void on_imageBrowseButton_clicked();
    void processFilenameChange(QString filename);
    void tftpState(QTftp::State state);
    void tftpDone(bool error);
    void tftpHandleError(QTftp::ErrorCode ,const QString &message);
    void on_flashButton_clicked();
    void updateDataTransferProgress(qint64 readBytes, qint64 totalBytes);
    void on_targetLine_textChanged(const QString &arg1);
    void restoreSettings();
    void saveSettings();

signals:
    void imageFilenameChanged(QString filename);
private:
    Ui::MainWindow *ui;
    QTftp *m_tftp;
    QFile *m_file;
    QString m_filename;
    bool m_connected;
};

#endif // MAINWINDOW_H
