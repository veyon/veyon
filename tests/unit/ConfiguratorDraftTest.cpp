// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include "ConfiguratorDraft.h"
#include "Configuration/PropertyModel.h"

class ConfiguratorDraftTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void backendChangesIncludeRemovedKeysAndIgnoreExplicitDefaults()
	{
		ConfiguratorDraft draft;
		draft.load({{QStringLiteral("LDAP/ServerHost"), QStringLiteral("old.example")},
			{QStringLiteral("UserGroups/Backend"), QUuid(QStringLiteral("6f0a491e-c1c6-4338-8244-f823b0bf8670"))}});
		draft.factoryReset();
		QVERIFY(draft.hasChangesInGroups({QStringLiteral("LDAP")}));
		QVERIFY(draft.hasChangesInGroups({QStringLiteral("UserGroups")}));
		QVERIFY(!draft.hasChangesInGroups({QStringLiteral("NetworkObjectDirectory")}));
		draft.revert();
		QVERIFY(!draft.hasChangesInGroups({QStringLiteral("LDAP"), QStringLiteral("UserGroups")}));
		draft.load({});
		draft.configuration().replaceData({{QStringLiteral("UserGroups/Backend"), draft.configuration().userGroupsBackendProperty().defaultValue()}});
		QVERIFY(!draft.hasChangesInGroups({QStringLiteral("UserGroups")}));
		draft.saved(false);
		draft.factoryReset();
		QVERIFY(!draft.hasChangesInGroups({QStringLiteral("UserGroups")}));
	}
	void restartDetectionReadsGroupedGlobalProperty()
	{
		VeyonConfiguration saved(nullptr);
		saved.setVeyonServerPort(11100);
		ConfiguratorDraft draft; draft.load(saved.data());
		QVERIFY(!draft.requiresServiceRestart());
		draft.configuration().setVeyonServerPort(11200);
		QVERIFY2(draft.requiresServiceRestart(), "A changed port in the nested Network map requires a service restart");
		draft.revert();
		QVERIFY(!draft.requiresServiceRestart());
	}
	void restartDetectionReadsNestedPluginProperty()
	{
		ConfiguratorDraft draft;
		Configuration::TypedProperty<bool> pluginOption(&draft.configuration(), QStringLiteral("Enabled"),
			QStringLiteral("ExamplePlugin/Nested"), false, Configuration::Property::Flag::Standard);
		Configuration::Property::UiMetadata metadata;
		metadata.effects = Configuration::Property::ApplyEffect::ServiceRestart;
		pluginOption.setUiMetadata(metadata);
		pluginOption.setValue(false);
		draft.load(draft.configuration().data());
		QVERIFY(!draft.requiresServiceRestart());
		pluginOption.setValue(true);
		QVERIFY2(draft.requiresServiceRestart(), "A plugin property in a nested group participates in restart detection");
	}
	void removingNondefaultGroupedSettingRequiresRestart()
	{
		VeyonConfiguration saved(nullptr);
		saved.setVeyonServerPort(11200);
		ConfiguratorDraft draft; draft.load(saved.data());
		draft.factoryReset();
		QCOMPARE(draft.configuration().veyonServerPort(), 11100);
		QVERIFY2(draft.requiresServiceRestart(), "Resetting a saved nondefault server port must request a restart");
	}
	void defaultRepresentationsAndUnrelatedEditsDoNotRequireRestart()
	{
		ConfiguratorDraft draft; draft.load({});
		draft.configuration().setVeyonServerPort(11100);
		QVERIFY(!draft.requiresServiceRestart());
		draft.saved(false);
		draft.applied();
		draft.factoryReset();
		QVERIFY(!draft.requiresServiceRestart());
		draft.load({});
		draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		QVERIFY(!draft.requiresServiceRestart());
	}
	void detectedRestartSurvivesFailedApplicationAndRetry()
	{
		ConfiguratorDraft draft; draft.load({});
		draft.configuration().setVeyonServerPort(11200);
		draft.saved(draft.requiresServiceRestart());
		QVERIFY(draft.applyPending());
		QVERIFY2(draft.restartPending(), "Saving a changed grouped setting retains its restart requirement until application succeeds");
		QVERIFY(!draft.requiresServiceRestart());
		draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		draft.revert();
		QVERIFY(draft.applyPending());
		QVERIFY(draft.restartPending());
		draft.saved(draft.requiresServiceRestart());
		QVERIFY(draft.restartPending());
		draft.applied();
		QVERIFY(!draft.applyPending());
		QVERIFY(!draft.restartPending());
		QVERIFY(!draft.dirty());
	}
	void editorChangesOnlyDraftAndRevertUpdatesModel()
	{
		VeyonConfiguration live(nullptr);
		live.setComputerMonitoringThumbnailSpacing(7);
		ConfiguratorDraft draft;
		draft.load(live.data());
		Configuration::PropertyModel model(&draft.configuration());
		QVERIFY(model.setValue(QStringLiteral("Master/ComputerMonitoringThumbnailSpacing"), 15));
		QVERIFY(draft.dirty());
		QCOMPARE(live.computerMonitoringThumbnailSpacing(), 7);
		draft.revert();
		QCOMPARE(model.value(QStringLiteral("Master/ComputerMonitoringThumbnailSpacing")).toInt(), 7);
		QVERIFY(!draft.dirty());
	}
	void factoryResetIsReversibleAndDoesNotWriteLiveSettings()
	{
		VeyonConfiguration live(nullptr);
		live.setComputerMonitoringThumbnailSpacing(17);
		ConfiguratorDraft draft; draft.load(live.data());
		draft.factoryReset();
		QVERIFY(draft.dirty());
		QCOMPARE(draft.configuration().computerMonitoringThumbnailSpacing(), 5);
		QCOMPARE(live.computerMonitoringThumbnailSpacing(), 17);
		draft.revert();
		QCOMPARE(draft.configuration().computerMonitoringThumbnailSpacing(), 17);
	}
	void savedButNotAppliedRemainsDirtyAcrossRevertAndRetry()
	{
		ConfiguratorDraft draft; draft.load({});
		draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		draft.saved(true);
		QVERIFY(draft.dirty()); QVERIFY(draft.applyPending()); QVERIFY(draft.restartPending());
		draft.configuration().setComputerMonitoringThumbnailSpacing(20);
		draft.revert();
		QCOMPARE(draft.configuration().computerMonitoringThumbnailSpacing(), 14);
		QVERIFY(draft.dirty()); QVERIFY(draft.restartPending());
		draft.saved(false); QVERIFY(draft.restartPending());
		draft.applied();
		QVERIFY(!draft.dirty()); QVERIFY(!draft.applyPending()); QVERIFY(!draft.restartPending());
	}
	void importFailureKeepsDraftAndSavedSnapshot()
	{
		QTemporaryDir directory;
		QFile input(directory.filePath(QStringLiteral("broken.json")));
		QVERIFY(input.open(QIODevice::WriteOnly)); input.write("{invalid"); input.close();
		ConfiguratorDraft draft; draft.load({});
		draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		const auto before = draft.configuration().data();
		QString error;
		QVERIFY(!draft.importConfiguration(input.fileName(), &error));
		QVERIFY(!error.isEmpty()); QCOMPARE(draft.configuration().data(), before); QVERIFY(draft.dirty());
		draft.revert(); QCOMPARE(draft.configuration().computerMonitoringThumbnailSpacing(), 5);
	}
	void exportFailureDoesNotClearDirtyState()
	{
		QTemporaryDir directory;
		ConfiguratorDraft draft; draft.load({}); draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		const auto before = draft.configuration().data(); QString error;
		QVERIFY(!draft.exportConfiguration(directory.path(), &error));
		QVERIFY(!error.isEmpty()); QVERIFY(draft.dirty()); QCOMPARE(draft.configuration().data(), before);
	}
	void encryptedPasswordsAndUnknownKeysSurviveRoundTripWithoutEditorConversion()
	{
		QTemporaryDir directory;
		ConfiguratorDraft draft;
		draft.load({{QStringLiteral("LDAP/BindPassword"), QStringLiteral("encrypted-placeholder-not-plaintext")}, {QStringLiteral("FuturePlugin/Option"), QStringLiteral("keep")}});
		QString error; const auto path = directory.filePath(QStringLiteral("config.json"));
		QVERIFY2(draft.exportConfiguration(path, &error), qPrintable(error));
		draft.configuration().setComputerMonitoringThumbnailSpacing(14);
		QVERIFY2(draft.importConfiguration(path, &error), qPrintable(error));
		QCOMPARE(draft.configuration().data().value(QStringLiteral("LDAP/BindPassword")).toString(), QStringLiteral("encrypted-placeholder-not-plaintext"));
		QCOMPARE(draft.configuration().data().value(QStringLiteral("FuturePlugin/Option")).toString(), QStringLiteral("keep"));
		QCOMPARE(draft.configuration().computerMonitoringThumbnailSpacing(), 14);
		draft.revert();
		QCOMPARE(draft.configuration().data().value(QStringLiteral("LDAP/BindPassword")).toString(), QStringLiteral("encrypted-placeholder-not-plaintext"));
		QVERIFY(!draft.dirty());
	}
};
QTEST_GUILESS_MAIN(ConfiguratorDraftTest)
#include "ConfiguratorDraftTest.moc"
