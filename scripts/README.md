# Veyon LaunchAgents Management Script

## Overview

The `launchAgents.sh` script allows you to install, uninstall, and manage the Veyon VNC LaunchAgent service on macOS. This service enables automatic startup of the Veyon VNC server when a user logs in.

## Usage

### Running the Script

The script provides an interactive menu system. To launch it:

```bash
cd /Applications/Veyon/Scripts/
./launchAgents.sh
```

**Important:** Do NOT run this script with `sudo`. The script will request administrator privileges when needed for specific operations.

### Interactive Menu Options

Once launched, you'll see a menu with the following options:

#### 1. Install globally (admin)

Installs the Veyon VNC LaunchAgent globally for all users using `/Library/LaunchAgents/`.

- Requires administrator privileges (will prompt for password)
- The agent will be available system-wide
- Uses the installer from Veyon Configurator's Resources

#### 2. Install current user

Installs the LaunchAgent for the current user only in `~/Library/LaunchAgents/`.

- Does not require administrator privileges
- Only affects the currently logged-in user
- Copies the plist and loads the agent immediately

#### 3. Check plist file

Shows the current state of the LaunchAgent plist files:

- Checks if global plist exists in `/Library/LaunchAgents/`
- Checks if user plist exists in `~/Library/LaunchAgents/`
- Displays file details if found

#### 4. Check runtime status

Displays the current runtime status of the Veyon VNC service:

- Shows if the service is loaded in launchctl
- Displays detailed service information
- Works for the current user or console user (if run as root)

#### 5. Uninstall

Removes the LaunchAgent:

- If you're an administrator: removes both global and user plists
- If you're not an administrator: removes only user plist
- Unloads the service before removing files

#### 0. Exit

Exits the script.

### Important Notes

1. **Do NOT use sudo**: Run the script as a normal user; it will request elevation when needed
2. **Veyon installation**: The script assumes Veyon is installed in `/Applications/Veyon/`
3. **Automatic startup**: Once installed, the VNC server will start automatically when the user logs in
4. **Source files**: The script uses plist and installer from `/Applications/Veyon/veyon-configurator.app/Contents/Resources/Scripts/`

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

## Example Workflows

### Setup for a classroom (Administrator)

1. Navigate to the Scripts folder:
   ```bash
   cd /Applications/Veyon/Scripts/
   ```

2. Run the script:
   ```bash
   ./launchAgents.sh
   ```

3. Select option `1` (Install globally) - this will prompt for your administrator password

4. For individual user setup, log in as each user and:
   - Run `./launchAgents.sh`
   - Select option `2` (Install current user)

### Check if the Agent is Running

1. Run the script:
   ```bash
   ./launchAgents.sh
   ```

2. Select option `4` (Check runtime status)

### Uninstall the Agent

1. Run the script:
   ```bash
   ./launchAgents.sh
   ```

2. Select option `5` (Uninstall)

## Security Considerations

- The LaunchAgent runs with the user's permissions (not as root)
- The agent starts automatically at user login
- Ensure proper Veyon configuration and permissions are set before deploying
- Review macOS Privacy & Security settings for Screen Recording permissions

## Support

For more information about Veyon, visit: https://veyon.io/
