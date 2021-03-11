#pragma once

#ifdef _WIN32_WINNT
#include <winsock2.h>
#endif

#ifdef __cplusplus
#ifdef QT_WIDGETS_LIB
#include <QApplication>
#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>
#endif
#include <QtConcurrent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>
#ifdef QT_NETWORK_LIB
#include <QTcpSocket>
#endif
#endif
