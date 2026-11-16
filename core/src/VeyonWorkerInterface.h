// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class BuiltinFeatures;
class FeatureMessage;

class VeyonWorkerInterface
{
public:
	virtual ~VeyonWorkerInterface() = default;

	virtual bool sendFeatureMessageReply(const FeatureMessage& reply) = 0;
};
