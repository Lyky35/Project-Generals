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
1. [ ] M1: HeightTex+ColorTex builders + upload, log stats (no draw change).
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
3. [ ] M4: delete tile subsystem (updateCenter/calcVis/drawOrg streaming,
       m_vertexBufferTiles) once clipmap is the default - NEXT SESSION.
4. [ ] M5: ring LOD - only if maps outgrow the250k-cell full-grid guard.
3. [ ] M3: shroud + alpha passes on clipmap; visual parity shell + mission.
4. [ ] M4: delete tile/window subsystem; full-map; perf pass.
5. [ ] M5: ring seams/LOD polish.

Status: branch created, design locked; code starts at M1.
