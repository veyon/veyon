// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QLatin1String>

// Command, argument and error names of the directory and user-groups broker
// (docs/specs/04-directory-broker.md §4). The same names are used in broker feature messages and
// as IPC commands/argument keys (02-ipc.md §6).
namespace DirectoryBrokerProtocol {

namespace Command {
inline constexpr auto DirectoryUpdate = QLatin1String("Directory.Update");
inline constexpr auto DirectoryFetchObjects = QLatin1String("Directory.FetchObjects");
inline constexpr auto DirectoryQueryObjects = QLatin1String("Directory.QueryObjects");
inline constexpr auto DirectoryQueryParents = QLatin1String("Directory.QueryParents");
inline constexpr auto DirectoryQueryObjectAttribute = QLatin1String("Directory.QueryObjectAttribute");
inline constexpr auto UserGroupsList = QLatin1String("UserGroups.List");
inline constexpr auto UserGroupsGroupsOfUser = QLatin1String("UserGroups.GroupsOfUser");
inline constexpr auto UserGroupsGroupSecurityIdentifier = QLatin1String("UserGroups.GroupSecurityIdentifier");
// reply only
inline constexpr auto Error = QLatin1String("Error");
} // namespace Command

namespace Key {
// requests
inline constexpr auto ObjectUid = QLatin1String("ObjectUid");
inline constexpr auto Cursor = QLatin1String("Cursor");
inline constexpr auto Type = QLatin1String("Type");
inline constexpr auto Attribute = QLatin1String("Attribute");
inline constexpr auto Value = QLatin1String("Value");
inline constexpr auto QueryDomainGroups = QLatin1String("QueryDomainGroups");
inline constexpr auto Username = QLatin1String("Username");
inline constexpr auto GroupName = QLatin1String("GroupName");
// set by the session server on requests it forwards for masters (overwriting any value sent by a
// master); the service queues them behind the session server's own requests
inline constexpr auto Forwarded = QLatin1String("Forwarded");
// replies
inline constexpr auto Objects = QLatin1String("Objects");
inline constexpr auto Generation = QLatin1String("Generation");
inline constexpr auto NextCursor = QLatin1String("NextCursor");
inline constexpr auto Groups = QLatin1String("Groups");
inline constexpr auto Sid = QLatin1String("Sid");
inline constexpr auto Code = QLatin1String("Code");
inline constexpr auto Message = QLatin1String("Message");
} // namespace Key

namespace Error {
inline constexpr auto Unavailable = QLatin1String("unavailable");
inline constexpr auto Denied = QLatin1String("denied");
inline constexpr auto TooLarge = QLatin1String("too-large");
inline constexpr auto BadRequest = QLatin1String("bad-request");
inline constexpr auto Timeout = QLatin1String("timeout");
inline constexpr auto RateLimited = QLatin1String("rate-limited");
} // namespace Error

} // namespace DirectoryBrokerProtocol
