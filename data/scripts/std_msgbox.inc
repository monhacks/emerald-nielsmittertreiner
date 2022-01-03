Std_MsgboxNPC:
	lock
	faceplayer
	specialvar VAR_RESULT, IsPlayerInFlashCave
	call_if_eq VAR_RESULT, FALSE, Std_MsgboxLoadSpeechBubble
	message NULL
	waitmessage
	waitbuttonpress
	specialvar VAR_RESULT, IsPlayerInFlashCave
	compare VAR_RESULT, 0
	call_if_eq VAR_RESULT, FALSE, Std_MsgboxDestroySpeechBubble
	release
	return

Std_MsgboxLoadSpeechBubble:
	load_speech_bubble VAR_LAST_TALKED
	return

Std_MsgboxDestroySpeechBubble:
	destroy_speech_bubble
	return

Std_MsgboxSign:
	lockall
	message NULL
	waitmessage
	waitbuttonpress
	releaseall
	return

Std_MsgboxDefault:
	message NULL
	waitmessage
	waitbuttonpress
	return

Std_MsgboxYesNo:
	message NULL
	waitmessage
	yesnobox 20, 8
	return

Std_MsgboxGetPoints:
	message NULL
	playfanfare MUS_OBTAIN_B_POINTS
	waitfanfare
	waitmessage
	return

@ Never used, pokenavcall is always used directly instead
Std_MsgboxPokenav:
	pokenavcall NULL
	waitmessage
	return

EventScript_UnusedReturn:
	return

Common_EventScript_SaveGame::
	special SaveGame
	waitstate
	return
