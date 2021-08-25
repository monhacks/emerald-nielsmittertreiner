Std_MsgboxNPC:
	lock
	faceplayer
	specialvar VAR_RESULT, IsPlayerInFlashCave
	compare VAR_RESULT, 0
	call_if_eq Std_MsgboxLoadSpeechBubble
	message 0x0
	waitmessage
	waitbuttonpress
	specialvar VAR_RESULT, IsPlayerInFlashCave
	compare VAR_RESULT, 0
	call_if_eq Std_MsgboxDestroySpeechBubble
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
	message 0x0
	waitmessage
	waitbuttonpress
	releaseall
	return

Std_MsgboxDefault:
	message 0x0
	waitmessage
	waitbuttonpress
	return

Std_MsgboxYesNo:
	message 0x0
	waitmessage
	yesnobox 20, 8
	return

Std_MsgboxGetPoints:
	message 0x0
	playfanfare MUS_OBTAIN_B_POINTS
	waitfanfare
	waitmessage
	return

Std_10:
	pokenavcall 0x0
	waitmessage
	return

EventScript_UnusedReturn:
	return

Common_EventScript_SaveGame::
	special SaveGame
	waitstate
	return
