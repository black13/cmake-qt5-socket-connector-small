# NodeGraph XML System

Authoritative description of the XML format NodeGraph reads and writes, as of
2026-07-22 (post-remediation; matches `ADVISORY.md` Entry 004 state).
Everything below is verified against the code and against real files written
by the app (e.g. `logs/smoke_roundtrip.xml`).

---

## 1. Who writes and who reads XML

| Direction | Component | Purpose |
|---|---|---|
| Write | `Window::saveGraph` / `Graph::saveToFile` / `Graph::toXml` | Manual save (Ctrl+S) and script-facing save |
| Write | `XmlAutosaveObserver` | `autosave.xml` in the working directory, 1200 ms after the last change |
| Write | `undo_commands` (in-memory) | XML snapshots of single nodes/edges for undo/redo — never persisted |
| Write | `GraphFactory::m_xmlDocument` | **Scratch pad only**, not authoritative; ignore its contents |
| Read  | `GraphFactory::loadFromXmlFile` | The **only** real reader (File→Open, CLI file arg, `graph.loadFromFile`) |

All writers serialize from the live scene via `Node::write` / `Edge::write`;
all loads go through one factory path with a three-phase all-or-nothing design.

## 2. Document shape

```xml
<?xml version="1.0" encoding="UTF-8"?>
<graph version="1.0">
  <node id="..." x="..." y="..." type="..." inputs="..." outputs="...">
    <script language="javascript">...</script>   <!-- optional -->
    <payload>...</payload>                       <!-- optional -->
  </node>
  <edge id="..." fromNode="..." toNode="..." fromSocketIndex="..." toSocketIndex="..."/>
</graph>
```

- Root: `<graph>` with `version="1.0"` (the loader does not currently enforce
  the version). Nodes and edges are **direct children of the root** — the
  loader does not accept any nested `<connections>` wrapper.
- `xmlns` appears only on the factory's internal scratch document, never in
  saved files.

## 3. `<node>` attributes

| Attribute | Meaning |
|---|---|
| `id` | UUID, usually written **without braces**; both `{uuid}` and bare forms are accepted on read |
| `x`, `y` | Scene position (real numbers) |
| `type` | One of `SOURCE`, `SINK`, `SPLIT`, `MERGE`, `TRANSFORM`, `SCRIPT` |
| `inputs` | Input socket count (recreates sockets on load) |
| `outputs` | Output socket count |

Every node is a `ScriptedNode`, so two optional children may follow:

- **`<script language="javascript">`** — the node's JavaScript source, written
  as raw text; libxml2 escapes `<`, `&`, `>` on write. Templates embed it as
  CDATA; both forms read back identically.
- **`<payload>`** — the node's payload map, JSON-encoded. Exotic QVariant
  types (e.g. `QUuid`, `QPointF`) do not survive JSON encoding; strings,
  numbers, bools, lists and maps do.

## 4. `<edge>` attributes

| Attribute | Meaning |
|---|---|
| `id` | Edge UUID (bare or braced accepted) |
| `fromNode` | Source **node** UUID |
| `toNode` | Destination **node** UUID |
| `fromSocketIndex` | Output socket index — **global**, see §5 |
| `toSocketIndex` | Input socket index — **global**, see §5 |

The reader also accepts legacy aliases: `from`/`to` for the node ids and
`from-socket`/`to-socket` for the indices (`Edge::read`).

## 5. Socket indexing — the part that trips everyone up

**The XML stores GLOBAL per-node indices: inputs first (0..n-1), then
outputs (n..n+m-1).** The runtime connect API (`graph.connectNodes`,
ghost-edge drags) uses **per-role** indices (first output = 0).

| Node type | Sockets | XML index of first input | XML index of first output |
|---|---|---|---|
| SOURCE | 0 in / 1 out | — | 0 |
| SINK | 1 in / 0 out | 0 | — |
| TRANSFORM / SCRIPT | 1 in / 1 out | 0 | **1** |
| MERGE | 2 in / 1 out | 0 | **2** |
| SPLIT | 1 in / 2 out | 0 | **1** |

Example from a real saved file: an edge leaving a TRANSFORM's output is
written `fromSocketIndex="1"`, because that node type's input occupies index 0.

Consequence for hand-authored files: use the global scheme above, or your
edges will resolve to the wrong sockets (or fail validation).

## 6. The load pipeline (`GraphFactory::loadFromXmlFile`)

1. **Phase 1 — attribute validation.** Every `<node>`/`<edge>` must carry its
   required attributes; anything else fails the whole file. (Note: duplicate
   UUIDs are *not* rejected today — deferred, see `ADVISORY.md` 4.4.)
2. **Phase 2 — object creation.** Nodes are created through the same
   `Node::read` path used for templates; edges store their connection data
   unresolved. Failure anywhere deletes everything created so far.
3. **Phase 3 — connection validation & resolution.** Duplicate socket usage is
   rejected with direction-prefixed keys (`out:<nodeId>:<index>` /
   `in:<nodeId>:<index>`), so an input and an output on the same node can
   never collide (fix A1). Edges then resolve their socket pointers.
   Unresolvable edges are currently **kept** and load still reports success
   (deferred, `ADVISORY.md` 4.4). Runtime policy blocks self-loops and
   double-occupied sockets; load-time validation relies on Phase 3 for the
   latter and does not check self-loops.

The whole load runs under a `GraphSubject::BatchGuard` (observer
notifications muted; one `onBatchEnded` flush at the end). A script calling
`graph.loadFromFile` gets a **replace** (the scene is cleared first), never a
merge (fix C6).

## 7. autosave.xml

Same format as a manual save. Behavior:

- Written 1200 ms after the last observed change (debounced), and once after
  each batch ends (`onBatchEnded` catch-up).
- On shutdown the scene is cleared for teardown safety; the observer is
  disabled first and also refuses to schedule saves once shutdown begins — so
  a clean exit **keeps** the last autosave (fix A2).
- Caveat (deferred, `ADVISORY.md` 4.12): the file is written through the
  locale codec, not UTF-8. Content with non-ASCII characters (scripts or
  labels with accents/CJK/etc.) can be corrupted on Windows until that fix
  lands.

## 8. Rules for hand-written files

1. Root `<graph>`, nodes and edges as direct children.
2. UUIDs bare or braced — both fine. Keep them unique (duplicates are not
   rejected and create zombie items — deferred).
3. Socket indices **global** (§5).
4. One edge per socket — Phase 3 rejects duplicates by direction.
5. No self-loops (runtime refuses them; load doesn't check — don't rely on it).
6. Scripts: raw JS text inside `<script>` is fine; `<`, `&`, `>` must be
   XML-escaped (libxml2 does this for app-written files).
