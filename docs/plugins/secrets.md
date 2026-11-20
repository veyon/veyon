# Secrets in plugins (`SecretStore`)

Veyon 4.12 keeps secrets (directory credentials, private keys, tokens) in the `SecretStore`
(`core/src/SecretStore.h`) instead of the configuration. The configuration only holds a reference
`secret:<scope>/<name>` (property type `Configuration::SecretReference`).

- Storage: Linux `/etc/veyon/secrets.d/<scope>__<name>` (directory `0700`, files `0600`, owner
  root); Windows `%ProgramData%\Veyon\secrets` (protected DACL: SYSTEM and Administrators only,
  content additionally wrapped with DPAPI in the machine scope). Exclude `/etc/veyon/secrets.d` from
  `etckeeper` and configuration backups or encrypt those backups.
- Availability: `SecretStore::isAvailable()` is true only in processes whose `main()` passes the
  `VeyonCore::InitializationFlag::SecretStore` flag (`veyon-service`, `veyon-configurator`,
  `veyon-cli`) and that run privileged (root, SYSTEM or elevated administrator). It is always false in
  `veyon-server`, `veyon-worker`, the master and helper processes, regardless of their user; these
  get secrets only via the service's local IPC or use the directory broker. Never fall back to
  another storage location when the store is not available.
- Scopes are naming hygiene, not access control: every plugin loaded into a privileged process can
  read every scope. Plugins use their lower-case plugin name as scope.
- Declare every secret property in `PluginInterface::secretProperties()` of the plugin, returning
  `{parentKey, key, secretKey, isLegacyPassword}` rows (`core/src/Configuration/SecretProperty.h`),
  e.g. `{QStringLiteral("MyPlugin"), QStringLiteral("ClientSecret"), QStringLiteral("myplugin/clientSecret"), false}`.
  No core change is needed. Right after loading a plugin, `PluginManager` merges its rows into the
  `Configuration::SecretPropertyRegistry` (`core/src/Configuration/SecretPropertyRegistry.h`), which
  also holds the rows of core; `veyon-cli config set/unset/clear`, `config export/import
  --include-secrets`, the configurator, the scrubbing and the legacy migration at privileged start
  use only this registry. A row is rejected with a critical log message if its secret key is
  invalid or uses a reserved scope (`core`, `ipc`, `pki`, `tls`). Rows declaring the same property
  or the same secret key (compared case-insensitively) conflict unless they are identical and come
  from the same plugin UID (e.g. a second `VeyonCore` in a process). Conflicting rows are **all**
  rejected regardless of the load order and both plugins are logged with name and UID; the
  properties are then never resolved (fail closed). Rows of core always win. Former `Configuration::Password`
  properties (e.g. the client secret of the Entra ID connector) are marked `isLegacyPassword` and
  are migrated by `LegacySecretMigration`.
- The registry is complete only after all plugins have been loaded: never resolve a secret
  property (`value()`, `SecretStore::read()` of its key) in the plugin's constructor, read it
  lazily when it is needed. The registry is filled in the main thread while `VeyonCore` is
  initialized and is read-only afterwards. Rows of objects registered with
  `PluginManager::registerExtraPluginInterface()` are added as well, but after the legacy migration
  of that start.
- A secret property without a valid row is disabled in the configurator (with a tooltip) and its
  critical log message is written once per process.
- Values of secret properties of plugins which are not installed (or failed to load) are not in
  the registry: they are never resolved and left untouched, including legacy encrypted values,
  which stay in the configuration until the plugin is installed and the next privileged start
  migrates them. `veyon-cli config set` refuses to change such a `secret:` value (key compared
  case-insensitively), `config unset` refuses to remove it (load the plugin to remove the value
  together with its secret, or use `config clear`), `config export --include-secrets` warns that
  its secret is not exported, and `config import --include-secrets` refuses secrets no loaded
  plugin declares.
- `veyon-cli config clear` and the reset in the configurator remove **every** stored secret outside
  the reserved scopes `core`, `ipc`, `pki` and `tls`, also those of plugins which are not loaded and
  secrets a plugin stores in its own scope without a property, and print (CLI) or log the removed
  keys. Host keys, CA keys and `core/preMigrationConfig` are kept.
- Declare the property as a `Configuration::SecretReference` property whose default value is the
  same fixed key, e.g.
  `OP(MyConfiguration, m_configuration, Configuration::SecretReference, clientSecret, setClientSecret, "ClientSecret", "MyPlugin", Configuration::SecretReference::defaultValue(QStringLiteral("myplugin/clientSecret")), Configuration::Property::Flag::Standard)`.
  The configurator, `veyon-cli config set` and `config export/import --include-secrets` then
  handle it; read it with `SecretStore::read(m_configuration.clientSecret().key())`. A stored value
  never redirects a property to another key, and a property whose default value doesn't match its
  registered row resolves to an invalid reference.
- Never log secret values, put them into command lines, environment variables or files outside the
  store; keep them in `Veyon::Crypto::SecureBuffer`.

## Windows: directories created by others

A standard user can create directories below `%ProgramData%` before Veyon does. The first
privileged write to the store (e.g. the migration at the first service start) therefore checks
both directories by handle, without following junctions or symlinks:

- `%ProgramData%\Veyon` is never moved (it also holds `keys\` and configuration templates). A
  junction or symlink in its place is moved aside (`Veyon.untrusted-<random>`); a directory with
  a foreign owner is taken over (owner Administrators, protected DACL: SYSTEM and Administrators
  full control, users read and execute).
- `%ProgramData%\Veyon\secrets` with a foreign owner or DACL, or a junction in its place, is moved
  aside and created again (SYSTEM and Administrators only) if it is empty; if moving fails, an
  empty directory is taken over by handle. If it already contains files, Veyon refuses to use it
  and logs a critical message, which is also written to the Windows event log (source of the
  Veyon component, e.g. the Veyon Service) regardless of `Logging/LogToSystem`, instead of
  silently starting with an empty store. The message names the directory, never secrets: check
  the content and move or delete the directory manually, then start the Veyon Service again.
- The installer doesn't change these directories (`takeown`/`icacls` follow junctions).

## Migration from 4.11 and downgrades (03-secretstore-config.md §4.4, §4.8)

- At every start of `veyon-service` and of an elevated CLI/configurator, legacy encrypted values
  of the former password properties are moved into the store.
- A legacy value found at startup never replaces or deletes a secret that is already stored
  (which may have been rotated), regardless of `Core/SecretMigrationVersion`; it is removed from
  the configuration with a warning. Only explicit imports of a file with legacy values
  (`veyon-cli config import`, loading settings in the configurator) replace a stored secret, with
  a warning naming the key (CLI: on the console, configurator: a message box). Remove legacy
  values from policies and rotate the LDAP bind password.
- `veyon-cli config import --include-secrets` also warns with the keys of stored secrets it
  replaces by the secrets of the file.
- After `veyon-cli config import` without `--include-secrets` (with administrator privileges) the
  CLI warns about every secret property whose reference points to a secret that is not stored,
  e.g. after importing a file on another computer: import the secrets with `--include-secrets` or
  set them with `veyon-cli config set`. Without access to the store nothing is checked.
- `veyon-cli config clear` keeps `core/preMigrationConfig` (the only way back to 4.11) and says
  so; it is no configuration property and has to be deleted from the secret storage manually.
- Before the first migration the complete configuration (legacy values included) is saved as
  secret `core/preMigrationConfig`. Legacy values migrated later (e.g. of a plugin installed
  after the first migration) are added to this backup; the rest of it is kept as it was. If the
  backup can't be read or written, nothing is migrated. `veyon-cli config export-premigration <file>` (administrator
  privileges) writes it to a private file (mode `0600`, on Windows SYSTEM and Administrators only)
  for a downgrade with `veyon-cli config import`; delete the file afterwards, its passwords are
  readable with the public default key.
- `veyon-cli config export --include-secrets` embeds the passphrase-protected secrets as member
  `VeyonSecrets` of the export; Veyon never stores it in a configuration and never writes it into a
  plain export. Only the secrets are protected by the passphrase, not the rest of the file.

## Residual risks

- Copies of secrets outside `SecureBuffer` are not wiped: the text of the configurator's password
  fields (`QLineEdit`), a value passed to `veyon-cli config set` on the command line (`QString`,
  also visible in the process list), the password of the external VNC server while the plugin
  hands it out (`QByteArray::fromRawData` into a QCA `SecureArray`), JSON copies during exports and
  imports, the `QString` passed to the LDAP library.
- The `VEYON_SECRET_DIR` override of the store location exists only in builds with `WITH_TESTS`;
  packages must never be built with `WITH_TESTS`.
- Windows: a local administrator or SYSTEM process can read every secret; DPAPI in the machine
  scope only protects against offline copies of single files, the DACL is the real control.

## Registered scopes and keys

| Scope | Keys | Owner |
|---|---|---|
| `core` | `preMigrationConfig` (backup before the 4.12 migration), `brokerClientKey` | core |
| `ipc` | – (reserved) | core |
| `tls` | `hostKey`, `hostKey.next`, `enrollmentToken` | core (PKI/TLS) |
| `pki` | `caKey`, `tokenSigningKey` | core (CA host) |
| `ldap` | `bindPassword` | LDAP plugin |
| `vncserver-external` | `password` | external VNC server plugin |
| `entra` | `clientSecret`, `clientCertificateKey` | Entra ID connector |
