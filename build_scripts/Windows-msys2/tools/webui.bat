setlocal

echo.
echo === webui
echo.
title Build webui

if not exist "%RsWebuiPath%" (
	echo Checking out webui source into %RsWebuiPath%
	%EnvMSYS2Cmd% "pacman --noconfirm --needed -S git"
	git clone https://github.com/RetroShare/RSNewWebUI.git "%RsWebuiPath%"
) else (
	echo Webui source found at %RsWebuiPath%		
)

pushd "%RsWebuiPath%\webui-src\make-src"
%EnvMSYS2Cmd% "sh build.sh"
popd

endlocal