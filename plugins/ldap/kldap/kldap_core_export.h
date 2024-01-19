// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#define KLDAP_CORE_EXPORT __attribute__((visibility("default")))
#define KLDAP_CORE_NO_EXPORT __attribute__((visibility("hidden")))
