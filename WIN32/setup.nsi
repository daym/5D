# Auto-generated by EclipseNSIS Script Wizard
# 20.11.2007 12:16:48

Name 5D

# Defines
!define REGKEY "SOFTWARE\$(^Name)"
!define VERSION 0.6.8
!define COMPANY "X"
!define URL ""

# MUI defines
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install-colorful.ico"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY ${REGKEY}
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME StartMenuGroup
!define MUI_STARTMENUPAGE_DEFAULTFOLDER 5D
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-colorful.ico"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

# Included files
!include Sections.nsh
!include MUI.nsh

# Variables
Var StartMenuGroup

# Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuGroup
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Installer languages
!insertmacro MUI_LANGUAGE German

# Installer attributes
OutFile setup.exe
InstallDir $PROGRAMFILES\5D
CRCCheck on
XPStyle on
ShowInstDetails show
VIProductVersion 0.1.0.0
VIAddVersionKey ProductName 5D
VIAddVersionKey ProductVersion "${VERSION}"
VIAddVersionKey CompanyName "${COMPANY}"
VIAddVersionKey FileVersion "${VERSION}"
VIAddVersionKey FileDescription ""
VIAddVersionKey LegalCopyright ""
InstallDirRegKey HKLM "${REGKEY}" Path
ShowUninstDetails show

# Installer sections
Section -Main SEC0000
    SetOutPath $INSTDIR\bin
    SetOverwrite on
    File /x .svn /x *.zip /x ./externals/NSIS/ /x ./externals/nsisant/ 5D.exe
    SetOutPath $INSTDIR\share
    SetOverwrite on
    File /x .svn /x *.zip /x ./externals/NSIS/ /x ./externals/nsisant/ 5D.ico
    SetOutPath $INSTDIR\doc
    File /r /x .svn ..\doc\*
    SetOutPath $INSTDIR\share\UI
    SetOverwrite on
    File ..\lib\UI\init.5D
    SetOutPath $INSTDIR\share\String
    SetOverwrite on
    File ..\lib\String\init.5D
    SetOutPath $INSTDIR\share\Pair
    SetOverwrite on
    File ..\lib\Pair\init.5D
    SetOutPath $INSTDIR\share\Reflection
    SetOverwrite on
    File ..\lib\Reflection\init.5D
    SetOutPath $INSTDIR\share\OS
    SetOverwrite on
    File ..\lib\OS\path.5D
    SetOutPath $INSTDIR\share\OO
    SetOverwrite on
    File ..\lib\OO\init.5D
    SetOutPath $INSTDIR\share\Logic
    SetOverwrite on
    File ..\lib\Logic\init.5D
    SetOutPath $INSTDIR\share\List
    SetOverwrite on
    File ..\lib\List\init.5D
    SetOutPath $INSTDIR\share\IO
    SetOverwrite on
    File ..\lib\IO\init.5D
    SetOutPath $INSTDIR\share\Maybe
    SetOverwrite on
    File ..\lib\Maybe\init.5D
    SetOutPath $INSTDIR\share\FFI
    SetOverwrite on
    File ..\lib\FFI\init.5D
    SetOutPath $INSTDIR\share\Composition
    SetOverwrite on
    File ..\lib\Composition\init.5D
    SetOutPath $INSTDIR\share\Arithmetic
    SetOverwrite on
    File ..\lib\Arithmetic\init.5D
    SetOutPath $INSTDIR\share\Trigonometry
    SetOverwrite on
    File ..\lib\Trigonometry\init.5D
    SetOutPath $INSTDIR\share\LinearAlgebra
    SetOverwrite on
    File ..\lib\LinearAlgebra\init.5D
    SetOutPath $INSTDIR\share\Error
    SetOverwrite on
    File ..\lib\Error\init.5D
    SetOutPath $INSTDIR\share\Tree
    SetOverwrite on
    File ..\lib\Tree\init.5D
    SetOutPath $INSTDIR\share\Testers
    SetOverwrite on
    File ..\lib\Testers\init.5D

    WriteRegStr HKLM "${REGKEY}\Components" Main 1
    
    SetOutPath $SMPROGRAMS\$StartMenuGroup
    CreateShortCut "$SMPROGRAMS\$StartMenuGroup\$(^Name).lnk" "$INSTDIR\bin\5D.exe" "" "$INSTDIR\share\5D.ico" "" SW_SHOWNORMAL "" "5D"
    CreateShortCut "$SMPROGRAMS\$StartMenuGroup\$(^Name) Tutorial.lnk" "$INSTDIR\doc\programming\tutorial\index.html" "" "$INSTDIR\share\5D.ico" "" SW_SHOWNORMAL "" "5D"
    CreateShortCut "$SMPROGRAMS\$StartMenuGroup\$(^Name) Programming Manual.lnk" "$INSTDIR\doc\programming\manual\index.html" "" "$INSTDIR\share\5D.ico" "" SW_SHOWNORMAL "" "5D"
    CreateShortCut "$SMPROGRAMS\$StartMenuGroup\$(^Name) Library Reference.lnk" "$INSTDIR\doc\library\index.html" "" "$INSTDIR\share\5D.ico" "" SW_SHOWNORMAL "" "5D"

    ; Processing [HKEY_CLASSES_ROOT\5DEnvironmentFile]
Push $0
Push $1
;HKEY_CLASSES_ROOT = 0x80000000, REG_CREATE_SUBKEY = 0x0004
System::Call /NOUNLOAD "Advapi32::RegCreateKeyExA(i, t, i, t, i, i, i, *i, i) i(0x80000000, &apos;5DEnvironmentFile&apos;, 0, &apos;&apos;, 0, 0x0004, 0, .r0, 0) .r1"
StrCmp $1 0 +2
SetErrors
StrCmp $0 0 +2
System::Call /NOUNLOAD "Advapi32::RegCloseKey(i) i(r0) .r1"
System::Free 0
Pop $1
Pop $0

; Processing [HKEY_CLASSES_ROOT\5DEnvironmentFile\DefaultIcon]
WriteRegStr HKEY_CLASSES_ROOT 5DEnvironmentFile\DefaultIcon "" "$INSTDIR\share\5D.ico"

; Processing [HKEY_CLASSES_ROOT\5DEnvironmentFile\shell]
Push $0
Push $1
;HKEY_CLASSES_ROOT = 0x80000000, REG_CREATE_SUBKEY = 0x0004
System::Call /NOUNLOAD "Advapi32::RegCreateKeyExA(i, t, i, t, i, i, i, *i, i) i(0x80000000, &apos;5DEnvironmentFile\shell&apos;, 0, &apos;&apos;, 0, 0x0004, 0, .r0, 0) .r1"
StrCmp $1 0 +2
SetErrors
StrCmp $0 0 +2
System::Call /NOUNLOAD "Advapi32::RegCloseKey(i) i(r0) .r1"
System::Free 0
Pop $1
Pop $0

; Processing [HKEY_CLASSES_ROOT\5DEnvironmentFile\shell\open]
Push $0
Push $1
;HKEY_CLASSES_ROOT = 0x80000000, REG_CREATE_SUBKEY = 0x0004
System::Call /NOUNLOAD "Advapi32::RegCreateKeyExA(i, t, i, t, i, i, i, *i, i) i(0x80000000, &apos;5DEnvironmentFile\shell\open&apos;, 0, &apos;&apos;, 0, 0x0004, 0, .r0, 0) .r1"
StrCmp $1 0 +2
SetErrors
StrCmp $0 0 +2
System::Call /NOUNLOAD "Advapi32::RegCloseKey(i) i(r0) .r1"
System::Free 0
Pop $1
Pop $0

; Processing [HKEY_CLASSES_ROOT\5DEnvironmentFile\shell\open\command]
WriteRegStr HKEY_CLASSES_ROOT "5DEnvironmentFile\shell\open\command" "" '"$INSTDIR\bin\5D.exe" "%1"'
; Processing [HKEY_CLASSES_ROOT\.5D]
WriteRegStr HKEY_CLASSES_ROOT .5D "" 5DEnvironmentFile
SectionEnd

Section -post SEC0001
    WriteRegStr HKLM "${REGKEY}" Path $INSTDIR
    WriteUninstaller $INSTDIR\uninstall.exe
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    SetOutPath $SMPROGRAMS\$StartMenuGroup
    CreateShortcut "$SMPROGRAMS\$StartMenuGroup\Uninstall $(^Name).lnk" $INSTDIR\uninstall.exe
    !insertmacro MUI_STARTMENU_WRITE_END
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayName "$(^Name)"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayVersion "${VERSION}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" Publisher "${COMPANY}"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" DisplayIcon $INSTDIR\uninstall.exe
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" UninstallString $INSTDIR\uninstall.exe
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoModify 1
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)" NoRepair 1
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\$(^Name).exe" "" '"$INSTDIR\bin\5D.exe"'
SectionEnd

# Macro for selecting uninstaller sections
!macro SELECT_UNSECTION SECTION_NAME UNSECTION_ID
    Push $R0
    ReadRegStr $R0 HKLM "${REGKEY}\Components" "${SECTION_NAME}"
    StrCmp $R0 1 0 next${UNSECTION_ID}
    !insertmacro SelectSection "${UNSECTION_ID}"
    GoTo done${UNSECTION_ID}
next${UNSECTION_ID}:
    !insertmacro UnselectSection "${UNSECTION_ID}"
done${UNSECTION_ID}:
    Pop $R0
!macroend

# Uninstaller sections
Section /o -un.Main UNSEC0000
    RmDir /r /REBOOTOK $INSTDIR\dist
    RmDir /r /REBOOTOK $INSTDIR\doc
    RmDir /r /REBOOTOK $INSTDIR\lib
    RmDir /r /REBOOTOK $INSTDIR\externals
    DeleteRegValue HKLM "${REGKEY}\Components" Main
SectionEnd

Section -un.post UNSEC0001
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$(^Name)"
    DeleteRegKey HKCR 5DEnvironmentFile
    DeleteRegKey HKCR .5D
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\Uninstall $(^Name).lnk"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\$(^Name).lnk"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\$(^Name) Tutorial.lnk"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\$(^Name) Programming Manual.lnk"
    Delete /REBOOTOK "$SMPROGRAMS\$StartMenuGroup\$(^Name) Library Reference.lnk"
    Delete /REBOOTOK $INSTDIR\uninstall.exe
    Delete /REBOOTOK $INSTDIR\bin\5D.exe
    Delete /REBOOTOK $INSTDIR\share\5D.ico
    DeleteRegValue HKLM "${REGKEY}" StartMenuGroup
    DeleteRegValue HKLM "${REGKEY}" Path
    DeleteRegKey /IfEmpty HKLM "${REGKEY}\Components"
    DeleteRegKey /IfEmpty HKLM "${REGKEY}"
    RmDir /REBOOTOK $SMPROGRAMS\$StartMenuGroup
    RmDir /REBOOTOK $INSTDIR\share
    RmDir /REBOOTOK $INSTDIR\bin
    RmDir /REBOOTOK $INSTDIR
    Push $R0
    StrCpy $R0 $StartMenuGroup 1
    StrCmp $R0 ">" no_smgroup
no_smgroup:
    Pop $R0
SectionEnd

# Installer functions
Function .onInit
    InitPluginsDir
FunctionEnd

# Uninstaller functions
Function un.onInit
    ReadRegStr $INSTDIR HKLM "${REGKEY}" Path
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuGroup
    !insertmacro SELECT_UNSECTION Main ${UNSEC0000}
FunctionEnd

