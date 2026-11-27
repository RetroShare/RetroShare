; Autostart entries in HKCU\...\Run.
; RetroShare names its own entry "RetroShare-<hash of the exe path>" (one per installation, see
; RshareSettings::setRunRetroshareOnBoot), the installer writes the legacy "RetroShare" one.
; RemoveRunEntriesForInstDir deletes every "RetroShare*" value whose command points into $INSTDIR,
; so an uninstall leaves no dead entry and an upgrade does not start the same exe twice. Entries of
; other installations are left alone.

!define RUN_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Run"

!macro RemoveRunEntriesForInstDir_Function _PREFIX
Function ${_PREFIX}RemoveRunEntriesForInstDir
	Push $0 ; value index
	Push $1 ; value name
	Push $2 ; value data
	Push $3 ; "$INSTDIR\"
	Push $4 ; length of $3
	Push $5 ; scratch

	StrCpy $3 "$INSTDIR\"
	StrLen $4 $3
	StrCpy $0 0

	loop:
		ClearErrors
		EnumRegValue $1 HKCU "${RUN_REG_KEY}" $0
		; The end of the list sets the error flag; the unnamed default value
		; comes back as "" without it and must not stop the loop
		IfErrors done
		StrCmp $1 "" next

		; Only look at our own values
		StrCpy $5 $1 10
		StrCmp $5 "RetroShare" 0 next

		; Strip the optional leading quote, then compare the path prefix (StrCmp ignores case)
		ReadRegStr $2 HKCU "${RUN_REG_KEY}" $1
		StrCpy $5 $2 1
		StrCmp $5 '"' 0 +2
			StrCpy $2 $2 "" 1
		StrCpy $5 $2 $4
		StrCmp $5 $3 0 next

		; Deleting shifts the following values down: keep the same index
		DeleteRegValue HKCU "${RUN_REG_KEY}" $1
		Goto loop

	next:
		IntOp $0 $0 + 1
		Goto loop

	done:
	Pop $5
	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Pop $0
FunctionEnd
!macroend

!insertmacro RemoveRunEntriesForInstDir_Function ""
!insertmacro RemoveRunEntriesForInstDir_Function "un."
