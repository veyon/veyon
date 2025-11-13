# Veyon LaunchAgents Management Script

## Overview

The `launchAgents.sh` script allows you to install, uninstall, and manage the Veyon VNC LaunchAgent service on macOS. This service enables automatic startup of the Veyon VNC server when a user logs in.

## Usage

### Basic Syntax

```bash
sudo ./launchAgents.sh [command] [username]
```

### Commands

#### Install LaunchAgent for a specific user

```bash
sudo ./launchAgents.sh install <username>
```

This will:
- Create the LaunchAgent plist file for the specified user
- Install it in `/Users/<username>/Library/LaunchAgents/`
- Set proper permissions
- Load the agent immediately

**Example:**
```bash
sudo ./launchAgents.sh install student
```

#### Install LaunchAgent for all users

```bash
sudo ./launchAgents.sh install-all
```

This will install the LaunchAgent for every user on the system (except system accounts like root, daemon, etc.).

#### Uninstall LaunchAgent from a specific user

```bash
sudo ./launchAgents.sh uninstall <username>
```

This will:
- Unload the LaunchAgent
- Remove the plist file from the user's LaunchAgents directory

**Example:**
```bash
sudo ./launchAgents.sh uninstall student
```

#### Uninstall LaunchAgent from all users

```bash
sudo ./launchAgents.sh uninstall-all
```

This will remove the LaunchAgent from all users who have it installed.

#### Check status

```bash
./launchAgents.sh status
```

This will show:
- Which users have the LaunchAgent installed
- Whether the agent is loaded/running for each user

**Note:** Status command does not require sudo.

### Important Notes

1. **Administrator privileges required**: All install/uninstall commands require `sudo`
2. **Veyon installation**: The script assumes Veyon is installed in `/Applications/Veyon/`
3. **Automatic startup**: Once installed, the VNC server will start automatically when the user logs in
4. **Current session**: The script automatically loads the agent for the current user session

## Prerequisites

- Veyon must be installed in `/Applications/Veyon/`
- The script must be run with administrator privileges (sudo) for install/uninstall operations
- Users must exist on the system before installing their LaunchAgent

## Troubleshooting

### Check if the agent is loaded

```bash
launchctl list | grep com.veyon.vnc
```

### View agent logs

Check the system logs for any errors:
```bash
log show --predicate 'process == "veyon-server"' --last 5m
```

### Manually unload the agent

If needed, you can manually unload the agent:
```bash
launchctl unload ~/Library/LaunchAgents/com.veyon.vnc.plist
```

### Manually load the agent

```bash
launchctl load ~/Library/LaunchAgents/com.veyon.vnc.plist
```

## File Locations

- **LaunchAgent plist**: `~/Library/LaunchAgents/com.veyon.vnc.plist`
- **Veyon server**: `/Applications/Veyon/veyon-server.app/Contents/MacOS/veyon-server`
- **Script location**: `/Applications/Veyon/Scripts/launchAgents.sh`

## Examples

### Setup for a classroom with multiple student accounts

```bash
# Install for all users at once
sudo ./launchAgents.sh install-all

# Or install individually
sudo ./launchAgents.sh install student1
sudo ./launchAgents.sh install student2
sudo ./launchAgents.sh install student3
```

### Check current status

```bash
./launchAgents.sh status
```

### Remove from all users

```bash
sudo ./launchAgents.sh uninstall-all
```

## Security Considerations

- The LaunchAgent runs with the user's permissions (not as root)
- The agent starts automatically at user login
- Ensure proper Veyon configuration and permissions are set before deploying
- Review macOS Privacy & Security settings for Screen Recording permissions

## Support

For more information about Veyon, visit: https://veyon.io/
