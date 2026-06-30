# Personal Game Engine — Architecture & Pipeline
### A Quake-inspired client-server FPS engine in C++

---

## 1. Design Philosophy

Three rules govern every decision below:

1. **Server is authoritative.** The server owns truth. Clients never decide outcomes — they predict, display, and send intent.
2. **Hard separation of concerns.** Rendering never touches physics. Physics never touches the network. Each layer talks to its neighbor through a narrow, well-defined interface and nothing else.
3. **Build the spine before the muscle.** Get a trivial end-to-end loop (input → server → network → render) working before adding any single feature in depth.

---

## 2. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                            YOUR GAME                              │
│              (entities, weapons, maps, rules)                     │
└───────────────────────────┬─────────────────────────────────────┘
                            │
        ┌───────────────────┴───────────────────┐
        │                                         │
┌───────▼────────┐                       ┌────────▼────────┐
│     CLIENT      │◄──── network ────────►│      SERVER      │
│                  │      protocol         │                  │
│ • Input capture  │                       │ • Simulation     │
│ • Prediction     │                       │ • Authority      │
│ • Rendering      │                       │ • Game rules     │
│ • Audio          │                       │ • Snapshot gen   │
│ • UI             │                       │                  │
└───────┬──────────┘                       └────────┬─────────┘
        │                                            │
┌───────▼──────────────────────────────────────────▼─────────┐
│                    SHARED CORE / FOUNDATION                   │
│  Math (vec/mat) • Memory • ECS • Collision • Asset system     │
└───────────────────────────┬───────────────────────────────────┘
                            │
┌───────────────────────────▼───────────────────────────────────┐
│                    PLATFORM ABSTRACTION                         │
│         Windowing • Input devices • File I/O • Sockets          │
└───────────────────────────────────────────────────────────────┘
```

**Key principle inherited from Quake:** client and server are *separate compilation units that can run in the same process or different machines*. Even in single-player, you run a local server and connect to it via loopback. This forces the separation to be real, not aspirational.

---

## 3. Module Breakdown

| Module | Responsibility | Depends on |
|---|---|---|
| `core/` | Math types, memory allocators, containers, logging | nothing |
| `platform/` | Window creation, input polling, file I/O, sockets | core |
| `net/` | Packet structures, serialization, reliability layer | core, platform |
| `ecs/` | Entities, components, systems | core |
| `assets/` | Loading, caching, hot-reload of textures/models/sounds | core, platform |
| `physics/` | Collision detection, movement resolution | core, ecs |
| `server/` | Simulation loop, authoritative game state, snapshot generation | ecs, physics, net |
| `client/` | Prediction, reconciliation, input capture | net, ecs |
| `render/` | OpenGL abstraction, draw calls, camera | core, assets |
| `audio/` | Sound playback, mixing | core, assets |
| `game/` | Actual gameplay code — weapons, movement rules, maps | everything above |

---

## 4. The Frame Pipeline (Client)

This is what happens every single frame on the client, in order:

```
1. POLL INPUT
   → Read keyboard/mouse/gamepad state
   → Package into a "usercmd" (move forward, turn, fire, jump)

2. SEND COMMAND
   → Usercmd is timestamped and sent to server (or local server if SP)
   → Client does NOT wait for a response

3. CLIENT-SIDE PREDICTION
   → Client immediately simulates its own movement locally using the
     same movement code the server uses, so the player feels no lag
   → Result is tentative — it WILL be corrected if wrong

4. RECEIVE SNAPSHOT
   → Server's last authoritative snapshot arrives (may be several
     commands "in the past" due to latency)
   → Client reconciles: if its prediction diverges from the
     server's truth, snap/interpolate back into agreement

5. INTERPOLATE OTHER ENTITIES
   → Other players/objects are NOT predicted — they're interpolated
     between the last two received snapshots for smoothness

6. UPDATE CAMERA
   → Apply view angles, recoil, screen shake

7. RENDER
   → Cull → batch → draw opaque → draw transparent → UI overlay

8. PLAY AUDIO
   → Update 3D positional audio sources based on new positions
```

## 5. The Frame Pipeline (Server)

The server runs on a **fixed tick rate**, independent of client framerate:

```
1. RECEIVE COMMANDS
   → Pull all buffered usercmds from each connected client

2. RUN SIMULATION
   → For each tick: apply commands → run movement/physics →
     resolve collisions → run game logic (damage, pickups, etc.)

3. BUILD SNAPSHOT
   → Capture the full world state (or delta from last ack'd
     snapshot — only send what changed)

4. BROADCAST
   → Send snapshot to all clients, each potentially delta-compressed
     relative to that specific client's last acknowledged snapshot
```

**Tick rate decision:** start at 60Hz fixed. You can move toward 128Hz (Valorant-style) or sub-tick (CS2-style) later — but get 60Hz rock solid first.

---

## 6. Network Protocol Design

```
CLIENT → SERVER:  Usercmd
┌────────────┬──────────┬───────────┬────────┐
│ tick_sent  │ move_xyz │ view_angle│ buttons│
└────────────┴──────────┴───────────┴────────┘

SERVER → CLIENT:  Snapshot
┌────────────┬──────────────────┬───────────────────┐
│ tick_num   │ last_acked_cmd   │ entity_deltas[]    │
└────────────┴──────────────────┴───────────────────┘
```

- Use **UDP**, not TCP — you want the latest state, not guaranteed delivery of old state.
- Use **ENet** (a library) to add selective reliability on top of raw UDP rather than writing your own reliability layer from scratch.
- **Delta compression**: only send fields that changed since the client's last acknowledged snapshot. This is the single biggest bandwidth win and the core trick in Quake's protocol.

---

## 7. Data Flow Through the ECS

```
        COMPONENTS                    SYSTEMS
   ┌──────────────────┐        ┌──────────────────────┐
   │ Transform         │        │ MovementSystem        │
   │ Velocity          │◄──────►│ CollisionSystem       │
   │ Health            │        │ DamageSystem          │
   │ Renderable        │        │ RenderSystem          │
   │ Collider          │        │ NetworkSyncSystem     │
   └──────────────────┘        └──────────────────────┘
            ▲                              │
            │         Entity = just an ID  │
            └──────────────────────────────┘
```

Entities are IDs. Components are pure data, stored in contiguous arrays per type. Systems iterate over entities that have the required component combination. Server and client share the *same* ECS component definitions — this is what makes prediction possible, since the client runs the exact same movement system the server does.

---

## 8. Build Order (Milestones)

Build in this order — each milestone should produce something runnable:

| # | Milestone | Proves |
|---|---|---|
| 1 | Window opens, clears to a color, closes cleanly | Platform layer works |
| 2 | A triangle renders via OpenGL | Render pipeline basics |
| 3 | A textured cube moves via keyboard input | Input + transform + camera |
| 4 | Server and client run in the same process, exchange dummy packets over loopback UDP | Network plumbing |
| 5 | Player position is server-authoritative; client predicts and reconciles | The core architectural lesson |
| 6 | Second "fake" player entity interpolates on screen from snapshots | Entity interpolation |
| 7 | AABB collision against static geometry | Physics basics |
| 8 | Load a model + texture from disk via asset system | Asset pipeline |
| 9 | Two real machines (or two processes over real sockets) connect and play | True client-server separation |
| 10 | Basic gameplay: shooting, health, respawn | Game layer on top of the engine |

Do not skip ahead. Milestone 5 is the one that took Carmack's team the most design iteration — budget real time for it.

---

## 9. Recommended Libraries (so you're not reinventing everything)

| Need | Library | Why |
|---|---|---|
| Window/input | SDL2 | Cross-platform, minimal abstraction |
| OpenGL function loading | GLAD | Standard, no overhead |
| Math | GLM | Matches GLSL conventions |
| Networking transport | ENet | UDP + selective reliability, used in many indie engines |
| Image loading | stb_image | Single header, trivial to integrate |
| Model loading | tinyobjloader / cgltf | Simple formats first |
| Audio | miniaudio | Single header, covers most needs |

---

## 10. What to Defer (don't build yet)

- BSP trees / spatial partitioning — AABB grid is enough until you have real maps
- Sub-tick networking — get fixed-tick rock solid first
- A scripting layer (Lua) — hardcode gameplay in C++ until the engine itself is stable
- A scene editor — command-line level loading is fine for a long time
- Advanced rendering (PBR, shadows, GI) — forward renderer with one light is enough to validate the pipeline

---

## 11. The One Question to Ask at Every Step

*"Where would this live in Quake's source — client, server, or shared core — and why?"*

If you can't answer that for a piece of code you're about to write, you haven't separated concerns enough yet.
