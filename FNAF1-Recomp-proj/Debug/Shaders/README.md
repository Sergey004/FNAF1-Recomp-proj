# Shaders/ -- Xbox 360 offline shader compilation

These `.hlsl` files are NOT compiled at runtime anymore (we used to call
`D3DXCompileShader` inside `SpriteBatch::Init()`, but that pulls the whole
HLSL compiler onto the PPC CPU at startup -- slow, and not guaranteed to be
available outside a devkit). Instead they're compiled once at build time
into raw shader bytecode (`.vsh` / `.psh`), which `SpriteBatch::Init()` just
loads and hands straight to `CreateVertexShader` / `CreatePixelShader`.

## Wiring this into the VS Xbox 360 project (one-time setup)

For each `.hlsl` file in this folder, in Visual Studio:

1. Right-click the file in Solution Explorer -> **Properties**.
2. Set **Item Type** / **Configuration Properties** to **Custom Build Tool**.
3. **Command Line** (adjust to whatever your XDK's shader compiler is
   actually called -- on most 360 XDKs this is invoked the same way the
   XDK sample projects do it, check an XDK sample's `.vcxproj` for the
   exact tool name if this doesn't match):
   ```
   "$(XEDK)\bin\win32\xbvsa.exe" /Fo "$(ProjectDir)Shaders\%(Filename).vsh" "%(FullPath)"
   ```
   for vertex shaders (`sprite_vs.hlsl`), and:
   ```
   "$(XEDK)\bin\win32\xbpsa.exe" /Fo "$(ProjectDir)Shaders\%(Filename).psh" "%(FullPath)"
   ```
   for pixel shaders (`sprite_ps.hlsl`).
4. **Outputs**: `$(ProjectDir)Shaders\%(Filename).vsh` (or `.psh`)
5. Make sure **Additional Dependencies** includes the source `.hlsl` file
   itself, so VS knows to recompile when you edit it.

If your XDK version names the compilers differently (some ship them as
`vsa.exe`/`psa.exe` without the `xb` prefix, depending on version), just
swap the executable name -- the important part is: HLSL source in this
folder in, `.vsh`/`.psh` bytecode out, next to it.

## Deploying compiled shaders alongside the game

Copy the resulting `Shaders/*.vsh` and `Shaders/*.psh` files to the same
relative `Shaders/` folder wherever `default.xex` ends up on the console
(alongside it in the deploy/package folder). `SpriteBatch::Init()` loads
them from `game:\Shaders\sprite_vs.vsh` and `game:\Shaders\sprite_ps.psh`
at runtime -- see the loader in `SpriteBatch.cpp`.
