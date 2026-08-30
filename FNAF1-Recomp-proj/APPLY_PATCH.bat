@echo off
REM ============================================================
REM  FNAF1-Recomp v2.7.4-RESTORED - patch verifier
REM  Put this file into the project ROOT (next to src\ and include\)
REM  and run it. All lines must say [OK].
REM ============================================================
echo.
echo === FNAF1-Recomp v2.7.4-RESTORED patch verifier ===
echo.

if not exist "src\main.cpp" goto WRONGDIR
if not exist "include\PakAssets.h" goto WRONGDIR

findstr /C:"v2.7.4: FIRST line of the log" "src\main.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\main.cpp is OLD - no v2.7.4 marker) else (echo [OK]   src\main.cpp has v2.7.4)

findstr /C:"v2.7.4 VERDICT CORRECTION" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.4 marker) else (echo [OK]   src\GameRender.cpp has v2.7.4)

findstr /C:"v2.7.4: name the normalization target" "src\PakLoader.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\PakLoader.cpp is OLD - no v2.7.4 marker) else (echo [OK]   src\PakLoader.cpp has v2.7.4)

findstr /C:"PakHotspot(i32 px, i32 py)" "include\PakAssets.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PakAssets.h is OLD - pre-v2.6.1, VC10 build will fail) else (echo [OK]   include\PakAssets.h has VC10 ctors)

findstr /C:"IMG_FLIP_BAR" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp lacks IMG_FLIP_BAR - pre-v2.7 purple slab code) else (echo [OK]   src\GameRender.cpp draws the real flip bar img_420)

findstr /C:"_M_PPCBE" "src\PakLoader.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\PakLoader.cpp lacks the 360 BE pass - noise on console) else (echo [OK]   src\PakLoader.cpp has the 360 big-endian audio pass)

findstr /C:"ContentBuilder" "docs\EXE_VERSION.md" >nul 2>&1
if errorlevel 1 (echo [WARN] docs\EXE_VERSION.md missing - optional) else (echo [OK]   docs\EXE_VERSION.md present)

echo.
echo Next step in Visual Studio 2010:
echo   Build -^> Rebuild Solution   (NOT just F5)
echo Then run. The FIRST line of the Output window must be:
echo   === FNAF1-Recomp v2.7.4-RESTORED built ... ===
echo and later:
echo   PakLoader: sounds normalized (... target BE(360))
echo If you see those, the new code IS in the build.
echo.
pause
exit /b 0

:WRONGDIR
echo [FAIL] src\main.cpp not found here.
echo Put APPLY_PATCH.bat into the INNER project folder:
echo   C:\Users\123\Desktop\FNAF1-Recomp-proj\FNAF1-Recomp-proj\
echo (the same folder that contains src\, include\ and the .vcxproj)
echo.
pause
exit /b 1
