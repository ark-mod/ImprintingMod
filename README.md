# ARK Beyond API: Imprinting Mod (Plugin)

## Introduction

Advance imprinting/cuddle support for ARK Survival Evolved servers using ARK Beyond API.

The purpose of this mod is to get around the fact that players need to wake up in the middle of the night in order to fully imprint most bred creatures.

Players are allowed to do approximately 1/3 of all imprints in advance using the chat command /imprint. The command will force any pending imprint countdown to finish immediately. To balance this somewhat all advance imprints require kibbles rather than the standard cuddle, walk or kibble.

The number of imprints available for advance imprints varies with the species of the baby dino. It is calculated using the time it takes for a particular species to fully grow and limited to two per full day.

Chat
* **/Imprint**: Trigger an advance cuddle which forces the current imprint countdown to finish immediately.
* **/ImprintCheck**: Print imprint and advance cuddle details for the owned baby dino in-front of the player.

## Configuration

`BabyMatureSpeedMultiplier` should be set according to the same settings in your ARK server.

`DatabasePath` must be a valid path and the directory must exist. The plugin will not attempt to create the directory.

`Species` entries are required for each dino species that allows advance imprints. The floating point value is the default maturation time in seconds. It is available in the devkit and from the field breeding→maturationTime on each species in https://github.com/cadon/ARKStatsExtractor/blob/master/ARKBreedingStats/json/values.json.

```json
{
	"DatabasePath":  "C:\\ImprintingMod.db",
	"BabyMatureSpeedMultiplier": 1.0,
	"Species":{
		"Allo_Character_BP_C": 166666.65625,
		"Angler_Character_BP_C": 133333.328125,
		"Ankylo_Character_BP_C": 175438.59375,
		...
	}
}
```

## Acknowledgements

This plugin is based on Michidu's work on Ark-Server-Plugins and ARK Beyond API. The basic plumbing code is copied directly from those plugins.

## Links

My ARK Beyond API Fork (https://github.com/tsebring/ARK-Server-Beyond-API)

ARK Beyond API by Michidu (https://github.com/Michidu/ARK-Server-Beyond-API)

Ark-Server-Plugins (https://github.com/Michidu/Ark-Server-Plugins)