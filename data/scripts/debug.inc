Debug_EventScript_DoConfetti::
	special DoDomeConfetti
	waitstate
	return


Debug_EventScript_ResetAllBerries::
	goto EventScript_ResetAllBerries
	end


.ifdef DEBUG

Debug_EventScript_ShowEndOfCurrentBeta::
	lock
	special ShowEndOfCurrentBetaScreen
	waitbuttonpress
	special HideEndOfCurrentBetaScreen
	release
	end


.else

Debug_EventScript_ShowEndOfCurrentBeta::
	end


.endif
