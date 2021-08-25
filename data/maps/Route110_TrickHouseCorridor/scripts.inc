Route110_TrickHouseCorridor_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Route110_TrickHouseCorridor_OnTransition
	.byte 0

Route110_TrickHouseCorridor_OnTransition:
	setvar VAR_TOURNAMENT_HALL_OPPONENT, 1
	end

