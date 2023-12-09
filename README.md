# Relay
Intermediary program to seamlessly connect clients with multiple servers.
# Compiling
Run `cmake --build . --target all`
The binary should be located at `build/default/MCPIRelay/mcpi-relay`

## Running 
The relay expects a `servers.json` file in the working directory.  
Optionally, `authorized.json` can be used for connecting an auth system.

The dedicated MCPI servers running on localhost should not use port 19132, and should not be available publicly. All connections must pass through the relay for correct functionality.  
For proper chunk loading, add [libno-compress.so](https://cdn.discordapp.com/attachments/1008833425056211045/1123781927053303858/libno-compress.so) to the mods folder for every server.  

# Documentation
! This page is not finished yet !
If you have any questions, ask @NikZapp.
## Server config
The configuration file `servers.json` contains the names of the servers, as well as their host, port, authentication requirements and the protected areas. 
Example:
```json
{
	"main": {
		"host": "localhost",
		"port": 19130,
		"protected": {
			"house": {
				"pos1": [20, 60, 30],
				"pos2": [40, 90, 50]
			},
			"portal": {
				"pos1": [100, 64, 100],
				"pos2": [103, 68, 101],
				"destination": "aux"
			}
		},
		"auth_only": false
	},
	"aux": {
		"host": "localhost",
		"port": 19131,
		"protected": {
			"portal": {
				"pos1": [100, 64, 100],
				"pos2": [103, 68, 101],
				"destination": "main"
			}
		},
		"auth_only": true
	}
}

```
In this example, there are two servers called `main` and `aux`. The name `main` is special as it will be the default server that a client is connected to.

The host:port pair can be set to anything, even to servers outside your LAN. However, that increases network lag and is not advised.

The `protected` field holds information about the protected areas for each server. The structure for an area is as follows:
```json
"area-name": {
	"pos1": [x1, y1, z1],
	"pos2": [x2, y2, z2],
	"destination": "server-name" - optional
}
```
Any interactions inside of a `pos1`-`pos2` cuboid will be suppressed by the relay, meaning you will not be able to place/break blocks, or use items. However, being inside the area does not count as an interaction and will not be stopped.

In the provided example, the area `house` is protected from modifications, while a `portal` area in both servers allows for movement between them.

If a `destination` is provided, any interaction inside the area will cause the player to move to a specified server.  
When moving between servers, player coordinates are not changed.

If a server is set as `auth_only`, only authorized clients will be able to travel to it. If `main` is set to `auth_only`, then every client will need to authenticate to connect to the relay.
## Coordinates
**All** coordinates used/reported by the relay are internal, meaning they are in the range (0..255, 0..127, 0..255) instead of the traditional coordinates you see in the top-left of the game screen.

To find the internal coordinates of a block, use a compass on it. The coordinates will be reported in chat.
## Auth system
The relay supports [Thorium](https://github.com/NikZapp/thorium-server) as an authentication system, however this is not enforced. As long as you use the same file structure for `authorized.json`, the relay will work.

An example `authorized.json` file looks like this:
```json
{
	"123.45.67.89": {
        "level": 3,
        "name": "NikZapp"
    },
	"234.56.78.90": {
        "level": 1,
        "name": "StevePi"
    }
}
```
The ip of an authorized player should be written as a string and contain a `level` and `name` attribute. The `name` attribute corresponds to token/account name, and should be unique per person. Players can still choose any username they want, this name will only be used in logs/commands.

The `level` attribute represents the access level of a player.

| Level | Description         |
| ----- | ------------------- |
| 0     | An unwanted player  |
| 1     | An untrusted player |
| 2     | A trusted player    |
| 3     | An admin            |

If the client is not authorized before joining, a `level` of 1 is set by default.  
The access level influences things like commands: Admin commands will not be usable by anyone other than admin.

After an authorized player joins, their entry in the file is cleared, meaning that they will have to re-authenticate if they leave the server.

## Chat
To send something to all connected clients, edit the `chat.txt` file. Anything written inside will be sent, and the file will be cleared. Use this instead of the normal MCPI API.
