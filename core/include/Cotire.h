#ifndef COTIRE_H
#define COTIRE_H

#include <veyonconfig.h>

#ifdef VEYON_BUILD_WIN32
#define INITGUID
#include <winsock2.h>
#include <winsock.h>
#include <netfw.h>
#include <knownfolders.h>
#endif

#include "VeyonCore.h"
#include <QApplication>
#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QTcpSocket>
#endif
