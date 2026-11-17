# M4c 素材 contents 化方案

- 状态：**步骤 A、B 已完成；步骤 C 经用户决定取消**
- 目标：把散落的源素材收进 `contents/`
- 硬约束：**移动后 ROM 必须仍然与基线逐位一致**（`make check`）

---

## 0. 执行结果（2026-11-17）

| 步骤 | 内容 | 状态 |
|---|---|---|
| A | `graphics/` 25 个目录 → `contents/` | ✅ 完成，`make check` OK |
| B | `sound/` 4 个子目录 + 6 个 `.s` → `contents/sounds/` | ✅ 完成，`make check` OK |
| C | 1,293 处 `baserom.gba` incbin 外置 | ❌ **用户决定不做** |

**步骤 C 取消的理由**：`baserom.gba` 作为构建输入是 decomp 工程常态（fe6-rebuild
至今仍保留 2,476 处 `incbin "fe6-base.gba"`）。强行外置只是把 ROM 依赖换成
文件依赖，数据仍是二进制、不可编辑；真正的收益要等 M4b 按数据类型重写时才实现。
因此这一步留空，等 M4b 数据整理时再看哪些值得转成可编辑格式。

### 实际落地的改动

| 类别 | 内容 |
|---|---|
| 目录移动 | `graphics/*` → `contents/*`（25 个），`sound/*` → `contents/sounds/*` |
| 路径改写 | 24 个 `data/*.s` 的 `incbin`、`graphics_file_rules.mk`、`banim_img_rules.mk`、`linker_script_banim.txt`（671 处）、`sym_iwram.txt`、`linker_script_sound.txt`、`Makefile` |
| 新增 | `tools/refactor/move_graphics.py`（搬目录 + 改写引用的通用脚本） |

**两个容易漏的地方**（都是构建失败才暴露的）：

1. **`linker_script_banim.txt` 里写死了 671 处 `graphics/banim/*.4bpp.lz`**。
   它不在 M4a 定的四个链接脚本里，但同样影响链接。
2. **`.gitignore` 的 `graphics/bg/*.bin`、`graphics/map/layout/*.bin`、
   `graphics/statscreen/*.bin`、`sound/direct_sound_samples/*.bin`、
   `sound/songs/midi/*.s`** 全是旧路径，移动后 400+ 生成物变成未跟踪。

---

## 1. 现状体检（移动前）

| 项 | 数量 |
|---|---|
| `graphics/` 顶层目录 | 25 个（`banim` `portrait` `item_icon` `unit_icon` `op_anim` `cg` `bg` `map` `chap_title` …） |
| `graphics/` PNG | 2,330 |
| `sound/` 子目录 | 4 个（`songs` 1777 文件、`direct_sound_samples` 878、`voicegroups` 186、`programmable_wave_samples` 11） |
| `.s` 里 `incbin "graphics/…"` | 24 个文件、约 3,400 处 |
| `.s` 里 `incbin "sound/…"` | 2 个文件 |
| `.s` 里 `incbin "baserom.gba"` | **34 个文件、1,293 处**（尚未外置的原始数据） |
| 总 `incbin` 行 | 6,031 |
| 规则文件 | `graphics_file_rules.mk` 204 行、`songs.mk` 1767 行、`json_data_rules.mk` 30 行、`graphics/banim/assets/img/banim_img_rules.mk` 38 行 |

---

## 2. 目标布局

对齐 fe6-rebuild 的 `contents/`：

```
contents/
├── banims/           战斗动画素材（graphics/banim 的 878 PNG + OAM/TSA）
├── portraits/        立绘（323 PNG + 249 辅助文件）
├── item_icon/        道具图标（224）
├── unit_icon/        单位图标（214）
├── op_anim/          OP 动画（207）
├── cg/               过场 CG（100）
├── bg/               战斗/地图背景（47 + 25）
├── btl_bg/           战斗背景（28 + 45）
├── map/              地图地块与布局（89 + 205）
├── chap_title/       章节标题（65）
├── efx/              特效（efxbattle 16 / efxdragon 64 / efxmagic 13 / efxskill 16）
├── ui/               界面（statscreen / minimap / player_interface / sysutils / titlescreen / bar_fx）
├── fonts/            字体（debug_font.png 等）
└── sounds/           音频
    ├── songs/        MIDI + MML
    ├── samples/      direct_sound_samples
    └── voicegroups/  音源组
```

**判断依据**：`graphics/` 下现有的 25 个目录本身就是按用途分的，说明上游已经想清楚了分类。
M4c 不再重新发明分类，只做「搬进 `contents/` + 补 `README` 说明每类是什么」。

---

## 3. 分三步，每步独立可验证

### 步骤 A：`graphics/` → `contents/`（低风险）

改的东西：

1. `git mv graphics/<dir> contents/<dir>`（25 个目录）
2. 改写 **24 个 `.s` 文件**里的 `incbin "graphics/…"` 路径
3. 改写 `graphics_file_rules.mk`（61 处 `graphics/`）、`banim_img_rules.mk`、
   `Makefile` 里的 `MAP_LAYOUT_SUBDIR := graphics/map/layout` 等
4. 改写 `tools/scripts/tsa_png.py` 等脚本里的 `graphics/` 常量

**验收**：`make clean && make -j16 && make check` → OK

风险点：`graphics/` 里同时存在「源素材」（PNG/PAL/TSA）和「生成的中间产物」
（`.4bpp` `.lz` `.bin`），后者已被 `.gitignore` 忽略，`git mv` 只搬源文件，
但**目录里会留下孤儿产物**，需要按 M4a 的办法清理（旁边没有源文件就删）。

### 步骤 B：`sound/` → `contents/sounds/`（低风险）

1. `git mv sound/songs contents/sounds/songs`、`direct_sound_samples` → `samples`、
   `voicegroups` → `voicegroups`
2. 改写 `songs.mk` 的 `$(MID_SUBDIR)` 变量（否则 1767 条规则全部指向旧路径）
3. 改写 `Makefile` 的 `SOUND_S_FILES` / `MID_SUBDIR` / `SAMPLE_SUBDIR`
4. 改写 2 个 `.s` 里的 `incbin "sound/…"`
5. `sound/` 下还有 6 个 `.s`（`song_table.s` 等），一并搬或留在原地——见 D3

**验收**：同上。

### 步骤 C：`baserom.gba` 的 1,293 处 incbin 外置 ❌ 已取消

这是让工程最终不依赖 ROM 的关键，也是工作量最大的一步。

现状：34 个 `.s` 文件里共 1,293 处 `.incbin "baserom.gba", 0xXXXX, 0xNNNN`。
热点：

| 文件 | incbin 数 |
|---|---|
| `data/data_9A31F8.s` | 423 |
| `data/banim-ekrdragonfx.s` | 180 |
| `data/data_A3E4D4.s` | 140 |
| `data/data_59E8E0.s` | 68 |
| `data/banim-efxlvupfx.s` | 52 |

做法（两种，按收益选）：

- **C1 原样外置**：把每个 incbin 段导出成 `contents/<类别>/<符号名>.bin`，
  `.s` 改成 `.incbin "contents/.../xxx.bin"`。
  - 优点：机械、可脚本化、ROM 必然一致
  - 缺点：只是把 ROM 依赖换成文件依赖，数据仍是二进制，**不可编辑**
- **C2 转成可编辑格式**：能识别的转 PNG（图）/ C 数组（表）/ MIDI（音），
  识别不了的才保留 `.bin`
  - 优点：这是「可自由定制」的真正目标
  - 缺点：慢，且每一类都要单独的转换与校验

**建议**：按文件分批推进，先做 C1 让 `baserom.gba` 退出构建，再在后续阶段
（M4b 数据整理）逐类做 C2。

**验收**：`make check` → OK，且 `git grep -c 'incbin "baserom.gba"'` 归零。

---

## 4. 执行顺序与批次

| 批 | 内容 | 预计改动 |
|---|---|---|
| 1 | `graphics/bar_fx` `cg` `chap_title` `efx*` `ui` 类（小目录先试） | ~150 文件 |
| 2 | `graphics/banim`（最大，878 PNG + 1414 辅助） | ~2,300 文件 |
| 3 | `graphics/portrait` `item_icon` `unit_icon` | ~1,000 文件 |
| 4 | `graphics/map` `bg` `btl_bg` `op_anim` | ~500 文件 |
| 5 | `sound/` 整体 | ~2,800 文件 |
| 6 | `baserom` incbin：小文件（<20 处） | ~600 处 |
| 7 | `baserom` incbin：热点文件 | ~700 处 |

每批结束跑一次 `make check`。

---

## 5. 待确认项

| # | 决策 | 我的建议 |
|---|---|---|
| D1 | `contents/` 下的分类名是否沿用 `graphics/` 现有目录名 | **沿用**，少一层心智负担；`banim` 保留复数 `banims` 以对齐 fe6 |
| D2 | `graphics/` 目录名是否保留（做成软链或空壳） | **不保留**，直接改成 `contents/`，少一层间接 |
| D3 | `sound/` 下 6 个 `.s`（`song_table.s`、`keysplit_tables.s` 等）去哪 | 搬进 `contents/sounds/`，与数据同级 |
| D4 | baserom 外置先做 C1（原样 bin）还是直接 C2（转可编辑） | **先 C1**，让 ROM 依赖归零；C2 留给 M4b 数据整理 |
| D5 | 生成的中间产物（`.4bpp` `.lz`）放哪 | 与源文件同目录（现状），`.gitignore` 已覆盖 |
| D6 | `contents/` 是否需要 `README.md` 说明每类 | **需要**，每个顶层目录一个简短说明 |

---

## 6. 回复方式

- 「按方案做，D1–D6 按建议」
- 「D4 直接做 C2」（会慢很多，但数据立刻可编辑）
- 「先只做步骤 A（graphics），sound 和 baserom 以后再说」
