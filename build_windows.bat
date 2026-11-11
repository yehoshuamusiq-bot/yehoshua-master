@echo off
REM ==========================================================
REM  YEHOSHU'A MUSIQ - MASTER : compilation Windows en 1 clic
REM  Prérequis : Visual Studio 2022 ou plus récent (charge "Développement C++"),
REM              CMake et Git (installés avec Visual Studio ou à part)
REM ==========================================================
cd /d "%~dp0"
echo [1/3] Configuration (telecharge JUCE automatiquement la 1re fois)...
cmake -B build -A x64 || goto :err
echo [2/3] Compilation Release...
cmake --build build --config Release --target YehoshuaMaster_VST3 YehoshuaMaster_Standalone --parallel || goto :err
echo [3/3] Copie du plugin...
if not exist "dist" mkdir dist
xcopy /E /I /Y "build\YehoshuaMaster_artefacts\Release\VST3\YEHOSHUA MASTER.vst3" "dist\YEHOSHUA MASTER.vst3" >nul
copy /Y "build\YehoshuaMaster_artefacts\Release\Standalone\YEHOSHUA MASTER.exe" "dist\" >nul
echo.
echo  TERMINE ! Le plugin est dans le dossier "dist".
echo  Copie "YEHOSHUA MASTER.vst3" dans : C:\Program Files\Common Files\VST3
echo  puis dans FL Studio : Options ^> Manage plugins ^> Find installed plugins.
pause
exit /b 0
:err
echo.
echo  ERREUR pendant la compilation. Verifie que Visual Studio 2022 (C++), CMake et Git sont installes.
pause
exit /b 1
