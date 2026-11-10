/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "SessionFileAccess.h"

#include <QFile>
#include <sys/stat.h>

bool SessionFileAccess::isXAuthoritySafe(const QString& path, uint sessionUid, bool isGreeter)
{
	struct stat info{};
	return !path.isEmpty() && ::stat(QFile::encodeName(path).constData(), &info) == 0 &&
		S_ISREG(info.st_mode) && !(info.st_mode & (S_IWGRP | S_IWOTH)) &&
		(info.st_uid == sessionUid || (isGreeter && info.st_uid == 0));
}
