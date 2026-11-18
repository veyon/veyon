// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include <QtPlugin>

class QuickConfigurationPluginInterface
{
public:
	virtual ~QuickConfigurationPluginInterface() = default;
	virtual QObject* createQuickConfiguration(Configuration::Object* draft, QObject* parent) = 0;
};
#define QuickConfigurationPluginInterface_iid "io.veyon.Veyon.Plugins.QuickConfigurationPluginInterface"
Q_DECLARE_INTERFACE(QuickConfigurationPluginInterface, QuickConfigurationPluginInterface_iid)

namespace QuickConfiguration
{
using Property = Configuration::Property;
inline Property::UiMetadata metadata(const QString& page, const QString& group, const QString& label,
	Property::Editor editor = Property::Editor::Automatic,
	const QVariant& minimum = {}, const QVariant& maximum = {}, const QString& suffix = {})
{
	Property::UiMetadata ui;
	ui.pageId = page; ui.groupId = group; ui.label = label; ui.editor = editor;
	ui.minimum = minimum; ui.maximum = maximum; ui.suffix = suffix;
	return ui;
}
inline void describe(Property& property, const QString& page, const QString& group, const QString& label,
	Property::Editor editor = Property::Editor::Automatic,
	const QVariant& minimum = {}, const QVariant& maximum = {}, const QString& suffix = {})
{
	property.setUiMetadata(metadata(page, group, label, editor, minimum, maximum, suffix));
}
inline void choices(Property& property, const QList<Property::Choice>& choices)
{
	auto ui = property.uiMetadata(); ui.editor = Property::Editor::Choice; ui.choices = choices;
	property.setUiMetadata(ui);
}
inline void depends(Property& property, const QStringList& keys, std::function<bool()> enabled)
{
	auto ui = property.uiMetadata(); ui.dependencyKeys = keys; ui.enabled = std::move(enabled);
	property.setUiMetadata(ui);
}
inline void restart(Property& property)
{
	auto ui = property.uiMetadata(); ui.effects |= Property::ApplyEffect::ServiceRestart;
	property.setUiMetadata(ui);
}
}
