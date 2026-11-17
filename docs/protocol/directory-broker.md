# Directory and user-groups broker

Since 4.12 directory and user-groups backends that need secrets (e.g. the LDAP bind password in
the SecretStore) only run in `veyon-service`. All other components get directory data and group
memberships from the service through the broker. Backends without secrets (builtin directory,
local system user groups, LDAP with anonymous bind, i.e. `UseBindCredentials` off) are not
affected. Specification: `docs/specs/04-directory-broker.md`.

## Where a backend runs

| Component | Backend with `requiresSecrets()` |
|---|---|
| `veyon-service` | in-process (the broker itself) |
| `veyon-server`, `veyon-worker` | always through the broker (IPC to the local service), in every mode |
| elevated `veyon-configurator`/`veyon-cli` (SecretStore available) | in-process |
| master, unprivileged configurator/CLI | through the broker (Veyon connection to the session server on `DirectoryBroker/Host`) |

`DirectoryBroker/Mode`:

- `Auto` (default) and `Required`: as in the table, **`Auto` behaves exactly like `Required`**: no
  unprivileged process can read the SecretStore, so the `Auto` fallback to an in-process backend
  never applies; while the broker is unreachable both modes show a persistent error entry in the
  master's computer tree.
- `Disabled`: legacy in-process behaviour in master/configurator/CLI; works only where the secret
  is readable. Session servers and workers use the broker regardless.
- Invalid values are read as `Required`.

`DirectoryBroker/Host` (default `localhost`): the master connects to the session server of its own
computer (`127.0.0.1`, port `Network/VeyonServerPort` + session offset), so the teacher computer
runs `veyon-service` and access control must allow the master user to access the local computer.
A central broker host is deferred until after the 4.12 TLS work: it requires a TLS connection with
verified host identity and `KeyFileBound` authentication between services. The configurator and
`veyon-cli config set/import` refuse to save a host other than `localhost`, `127.0.0.1` or `::1`;
a configuration distributed otherwise is refused by masters ("directory host identity not
verified") and services answer all broker requests with `unavailable`.

## Transport

- Master → session server: broker feature message (`f34c9499-dcbf-48b2-a067-ca23c65618c8`,
  `Meta | Builtin`), arguments `RequestId` (int64 holding a uint32), `Command` (string) and
  `Arguments` (JSON object, encoded). Old components ignore the unknown feature.
- Session server → service: IPC command `cmd` = command name, `args` = the same JSON object
  (02-ipc.md §6); the IPC request ID takes the role of the `RequestId`. Requests forwarded for
  masters carry `Forwarded: true` (set by the session server, overwriting a master's value).
- Logs: broker feature messages are logged without their arguments (user names, group
  memberships).

The session server forwards a master's request only if the connection passed access control
(`AccessControlState::Successful`) and authenticated with `KeyFile` or `Logon`; `Token`
connections, which skip access control, are answered with `denied`. With key-file authentication
the user name is asserted by the client, so the broker authorization equals access control and is
advisory for key holders (accepted risk R-21); every connection that passes access control can
enumerate rooms, computers and group memberships (R-11).

## Commands

| Command | Arguments | Reply |
|---|---|---|
| `Directory.Update` | `Cursor`? | `Objects`, `Generation`, `NextCursor` |
| `Directory.FetchObjects` | `ObjectUid`, `Cursor`? | `Objects`, `NextCursor` |
| `Directory.QueryObjects` | `Type`, `Attribute`, `Value`?, `Cursor`? | `Objects`, `NextCursor` |
| `Directory.QueryParents` | `ObjectUid`, `Cursor`? | `Objects`, `NextCursor` |
| `Directory.QueryObjectAttribute` | `ObjectUid`, `Attribute` (`Name`, `HostAddress`, `MacAddress`, `DirectoryAddress`) | `Value` |
| `UserGroups.List` | `QueryDomainGroups`?, `Cursor`? | `Groups`, `NextCursor` |
| `UserGroups.GroupsOfUser` | `Username`, `QueryDomainGroups`?, `Cursor`? | `Groups`, `NextCursor` |
| `UserGroups.GroupSecurityIdentifier` | `GroupName` | `Sid` |
| `Error` (reply) | – | `Code` (`unavailable`, `denied`, `too-large`, `bad-request`, `timeout`, `rate-limited`), `Message` |

- `Objects` are `NetworkObject::toJson()` objects, `Type`/`Attribute` arguments are enumerator
  names (numbers are rejected), UIDs are strings. Unknown commands, arguments, enumerator names,
  strings over 256 UTF-8 bytes or with control characters are rejected with `bad-request`.
- `ObjectUid` can only address objects the service returned before to the same session server
  (or the root object), separately for its own and for forwarded requests (least recently used
  first, 1000 own and 20 000 forwarded objects); unknown objects are answered with `bad-request`.
- Replies to forwarded requests contain no `DirectoryAddress` (DNs) and `QueryObjectAttribute`
  refuses it for them.
- `Value` of `QueryObjects` is used literally: the service never resolves host names (the session
  server sends the FQDN; IP addresses do not match).
- Every list reply is paged (also `Update`, `QueryParents` and `GroupsOfUser`, as one IPC record is
  limited to 64 KiB): at most 500 objects or 2000 group names and about 60 KiB per page; the client
  repeats the request with `Cursor` = `NextCursor` until `NextCursor` is empty. Cursors are bound
  to the connection and the command and expire after 60 s.
- Failures of the backend are reported as `unavailable`, never as an empty result. For LDAP a query
  fails unless every search completed with `LDAP_SUCCESS` (also size or time limit exceeded, busy,
  insufficient access, no such object, lost connection after one retry). `LDAP/QueryTimeout` is the
  maximum time without any response from the server, a whole search (all pages) is limited to
  30 s; timed out searches are abandoned and the next query uses a new connection. TCP keep-alive
  detects silently dropped connections.
- Referrals and search references are never chased (AD returns them for subtree searches at the
  domain root; chasing would connect anonymously to hosts named by the server).
- Searches use simple paged results (RFC 2696, 500 entries per page, non-critical), so AD's
  `MaxPageSize` (1000 by default) does not limit results. A server without support for paged
  results must allow results of the needed size; otherwise the search fails with
  sizeLimitExceeded (logged as such).
- **Maximum directory size**: one result holds at most 20 000 entries (also the LDAP size limit
  set by Veyon), larger listings fail with `too-large`. Directories with more computers have to be
  split into locations.
- A failed or aborted directory update is answered with `unavailable`/`timeout` and not reused for
  coalesced requests, so masters never get an incomplete tree as the current state.

## Limits and execution

| Limit | Session server's own requests | Forwarded requests (all masters of a session) | Per master connection |
|---|---|---|---|
| requests per second | 20 | 15 | 10 |
| outstanding requests | 4 | 3 | 2 |
| cursors | 16 | 48 | the last 4 it received |
| cursor memory | 1 MiB | 3 MiB | – |

Each request has 10 s, forwarded requests may run for at most 4 s (they delay queued own
requests); reconnecting to the LDAP server is not started for a cancelled request. A full cursor
pool drops its oldest cursor. The service executes the
requests in one directory worker thread: the session servers' own requests (access control)
round-robin before all forwarded requests, then the forwarded ones round-robin. Requests are
cancelled at their deadline, also while running (the LDAP client checks between and during
searches). Concurrent `Update` requests are answered by one update. A backend that could not be
created is tried again after 30 s.

## Access control

- New connections fail closed: if a group membership or location query needed by the configured
  access control method fails, access is denied ("Directory for access control not available").
  Rule processing stops at the first failed query. After a failure, queries fail immediately for
  3 s (still denying), so that an outage does not cost 10 s per query.
- Established, already authorised connections: if a re-check (when another authorised client
  leaves) fails because the directory is unavailable, the previous decision is kept, the
  connection is checked again every 15 s and closed after 120 s without a successful check. A
  connection closed by a re-check or the grace period triggers a further re-check of the others.
  Re-checks run in the session server's main thread and block it while waiting for the broker
  (bounded by the request timeout and the 3 s circuit breaker).
- Within one check or one re-check pass of all connections every directory query is made once.
  Group memberships are never cached beyond that; only the user group listing of the
  configuration pages is cached for 60 s.
- Whether the session server listens on localhost only (deny rule for the local state) is not
  restricted when the directory is unavailable at its start.

## Plugin interfaces (WP15, out-of-tree plugins)

`NetworkObjectDirectoryPluginInterface` and `UserGroupsBackendInterface` have new IIDs
(`…/2`) in 4.12 because of new virtual functions (`requiresSecrets()`, `tryQueryGroupsOfUser()`,
`NetworkObjectDirectory::tryQueryObjects()/tryQueryParents()`): plugins built for 4.11 (e.g. the
Entra ID connector) are not loaded as directory/user groups backends until rebuilt. Backends
reading the SecretStore return `true` from `requiresSecrets()` and should implement the `try…()`
functions so that failures are not taken as empty results.
