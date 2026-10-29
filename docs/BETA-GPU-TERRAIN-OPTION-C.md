# Beta branch: Option C - Native GPU terrain (clipmap)

Goal: replace the CPU sliding-window heightmap rendering (129x129 tile VBs,
drawOrg streaming, updateCenter/calcVis) with a camera-centered GPU clipmap
sourced from height/color textures. The window subsystem (and its whole class
of bugs - black terrain, wrong culling direction, drawOrg jumps) disappears.

Branch: `beta` (cut from main @ ebc3f65 - Option B landed as the interim fix).

## Why (and honest scope)
- Option B (AABB rect) is verified and ships on main; C is the structural
  modernization, not a patch.
- Blast radius: rendering only. Logic, collision, shroud, selection, radar all
  read WORLD heights/coords from WorldHeightMap - unchanged.
- External drawOrg consumers to keep working: W3DProjectedShadow,
  W3DCustomEdging, W3DShroud (its camera-anchored window uses its own
  visStart - independent of terrain tiles as long as shroud math holds).

## Architecture

### Data (built once per map, rebuilt on height change hook)
1. `HeightTex` - R16_UNORM, W x H = map world extent / MAP_XY_FACTOR (cell grid),
   containing display height * scale. Sampled in VS: worldToUV = worldXY / mapExtent.
2. `ColorTex` - A8R8G8B8: baked static vertex diffuse (getStaticDiffuse) - what
   updateVB writes as vertex color today. Same grid.
3. Atlas/alpha maps: UNCHANGED - terrain shader already samples them by
   world-space UV (stretch constants); the clipmap VS/FS keeps those UVs.
4. Cloud/noise/lightmap passes: unchanged (world-space UV formulas in terrain.frag).

### Geometry (drawn every frame, no CPU culling)
- `ClipmapMesh`: concentric rings of grid patches centered at camera-snap
  position (snap to powers of two x cellSize to avoid swimming): rings at
  1,2,4,8,16,32,64 x base spacing, ~64x64 grid per ring, vertex attribute =
  local grid XY only; VS displaces:
    world = snapCenter + localXY * ringScale
    world.z = texture(HeightTex, worldToUV(world)).r
- Total verts ~30-50k regardless of map size; one static index buffer (skirts
  optional for ring seams).
- The far ring replaces the129-cell window entirely.

### Draw path (phased, flag-gated)
- Flag: `RTS_CLIPMAP=1` (env) -> terrain base + alpha passes render via
  clipmap pipeline; LEGACY path remains default until verified.
- Phase1: base pass (mode0/1) via clipmap; legacy still draws shroud/extra
  passes over old tiles (dual-path tolerated briefly).
- Phase2: shroud multiply + extra-blend + shorelines move onto clipmap UVs;
  then delete tile machinery (updateCenter/calcVis/drawOrg streaming,
  m_vertexBufferTiles, updateVB) and pin drawOrg to (0,0) full map.
- Phase3: LOD polish (ring transition dithering), optional compute-based
  height updates (dynamic crater/decal height edits if ever needed).

### Pipeline
- New VkPipeline: VS=clipmap.vert (height fetch + color fetch + atlas UV),
  FS=terrain.frag reused (mode plumbing identical). Depth/cull states mirror
  current native terrain pipelines. Shroud pass: same clipmap VS + shroud
  sample FS (camera-space texgen matrix already proven).

## Risks / open questions
- Ring seams (skirts vs crack-free topology) - visual QA.
- staticLightingChanged = full ColorTex re-upload (cheap vs VB rebuilds).
- W3DProjectedShadow/CustomEdging assume drawOrg - keep drawOrg meaningful
  (org=(0,0), full extent) so their math degenerates to identity.
- Roads/bibs draw in world space already (independent).
- RTS_CLIPMAP=0 must be bit-identical behavior to the Option-B main build.

## Milestones
1. [x] M1 LANDED: HeightTex+ColorTex builders + upload (R16 height byte*257 exact
   vs legacy z; A8R8G8B8 static diffuse same packed value updateVB writes),
   ClipMap built cells=185x185 border=30 shell proof, zero crashes.
2. [x] M2a LANDED: full-map quad-corner grid (legacy topology, FLIP_TRIANGLES
       payload rotation replicated) + clipmap.vert (height/color texture fetch
       in VS, baked atlas/alpha UVs) + GetClipmapPipeline + VulkanDrawClipmapPass
       (UINT32 indices). Gate: RTS_CLIPMAP=1 env, mode0 base pass only; legacy
       tiles stay the fallback/default (boot parity verified). Shell proof:
       mesh=185x185 idx=205350, pipeline built, draw ACTIVE, terrain mean~56
       bright, zero crashes. NOTE: the runtime loads *.spv from the DATA dir
       shaders/ - ship clipmap.vert.spv there (build/bin/shaders is not read).
       Remaining M2: alpha+shroud passes on clipmap (visual parity on missions),
       then M4 deletes the tile subsystem; rings/LOD = M5.
2b. [x] M2b COMPLETE + VERIFIED: alpha(mode1) AND shroud-multiply passes now run
       on the clipmap BY DEFAULT under RTS_CLIPMAP=1 (opt OUT shroud via
       RTS_CLIPMAP_NOSHROUD=1; legacy stays the default with the flag unset).
       CRASH ROOT CAUSE FOUND VIA VALIDATION LAYER: vkUpdateDescriptorSets was
       called with count=2 - binding2 (shadowMap/colorTex) was NEVER written;
       clipmap.vert statically samples binding2 (color fetch) => UB => lavapipe
       SEGV at draw#2 (diag: fetch-stubbed VS ran215 draws stable, real fetch
       died every time; VUID-vkCmdDrawIndexed-None-08114 "Binding 2 never
       updated"). Fixed writes=3. Also: texture descriptor stageFlags were
       FRAGMENT-only (VS access invalid) -> VERTEX|FRAGMENT; field debug
       probes that recorded barriers+copies INSIDE the active render pass
       (EndFrame color/depth readback every60f + unit-texture TEXREADBACK) are
       DEFAULT-OFF behind RTS_SCREENDUMP=1 (suspects in the field amdvlk
       mission crash); depth barriers fixed to DEPTH|STENCIL (D24S8).
       VERIFICATION: full clipmap boots PASS=3/3 (hb~170 each, shroud pipe
       built); MISSION cinematic renders correctly on the clipmap path (bright
       roads/plaza/buildings, fin2 mean=57 incl. letterbox), zero crashes.
3. [x] M4 COMPLETE + VERIFIED: tile/window subsystem retired under
        RTS_CLIPMAP=1. initHeightData forces m_drawEntireTerrain=TRUE +
        full-extent x/y + drawOrg(0,0); the tile VB/IB allocation block is
        skipped (clipmap branch restores m_map/stage/destAlpha/scorch that
        freeMapResources releases; tile counts=0, tiles=NULL so updateCenter
        early-returns and tile loops are zero-iterations). CRASH ROOT CAUSE
        FOUND: freeMapResources does REF_PTR_RELEASE(m_map) - the first M4
        build did not re-set m_map in the clipmap branch => roads preload
        (updateSegLighting->TheTerrainRenderObject->getMap()->getBorderSize())
        NULL-deref (fault=0x14) at map load. Fixed with REF_PTR_SET(m_map,pMap)
        + stage/destAlpha/scorch recreation. VERIFY: clipmap boot hb=157
        drawOK=11 FAILED=0 mean~65; legacy boot hb=145 mean~63; clipmap
        MISSION_OK mean~24 == legacy mean~23, zero crashes both modes.
4. [x] M5 PARTIAL: chunk-major 32x32-cell IB + height-derived bounding spheres +
   per-chunk frustum cull landed with DEFAULT-ON clipmap (RTS_CLIPMAP=0 =
   legacy-only opt-out; shader-availability gate keeps tiles when clipmap .spv
   missing); full-grid guard raised 250k->500k cells covers all ZH maps so
   concentric-ring LOD/seams stay deferred. Field black-terrain ROOT CAUSE was
   NOT the culling: W3DShroud sized its texture from the pre-M4 31-cell window
   -> clamped-black border sampled outside camera window (direction-dependent
   black moving with camera); fixed by setDrawOrg(0,0) BEFORE shroud->init.

Status: M1-M5 landed (ring LOD deferred behind the 500k guard); field rounds
2-3 (blending/bridges/clouds/minimap/letterbox + dynamic lights), D3DX stub
class (#4b), beta9 water rewire carried by commit messages; round-5 triage
in the ADDENDUM below.

## ADDENDUM 2026-10-25 — round-5 field triage on the clipmap pipeline (beta10)

Field report on `v1.3-beta` (beta9 exe, Win10 + campaign USA01-style maps).
Everything landed on the clipmap side WITHOUT touching the design above:

- CLIFF TEXTURE CORRUPTION = sampler semantics split (see AUDIT #10 item 2):
  atlas sampler is now mode-parity `repeat = (mode >= 2)` at the pass call
  site in dx8wrapper.cpp - CLAMP for modes0/1 (identical to the legacy tile
  pass, which ran the SAME out-of-range campaign cliff UVs through a CLAMP
  sampler), WRAP for mode2 cloud (CPU slide offsets wrap and REQUIRE sampler
  wrap). Slot2 noise sampler gained the `repeatShadow` param (WRAP in the
  clipmap call, default CLAMP for the shadowMap callers in VkDescriptors.h/
  .cpp).
- Driver-strictness UB class fixed at pipeline creation (GetClipmapPipeline +
  GetClipmapShroudPipeline in VkShader.cpp): the declared-but-never-bound
  INSTANCE binding1 (stride64 dummy normal, attr location1) is gone;
  bindingCount=1 / attributeCount=4. Unbound vertex inputs are UB and strict
  Windows drivers can drop exactly the terrain draws this class touched -
  the "field is healthy under lavapipe but suspicious on Windows" direction
  this fix closes.
- Field-log verdict that kept the pipeline design innocent: all clipmap modes
  0/1/2 + shroud pipelines BUILT on the user's machine, 0 FAILED; the
  mode0/1 OK-line sparseness was breadcrumb `dex%60` sampling on a 3-mode
  cycle (60 mod 3=0 always samples mode2), NOT a failing pass. Breadcrumb
  sampling must not share a factor with stream periods.
- The letterbox renderer rewrite and the `VK: MDRW`/`VK: LB state` field
  probes for the black water/bridges/buildings class live on the draw side
  (W3DDisplay.cpp / VulkanDrawIndexed) and are carried by the beta10 commit
  + AUDIT #10; open suspects on Windows are driver strictness and dest-alpha
  composition (feather water blend DESTALPHA/INVDESTALPHA, W3DWater.cpp:3244-
  45), NOT terrain residency (all 1776+ wine MDRW draws miss=0).

## ADDENDUM 2026-10-25 (round-6, beta11) — water env-bump via stage-2 + the constants that had to move together

- The beta11 water rework raised `MAX_TEXTURE_STAGES` (dx8wrapper.h) 2->4. The
  `WWASSERT(MAX_TEXTURE_STAGES==MeshMatDescClass::MAX_TEX_STAGES)` at
  dx8renderer.cpp:1187 pins `MeshMatDescClass::MAX_TEX_STAGES` (meshmatdesc.h:68)
  too — and the split-table / meshmatdesc texture arrays
  (`Texture[MAX_PASSES][MAX_TEX_STAGES]`, UVSource, TextureArray, MAX_UV_ARRAYS)
  are the terrain/mesh material storage this Option-C pipeline feeds. Both
  constants move ATOMICALLY or `Generate_Texture_Categories` reads past the
  2-entry table: the symptom is an AV inside `DX8TextureCategoryClass` ctor
  (garbage `textures_[2]` vtable probe), i.e. a crash at first scene build —
  exactly what the first beta11 soak hit before meshmatdesc.h was raised.
- Material semantics unchanged: W3D material parsing still caps stages via
  matpass.h's own `MAX_TEX_STAGES = 2` enum (ZH assets use <=2 per pass); the
  extra slots exist only for the port's stage2 noise binding on trapezoidWater
  (op7 ADDPRODUCT: prev + sparkle*noise, `D3DTOP_MULTIPLYADD` carrier, CAMERASPACE
  POSITION texgen + `texMat2` UBO at 704). GPU terrain draws never see
  `TEX2_OP>0` — the capture gates tex2Op on `BoundTextures[2] && stage2op!=DISABLE`.

## ADDENDUM 2026-10-27 (round-7, beta13) — GPU terrain RECEIVES shadows: set1 UBO + slot3 sampler on the clipmap pipeline

- terrain.frag grew the shadow-receive tail: shadowTint() PCF over `clipShadowMap`
  (set 0 binding 3, sun-shadow DEPTH map; white = depth 1.0 => PCF term 0 => untouched)
  + a `set=1 FrameData` UBO (lightMvp, shadowParams, ambients, fog) gated to MODE 0/1.
  The CLIPMAP pipeline (GetClipmapPipeline, modes 0-1 = clipmap.vert + m_terrainFragModule)
  SHARES m_pipelineLayout with the mesh pipeline, so the C++ side had to match: VulkanDrawClipmapPass
  now (a) AllocateTextureSet with a 4th pair shadowTex = (g_lightMvpValid && g_nativeShadowDepth)
  ? g_nativeShadowDepth : &VulkanWhiteTexture, (b) binds TWO sets `0, 2, sets` with a per-SCENE
  cached FrameData UBO set (sClipScene/sClipFrameSet; g_vkSceneCounter++ in Begin_Scene) - per-chunk
  UBO allocs drained the 2048-slot ring under the shadow pass scene re-render (>1900 meshes x2),
  turning later clipmap draws into UINT32_MAX/'ClipMap draw FAILED' = black ground (AUDIT #12.3).
- NativeShadow plumbing got the lazy-RT fixes (AUDIT #12.4): eager VulkanGetRtFramebuffer
  before VulkanGetRtDepthOf in Ready(), depth refresh after EnsureTargetPass() in OpenRT();
  without them Ready() saw depth=null forever (triple created only on first bind-as-target)
  = zero shadow draws, i.e. the shader tail above silently never ran.
- Soak proof local (lavapipe/wine): Tournament Lake + ShellMap1 exit=124 timeout-kill, 0
  `Exception c0000005`, captures show blob shadows ON the ground + painted radar + opaque
  swapchain. The unbound-descriptor AV class was reproduced locally ONLY after the shader
  grew past the C++ plumbing - remember the bisect matrix (per-.spv swap) over symbol hunts.

## Addendum (beta14, round 8) - shroud-texture bridge pass fixed C++-side, GLSL chain untouched
- ShroudTextureShader::set now composes D3DTS_TEXTURE0 with an EXACT general affine inverse of
  render_state.view (inline adjugate/det; det-guard keeps curView). Reason: the ZH view 3x3 block
  is SKEWED (row0.row1 = 0.609, not orthonormal) - Matrix3D::Get_Inverse (orthogonal/transpose
  delegate) mis-inverts it, and W3D Matrix4::Inverse() mis-indexes its cofactors. Both were
  measured numerically against the hand inverse in this round.
- mesh.frag selectUV keeps the ORIGINAL single uv chain (sel>=4 samples (texMat*viewMat*world).xy):
  with the C++ matrix correct the cancellation is exact (uv probe (0.120,0.100) == terrain clipmap
  formula (572+40)*0.000195), so no shroud special case exists in GLSL. A shader-side special case
  using a stale vp reference broke glslang and BLOCKED the RTS link (stale RTS.exe silently reused)
  - build-chain lesson: verify shaders/ target built, never trust the binary alone (strings-check
  the fixes/probes).
- The two remaining curView.Inverse() call sites in W3DShaderManager (mask ~1236, cloud/noise
  ~1519) are left alone: their phase error is imperceptible on road feather/cloud layers, and
  the general-inverse helper can be hoisted there in beta15 if the retail parity audit wants it.

## Addendum (beta14, round 9) - water body blend: dstA=0 port artifact
- drawTrapezoidWater's soft-edge body blend (DESTALPHA,INVDESTALPHA) assumes the retail A8R8G8B8
  backbuffer where dstA=1 after opaque terrain. The port's opaque terrain writes framebuffer alpha from
  mesh.frag's alpha-op chain = 0 on clipmap vertices -> water body contributed exactly zero pixels (drew=1).
  Port now rewrites the (7,8) pair to (ONE,ZERO) replace-blend in VulkanBuildPipelineState (grep D3DBLEND_DESTALPHA:
  single consumer in W3DWater.cpp) = faithful emulation of retail's dstA=1. Shaders untouched.
- If a future alpha-transparent effect needs real dstA, the port must first make OPAQUE variants write
  a=vertex-alpha into the swapchain (mesh.frag alpha-out for ALPHAOP-disabled chains), then delete the
  blend rewrite. Until then do NOT map 7/8 through TranslateBlend unchanged for water draws.

## Addendum (beta14, round 10) - water fallback sheet removal
- Field water "still missing on some maps" had a second identity: the warm-brown sheet = W3DWater's
  FALLBACK shroud second-pass draws, taken on the port because m_trapezoidWaterPixelShader is null
  (pixel-shader branch never set up). It repaints the whole plane opaque over the pass-1 feather,
  map-dependent only via which fallback branch runs. Fix = delete all four fallback sites (trap else,
  drawSea sea-patch loop, flat-river block, mesh-water if/else); the mesh-water site is a single body
  draw so its fix is forcing the unshrouded else-draw form, NOT deleting the draw. Pass-1-only equals
  mode-h semantics (proven teal: TL 3.5%, SR 19.5%); BB's brown sheet ~= retail muddy river.

## Addendum (beta14, round 11) - water from-scratch opaque rebuild
- All three water body sites now share ONE state: stage0 white 1x1 MODULATE, PRELIT_DIFFUSE vertex
  color, _PresetOpaqueShader, no alpha-op, no DESTALPHA feather, ZWRITE+LEQUAL. The wave-grid site
  lost the raw m_pDev WATER_MESH_FVF submission (dynamic VB/IB instead, same m_meshData heights);
  the pv-shader sea site lost reflection RT + custom VS (clipped trapezoid instead). Port-invented
  stage classes were the silent-drop suspect for the field 'no water anywhere' (see AUDIT 8).
- Detail pass (flow/noise/sparkle) is deferred to beta15 and must be built on existing proven
  variant classes only; do NOT re-add CAMERASPACEPOSITION noise texgen or the op-7 ADDPRODUCT stage.

## Addendum (beta14, round 12) - beta14f shadow-map + water-class fix
- Shadow RT: FIXED 2048x2048 (was swap-sized: non-square 19/31u texels at field res vs 23u bias
  = global acne = the 'everything is now dark' report). bias 0.0011 NDC = ~23u = ~1.9 texels.
- Water now NEVER draws in the shadow light pass (opaque Z-writer as caster = global occluder);
  guard = file-scope extern "C" Vulkan_NativeShadow_Active() at WaterRenderObjClass::Render top.
- Water body class = verbatim replicate of the field-proven shroud-fallback pass (stage0 shroud
  texture modulate via W3DShaderManager + PRELIT_DIFFUSE + _PresetAlphaShader + ALPHAOP ADD +
  TCI 0 + stages1-3 off + LEQUAL + SRCALPHA blend, VB alpha 0xff). White-1x1/opaque-forced was
  still a never-exercised variant = silent drop. Green-coverage probe confirmed the replicated
  class paints real water areas locally (SR 23px / TL 26px at 256x192 scale).

### ROUND 13 addendum (beta14g) - shadow map emptiness was VIEWPORT, not range/bias

VIZ proved beta14f's map was empty AT RECEIVER UV (dc=1 everywhere, one content patch
= the window-rect corner of the 2048 FB). One-line fix at the four draw sites: during
g_nativeShadowActive the viewport/scissor come from the target extent, never CachedViewport.
Water body class (beta14f) UNCHANGED - it now visibly receives the cast shadows (SR floor
59,61,88: sheet bluer+darker than 48,54,66 with an empty map) - 'water missing' reads were
the FLAT sheet next to the bed, cured by the shadows arriving, not by a new water class.

### ROUND 14 addendum (beta14h) - band is WORLD space; water modulate was BLACK for trap UVs
- Shadow band: g_shadowBandNdc = 107.0f * fabsf(CachedProjection[2][2]) (WBND census: oscale
  -4.7e-5 x 21084u rig = 1.0 -> WW3D ortho z-scale measures exactly 1/(f-n) here); 107u = the
  beta14d field-proven penumbra (0.03 NDC on a 3574u rig). The 2/|scale| 'range' detour DOUBLED
  it (53u = over-black hard fans). Both receivers take band from UBO shadowParams[3]; PCF rad
  3.0 texels; darkening 0.40. Light pass UNCHANGED from round 13 (WSLP census ~= 2.5k submissions/
  frame proves the shell map + meshes cast every frame - 'use the shell map' needs no re-arch).
- Water sheet z was NEVER wrong (WBGRID: BB minz=10<41, TL minz=2.2/maxz=157 vs wz=137, SR
  minz=0<7 within +-700u of centroids; +Z=down so bed 50 is 9u UNDER water 41 = shallow flood).
  The sheet was invisible because the shroud MODULATE reads BLACK for trap quads (VB uv0 =
  world/waterFactor ~= 6.6, CLAMP -> unexplored edge texel). Shipped: same stage config +
  manager state (KEY unchanged), texture re-bound to the white 1x1, VB diffuse forced 0x004070C0.
- Variant trap: D3DRS_ZFUNC edits create unexercised pipeline variants (ALWAYS broke the clipmap
  = black void on lavapipe); NEVER inject render states outside the proven sequence to 'prove'
  a draw - lift the geometry instead.

- ROUND 15 (beta14i, 2026-10-28): 'blue plane all over the map' = the +24 WDBG NUMERIC lift shipped
  (numeric debug is invisible to probe-strings - lesson 12). Removed; water = map z exactly.
  Coverage census: e43 flood 191,365 px -> e44 149/172/29,834 px basin-only. Grid (lvl=7, 2000x2000)
  needs NO height gate - under the whole map + proven depth test handles it. Shadows untouched
  (107u*|oscale| band / 0.40 / rad 3.0 = sBB parity mean delta (16,11,7), AE-fuzz10 1.0%).

- ROUND 16 (beta14j, 2026-10-28): shadow visibility tune = world band 64u (band_ndc = 64*|oscale|)
  + darkening 0.45; PROVEN in the shell-menu background venue (menu16 vs pre-tune: grove mean
  (33,26,19), units (29,22,16) = fans read under minarets/palms/tanks). Water untouched (coverage
  census identical: BB 152 / TL 172 / SR 29,528 px basin-only). Lesson 13: tune looks in stable
  menu/caster scenes, not spawn views.

- ROUND 17 (beta14k, 2026-10-28): unit blob shadows (round-17 fix class): bias root cause pinned
  (23.2u world bias > 10-20u unit height = units NEVER register inShade despite WSLP submitting
  them; plus 12u/texel sub-footprint). CPU blobs in W3DWater renderWater: GeometryInfo height
  gate (<=22u = where RT can't work; trees 40-60u skipped, lesson 14 no-blob-stacking), footprint
  radii, proven flat-water alpha class + ALPHAOP MODULATE, one batched 12-gon x2 fans draw.
  Menu venue units-deck delta (21,16,11) vs r16 no-blob; e49 trio exit 0/0 exc/probe 0; tree crop
  e49-vs-e45 (6,4,2) = drift only = fans untouched. Fingerprint 21,939,430B sha 0c0db851...5798.

- ROUND 18 (beta14l, 2026-10-28, SHIPPED build64 21,940,454B sha 0c45de9f...a6a8e3): soft CAST
  shadows, CPU side (RT pipeline STILL untouched - bias fix re-rejected: 23.2u bias > every drawable
  height, shrinking it = beta14e acne class). 18a: swept ellipse (footprint (+) sun-ray Minkowski
  sum, semis lifted by mesh sphere r: mjF=max(mj,r*.72) mnF=max(mn,r*.5); xfBlob heading) rendered as
  center disc + 2 annuli with vertex-alpha ramp 0x48>0x36>0x1e>0 (rasterizer-continuous gradient,
  one batched draw, flat-water class + MODULATE unchanged); coarse r 3.5..46 healed the r28..45
  vehicle class (got NEITHER blob NOR RT in r17 = the real some-units bug). 18b (the skybox
  symptom): kindOf gate, blob iff template has VEHICLE/INFANTRY/.../CRATE/BRIDGE bits (census named
  TreeDogwood8/TreePalm1/2 clones with lying hh=15 at r31-39 stacking on their RT fans = cap-full
  stain wash + cap starvation kept 128/128); BLOB_MAX 256. Venues: menu m24 = soft ellipses under
  every unit / clean plaza / fans intact; e54 trio EXIT_WINE=0 exceptions=0 x3; probe-strings census
  0 (AUDIT lesson 15: templates honest where GeometryInfo lies).


- ROUND 19 (beta14m, 2026-10-28, SHIPPED build66 21,935,320B sha 01e83bc6...92bb): RT SHADOWS REVIVED
  + blob pass DELETED (user: "don't put blob shadows... nice pretty projected soft shadows on
  buildings, trees, units"; "GPU rig almost worked at 7a10eb1" = docs commit, working code 92b03e6
  beta14d). Rig back to small follow-cam: SHADOW_DIST 2048 / far 4,480 / half-extent 2,560 (2.5u/texel
  pinned 2048^2) / bias literal KEPT 0.0011f = 4.9 WORLD u = 2 texels: clears kernel self-spread 3.3u,
  infantry (7-8u) + tanks (13-18u) CLEAR it = lesson-14 numbers FLIPPED (x4 rig = 23.2u = WHY the blobs
  existed; deleting the rig growth makes blobs unnecessary). Band 64u->8u world (unit class = whole
  caster range; 64u wash confirmed in venue pass 1 at 12u). Receiver edge fade replaces pop-in
  (smoothstep(0.7,1.0,max|lpc.xy|) both shaders). terrain.frag shadowTint() final return RESTORED
  (d7fcf78 collateral deleted it = UB = lesson 16). Water class NOT regressed: full-frame teal "collapse"
  was minimap + camera-phase contamination (lesson 17); clean game-view crop census comparable BB 61/45
  TL 42/60 SR 224-485. Venues/soaks: menu26 fans under every unit class, eNNBB/eNNTL/eNN_SR green x3,
  probe census 0. Bridges cast RT fans natively now (blob gone) - eighth ask (small-bridges map +
  screenshot) remains open pending user.

- ROUND 20 (beta14n, 2026-10-29, SHIPPED build72 21,935,320B sha e14d5eed...4aeed, probe census 0):
  ROTATE-SHADOW-LOSS + GIANT-SQUARE class root-caused: box footprint centered on EYE XY and the along-sun
  eye anchor put coverage ~975u toward sunXY of camCell; unbounded fans truncated at the suv-guard SQUARE
  and the box-normalized fade PAINTED that straight edge = dark square sweeping with orbit (repro rot_z1-19
  Tournament Lake zoom peak). Cleared-depth theory DISPROVEN (LOAD_OP_CLEAR inherited, yaw sweep clean of
  ghosts, clear patch never shipped) and capture-drift theory DISPROVEN (LMVP probe zero drift). FIX:
  SYMMETRIC anchor eye=(camXY, sunZ x 2048), halfAtDist 256->512 (half 5,120u), far 13,312 (range 13,302u;
  -sun shadow landings covered; sun-side z-cut radial ~3,326u = past max view ~2,400u); map 2048^2->4096^2
  FIXED SQUARE keeps 2.5 u/texel (lesson-14 tension doubled era: coverage MUST double map or bias loses
  unit deltas = fan-loss class; beta14f non-square guard kept); bias literal 0.0011f->0.0004f = 5.3 world u
  = ~2 texels (kernel self-spread 3.3u cleared, infantry 7-8u clears, tanks saturate; 8u auto band +
  0.45 darken KEPT); receivers mesh.frag + terrain.frag box-normalized fade -> RADIAL cam-distance fade
  smoothstep(2500,3900,|world.xy - miscParams.yz|) (miscParams.yz = game cam world XY = fog source, NO
  layout change; end 3,900u inside XY 5,120u and z-cut limits => no hard edge can pop in view, orbit-
  invariant; suv/z guards kept). Venue rotate.sh sweep+zoom-osc to 1.8: TL/SR/BB GREEN (square gone at
  peaks, fans live, match-frame means within 7 pts of r19, r20 slightly brighter zoomed-out). Soaks eNOBB/
  eNNTL/eNO_SR green x3 EXIT_WINE=0, 0 exceptions; water crop teal TL 282k BB 157k SR 581 (no r10 class).
