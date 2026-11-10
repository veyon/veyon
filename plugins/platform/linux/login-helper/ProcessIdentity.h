/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QByteArray>
#include <QString>

// A PID alone is not an identity: retain the kernel start time and executable
// inode, and recheck them before delivering credentials to a registered agent.
struct ProcessIdentity
{
	uint pid = 0;
	uint parentPid = 0;
	uint uid = uint(-1);
	quint64 startTime = 0;
	quint64 device = 0;
	quint64 inode = 0;
	QString executable;
	bool trustedExecutable = false;

	bool isValid() const;
	bool sameProcess(const ProcessIdentity& other) const;
	bool isLightDmWrapper(const ProcessIdentity& parent, const ProcessIdentity& helper,
		uint leader, uint greeterUid) const;
	bool isLightDmSessionChildOf(const ProcessIdentity& daemon) const;
	static ProcessIdentity read(uint pid);
	static bool parseStat(const QByteArray& data, uint& pid, uint& parentPid, quint64& startTime);
};
