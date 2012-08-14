/*
 * Copyright (c) 2012 by Maximilian Güntner <maximilian.guentner@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_tftp(new QTftp),
    m_file(NULL),
    m_connected(false)
{
    QCoreApplication::setOrganizationName("Ethersex");
    QCoreApplication::setOrganizationDomain("www.ethersex.de");
    QCoreApplication::setApplicationName("EthersexFlash");
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/bunnies.png"));
    setupSignalsAndSlots();
    QTimer::singleShot(0, this, SLOT(restoreSettings()));
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
    if (m_file == NULL)
        return;
    if (m_file->isOpen()) {
        m_file->close();
        delete m_file;
    }
}

void MainWindow::setupSignalsAndSlots()
{
    connect(this, SIGNAL(imageFilenameChanged(QString)), this, SLOT(processFilenameChange(QString)));
    connect(ui->imageLine, SIGNAL(textChanged(QString)), this, SLOT(processFilenameChange(QString)));
    connect(m_tftp, SIGNAL(stateChanged(QTftp::State)), this, SLOT(tftpState(QTftp::State)));
    connect(m_tftp, SIGNAL(dataTransferProgress(qint64,qint64)), this, SLOT(updateDataTransferProgress(qint64,qint64)));
    connect(m_tftp, SIGNAL(done(bool)), this, SLOT(tftpDone(bool)));
    connect(m_tftp, SIGNAL(error(QTftp::ErrorCode,QString)), this, SLOT(tftpHandleError(QTftp::ErrorCode,QString)));
}

void MainWindow::on_imageBrowseButton_clicked()
{
    QDir directory = QFileInfo(m_filename).dir();
    if (directory.exists())
        emit imageFilenameChanged(QFileDialog::getOpenFileName(this,tr("Open Image"), directory.absolutePath()));
    else
        emit imageFilenameChanged(QFileDialog::getOpenFileName(this,tr("Open Image")));
}
void MainWindow::processFilenameChange(QString filename)
{
    QPalette imageLinePalette = QApplication::palette();
    QFileInfo imageFileInfo(filename);
    if (!imageFileInfo.exists() || imageFileInfo.isDir() || !imageFileInfo.isReadable()) {
        imageLinePalette.setColor(QPalette::WindowText,Qt::red);
        imageLinePalette.setColor(QPalette::Text,Qt::red);
    }
    ui->imageLine->setText(filename);
    ui->imageLine->setPalette(imageLinePalette);
    m_filename = filename;
}

void MainWindow::on_flashButton_clicked()
{
    if (m_connected)
        upload();
    else
        m_tftp->connectToHost(ui->targetLine->currentText(), 69);
}

void MainWindow::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
    ui->progressBar->setMaximum(totalBytes);
    ui->progressBar->setValue(readBytes);
}
void MainWindow::upload()
{
    QFileInfo fi(m_filename);
    if (m_filename.size() == 0) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a firmware file first."));
        return;
    }
    m_file = new QFile(m_filename);
    if (!m_file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open ") + m_filename + " ");
        delete m_file;
        return;
    } else {
        QMessageBox::information(this, tr("Prepare your device now."), tr("Please reset your Ethersex device to the bootloader and press OK"));
        m_tftp->put(m_file, fi.fileName());
    }
}
void MainWindow::tftpState(QTftp::State state)
{
    if (state == QTftp::Connected) {
        ui->statusBar->showMessage(tr("Connected"));
        if (m_connected == false) {
            upload();
            m_connected = true;
        }
    }
    if (state == QTftp::Transfering)
        ui->statusBar->showMessage(tr("Transfering"));
    if (state == QTftp::HostLookup)
        ui->statusBar->showMessage(tr("Looking up host"));
    if (state == QTftp::Idle) {
        ui->statusBar->showMessage(tr("Idle"));
        m_connected = false;
    }
    if (state == QTftp::Unconnected) {
        ui->statusBar->showMessage(tr("Unconnected"));
        m_connected = false;
    }
}

void MainWindow::tftpDone(bool error)
{
    if (!error)
        QMessageBox::information(this, tr("Upload sucessful"), tr("You should now be able to access your ethersex device."));
    if (m_file->isOpen() && m_file != NULL)
        m_file->close();
}

void MainWindow::tftpHandleError(QTftp::ErrorCode, const QString &message)
{
    QMessageBox::warning(this, tr("Error"), message);
}
void MainWindow::on_targetLine_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    m_tftp->disconnectFromHost();
}

void MainWindow::restoreSettings()
{
    /* Restore history */
    QSettings settings;
    QStringList devices = settings.value("devices").toStringList();
    ui->targetLine->addItems(devices);
    if (ui->targetLine->count() == 0)
        ui->targetLine->addItem("192.168.0.90");
    QString lastImage = settings.value("lastImage").toString();
    if (lastImage != "")
        ui->imageLine->setText(lastImage);
    /* Restore done */
}

void MainWindow::saveSettings()
{
    /* Save device */
    QSettings settings;
    QStringList devices;
    for ( qint16 i=ui->targetLine->count()-1; i >= 0 ; i--) {
        devices.append(ui->targetLine->itemText(i));
    }
    settings.setValue("lastImage", ui->imageLine->text());
    settings.setValue("devices", devices);
}
