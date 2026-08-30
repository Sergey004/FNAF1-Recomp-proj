FNAF1-Recomp v2.7.9-tabletflip -- the tablet RAISE animation + shared monitor HUD
================================================================================

SYMPTOM (user report)
---------------------
"est' animaciya OPUSKANIYA, no net animacii PODNIMANIYA plansheta" -- v2.7.8
played the drop wipe (obj 73) but the raise was an instant pop-in: the frame
the flip bar is clicked, the monitor view just replaced the office.

EVIDENCE CHAIN (all from the original EXE dumps)
------------------------------------------------
1. ANIM TABLE (application.json, JSON Export): obj 68 "panel" anim 0,
   speed 50, one-shot (repeat 1), frames:
       142, 46, 144, 132, 133, 136, 137, 138, 139, 140, 141
   Every frame is a NATIVE 1280x720 window-space image.
   obj 73 "flip down 2" anim 0 = the SAME stack reversed:
       141, 140, 139, 138, 137, 136, 133, 132, 144, 46, 142
   -> the raise is the wipe played backwards (WIPE_SEQ in v2.7.8).
   Pixels: 142 = tablet parked on the desk (bottom edge + red/green LEDs),
   133 = half risen, 141 = almost fully covering the screen.
   Frame time: speed 50 -> (100/50)/50 = 0.040 s -> 11 frames = 0.44 s.
2. EVENT GROUP 130 (raise trigger): viewing==0 AND flip it==0 AND click on
   the 'flip up' zone AND power down==0 AND fox progress != 5 ->
   Create 'panel', flip it := 1, flip up alterable[0] := 1,
   hide 'Active 8', Speaker #6 'CAMERA_VIDEO_LOA_60105303' (already wired
   in our build via Snd::CAMERA_SWITCH / OnCameraChange), hide 'Active 6'.
3. EVENT GROUP 133 (raise finished): ON panel 'animation finished' ->
   Destroy panel, 'set viewing to' := last clicked -> group 18 fires:
   blip := 1, viewing := set viewing to -> group 16: blip==1 ->
   Create 'Active 5' (the 9-frame white flash, FLASH_SEQ from v2.7.8)
   + 'blip3' sound; group 17 destroys the flash when its anim ends.
   => the feed appears WITH the white flash over it, never without.
   Group 18 is the SAME trigger the map buttons use (groups 136+:
   click a cam button -> 'set viewing to' := N) -- so EVERY committed
   cam change flashes, not only the raise arrival. The user's reference
   screenshot (feed under white noise) is exactly this flash mid-dissolve.
4. EVENT GROUPS 330/331: the 'flip panel' bar (img_420) is SHOWN while
   flip up alterable[0] == 0 and HIDDEN otherwise -> the bar hides the
   moment the raise starts and stays hidden during the drop (group 322
   sets alterable[0] := 1; group 135 re-shows it on click).
5. LAYER-3 HUD IS SHARED: groups 81/82 (viewing==0 / viewing>0) toggle
   ONLY 'Active 5' + 'Active' (static) + 'Active 2' (REC) + 'frame'
   (bezel) -- all layer 2. The clock (img_251), 'night word' (img_447),
   'Power left:' (img_207), '%' (img_208), 'Usage:' (img_189), the usage
   bars and 'mute call' are layer 3 and are NEVER hidden by the monitor
   state -> the original monitor shows "1 AM / Night N / Power left: NN% /
   Usage" exactly like the office (confirmed by the user's reference
   screenshot). v2.7.8 and earlier did NOT draw the HUD on the monitor.

IMPLEMENTATION (diff vs v2.7.8-officefx)
----------------------------------------
* GameRender.cpp:
  - RAISE_SEQ[11] = {142,46,144,132,133,136,137,138,139,140,141},
    RAISE_FRAME_T = 0.040f (speed 50), placed next to WIPE_SEQ;
  - m_raiseT initialized to -1 in the constructor;
  - RenderOffice: drop branch now also cancels m_raiseT/m_flashT;
    the flip bar is skipped while m_raiseT >= 0 or m_wipeT >= 0;
  - the office HUD block moved VERBATIM into the new private method
    DrawSharedHud(game, phonePlaying) (zero code changes inside);
  - RenderCamera(game, phonePlaying): on the monitor-up edge starts
    m_raiseT; while it runs, the office is rendered normally and the
    RAISE_SEQ frame is drawn FLAT on top (window-space art, straight
    edges baked into the images -- NOT bent through the panorama, the
    same screen-space treatment as the v2.7.8 wipe); when it finishes,
    the feed takes over and flashes once (see the m_prevCam check);
  - m_prevCam: EVERY committed cam change restarts m_flashT -- the
    group 18->16 chain shared by map clicks and the raise commit (the
    drop resets m_prevCam so the next arrival always flashes);
  - RenderCamera now calls DrawSharedHud above the map, under the bar.
* GameRender.h: RenderCamera signature + DrawSharedHud decl + m_raiseT
  + m_prevCam.
* main.cpp: RenderCamera(game, s_phonePlaying); banner v2.7.9-tabletflip.
* APPLY_PATCH.bat: version bump + new verifier line (findstr RAISE_SEQ).

WHY FLAT, NOT BENT
------------------
The panel frames are 1280x720 (window size, not the 1600x720 scene size)
and the tablet edges baked into the art are straight horizontal lines.
Bent through the PANORAMA curve they would curve down at the sides. The
v2.7.8 wipe uses the same images flat and looked correct, so the raise
gets the same screen-space treatment (DrawFrame, not DrawBentInstance).

EDGE CASES
----------
* Drop in the middle of a raise: RenderOffice's drop branch fires the
  wipe and resets m_raiseT/m_flashT -> clean state, no stuck timers.
* Jumpscare while the monitor is up: state leaves the office/camera
  path; the next raise starts from m_prevMonitor handling (wiped states
  restart the anim cleanly).
* Raise sound: Snd::CAMERA_SWITCH already plays in OnCameraChange (the
  same CAMERA_VIDEO_LOA_60105303 the original plays in group 130).

VERIFICATION
------------
* Byte anchors: every old block found exactly once before replace.
* Brace balance before == after (both patched files).
* CRLF purity kept (GameRender.cpp / main.cpp / APPLY_PATCH.bat pure
  CRLF; GameRender.h mixed layout preserved byte-for-byte).
* Markers kept: 'v2.7.7 CLEANROOM PERSPECTIVE', 'OFFICE_FX_TABLES',
  'v2.7.4: FIRST line of the log' (banner comment in the bat).
* Preview BEFORE building: download/tablet_flip_preview/ (raise frames
  composited over the real bent office, exact 0.040 s/frame timing).

ROLLOUT
-------
* Artifact: FNAF1-Recomp-proj-v2.7.9-tabletflip.zip (full tree).
* Rollback: FNAF1-Recomp-proj-v2.7.8-officefx.zip (or 2.7.7a / 2.7.7 /
  2.7.6 / 2.7.5 / 2.7.4-RESTORED).
