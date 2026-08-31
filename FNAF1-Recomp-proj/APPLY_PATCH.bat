@echo off
REM ============================================================
REM  FNAF1-Recomp v2.7.12-aibrains - patch verifier
REM  Put this file into the project ROOT (next to src\ and include\)
REM  and run it. All lines must say [OK].
REM ============================================================
echo.
echo === FNAF1-Recomp v2.7.12-aibrains patch verifier ===
echo.

if not exist "src\main.cpp" goto WRONGDIR
if not exist "include\PakAssets.h" goto WRONGDIR

findstr /C:"v2.7.4: FIRST line of the log" "src\main.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\main.cpp is OLD - no v2.7.4 marker) else (echo [OK]   src\main.cpp has v2.7.4)

findstr /C:"v2.7.4 VERDICT CORRECTION" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.4 marker) else (echo [OK]   src\GameRender.cpp has v2.7.4)

findstr /C:"v2.7.7 CLEANROOM PERSPECTIVE" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.7 clean-room Perspective) else (echo [OK]   src\GameRender.cpp has the v2.7.7 clean-room Perspective)

findstr /C:"DrawBentInstance" "include\GameRender.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\GameRender.h is OLD - no DrawBentInstance, LNK2019 guaranteed) else (echo [OK]   include\GameRender.h declares DrawBentInstance)

findstr /C:"DrawTriangles" "include\SpriteBatch.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\SpriteBatch.h is OLD - no DrawTriangles, LNK2019 guaranteed) else (echo [OK]   include\SpriteBatch.h declares DrawTriangles)

findstr /C:"OFFICE_FX_TABLES" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.8 office FX tables) else (echo [OK]   src\GameRender.cpp has the v2.7.8 office FX)
findstr /C:"RAISE_SEQ" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.9 tablet raise) else (echo [OK]   src\GameRender.cpp has the v2.7.9 tablet raise)

findstr /C:"CAMFEED_5_EMPTY    = 83," "include\PakAssets.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PakAssets.h is OLD - CAM 5 not backstage img_83) else (echo [OK]   PakAssets.h: CAM 5 = img_83 BACKSTAGE)
findstr /C:"CAMFEED_2B_EMPTY   = 0," "include\PakAssets.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PakAssets.h is OLD - CAM 2B not img_0) else (echo [OK]   PakAssets.h: CAM 2B = img_0 corner)
findstr /C:"COVE_CLOSED        = 66," "include\PakAssets.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PakAssets.h is OLD - cove stages not canonical) else (echo [OK]   PakAssets.h: cove 66/211/338/240)
findstr /C:"v2.7.10: >=0 -- img_0 (CAM 2B) must draw too" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.10 feed>=0) else (echo [OK]   src\GameRender.cpp has the v2.7.10 camfix)
findstr /C:"v2.7.11 PERSPECTIVE TUNER" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.11 tuner knobs) else (echo [OK]   src\GameRender.cpp has the v2.7.11 tuner knobs)
findstr /C:"PerspTunerAdjust" "include\GameRender.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\GameRender.h is OLD - no tuner API, LNK2019 guaranteed) else (echo [OK]   include\GameRender.h declares the tuner API)
findstr /C:"tunerToggle" "include\InputSystem.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\InputSystem.h is OLD - no tunerToggle, L3+R3 dead) else (echo [OK]   include\InputSystem.h has tunerToggle (L3+R3))
findstr /C:"IMG_LIGHT_L_HALL" "include\PakAssets.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PakAssets.h is OLD - no v2.7.8 light variants) else (echo [OK]   include\PakAssets.h has the v2.7.8 light panorama variants)

findstr /C:"v2.7.12: real kill animations" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - no v2.7.12 scare tables) else (echo [OK]   src\GameRender.cpp has the v2.7.12 kill anims)
findstr /C:"SCARE_BONNIE_KILL" "src\GameRender.cpp" >nul 2>&1
if errorlevel 1 (echo [FAIL] src\GameRender.cpp is OLD - Bonnie still uses the window pose) else (echo [OK]   src\GameRender.cpp: Bonnie kill = anim 35 frames)
findstr /C:"GetPowerOutPhase" "include\Game.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\Game.h is OLD - no power-out phases) else (echo [OK]   include\Game.h has the 4-phase power-out API)
findstr /C:"FOXY_MOVE_INTERVAL_SEC" "include\Types.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\Types.h is OLD - shared 5s opportunity) else (echo [OK]   include\Types.h has per-animatronic intervals)
findstr /C:"OnCoveLooked" "include\AnimatronicAI.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\AnimatronicAI.h is OLD - pre-v2.7.12 AI) else (echo [OK]   include\AnimatronicAI.h is the v2.7.12 brain)
findstr /C:"freddyRandomStart" "include\NightConfig.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\NightConfig.h is OLD - wrong night tables) else (echo [OK]   include\NightConfig.h has the real night tables)
findstr /C:"OnFoxyDoorBangTenths" "include\PowerSystem.h" >nul 2>&1
if errorlevel 1 (echo [FAIL] include\PowerSystem.h is OLD - percent float power) else (echo [OK]   include\PowerSystem.h has tenth-percent power)

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
echo   === FNAF1-Recomp v2.7.12-aibrains built ... ===
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
