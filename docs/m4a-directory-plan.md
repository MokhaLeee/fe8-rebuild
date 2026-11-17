# M4a 目录重组方案（待确认）

- 状态：**方案待确认，尚未移动任何文件**
- 目标：把平铺在 `src/` 的 359 个 `.c` 按 FE8 自身模块域分到子目录
- 硬约束：**移动后 ROM 必须仍然与基线逐位一致**（`make check`）

---

## 1. 为什么不能直接 `git mv`

`ldscript.txt` 是**对象文件粒度**的布局脚本，每个对象按名字和出现顺序钉在固定地址上：

```
src/main.o(.text);
src/rng.o(.text);
src/hardware.o(.text);
...
```

- 共 **911 处**对象引用、**424** 个唯一目标文件
- **顺序即地址**：改顺序 → 地址变 → ROM 不一致
- 还有两个被 `INCLUDE` 进来的链接脚本同样引用对象路径：
  - `linker_script_sound.txt`（第 1 行就是 `src/m4a_tables.o(.rodata);`）
  - `linker_script_banim.txt`

所以移动文件必须**同步改写三个链接脚本里的路径，同时保持行序不变**。

---

## 2. 目标目录树

```
src/
├── Boot/          15  启动与运行时：main / crt0 / irq / hardware / rng / time
├── Core/              引擎核心（不含战斗与界面）
│   ├── Map/       44  地图、单位移动、光标、地形、AI 行动
│   ├── Unit/      10  单位、道具使用、转职、怪物生成
│   ├── Save/       9  存档读写、存档菜单数据
│   ├── Prep/      17  准备画面
│   ├── WorldMap/  26  世界地图
│   ├── Sio/       20  联机
│   ├── Events/    22  事件脚本、事件表
│   └── Ending/     4  结局、通关评价
├── Battle/        87  战斗、战斗动画（banim / ekr / efx / cp / bb）
├── MapAnim/       20  地图动画（mapanim / 各类地图特效）
├── UI/            40  菜单、状态画面、章节标题、OP 动画
├── Common/        12  通用：bmlib / fontgrp / icon / msg / m4a / soundwrapper
├── Data/          33  纯数据表
└── Debug/          3  decomp 遗留（code_* / fe3_dummy）
```

合计 362 —— 比实际的 359 多 3，因为 `spellassoc-data`、`bmreliance-data`、
`banim_data` 这几个在「跟模块」与「进 Data」之间有重叠计数；归位后按 359 一文件一目录。

### 2.1 与 fe6-rebuild 的差异

fe6 用 `Core/`、`Common/`、`SkillEffects/`、`Test/`。本仓库**不照抄**：

- FE8 没有「技能系统」，`SkillEffects/` 不适用
- fe6 的 `Common/` 混装了 34 个 banim 文件，本仓库把战斗动画单独放 `Battle/`
- `Data/` 独立出来，因为本仓库有 33 个纯数据 `.c`（fe6 的数据多在 `gamedata/`）

---

## 3. 文件归属明细

### Boot/（15）

```
main irq hardware rng time agb_sram ramfunc sysutil ap proc gamecontrol
animedrv scene face ctc
```

### Core/Map/（44）

```
bm bmmap bmarch bmcamadjust bmcontainer bmdebug bmgold bmitem bmmind bmphase
bmreliance bmshop bmtarget bmtrade bmtrap bmtrick bmusemind bmusort bmxfade
bmudisp bmidoten bmpatharrowdisp bmcommanddbg bmdifficulty bmguide bmguide_config
broken: bksel cursor_hand fogmap minimap trapfx spinning_arrow unit_facing
playerphase phasechangefx mu muctrl koido player_interface bmarena
```

> `bmdebug` / `bmcommanddbg` 是否归 `Debug/`，见 §6 待定项 D3。

### Core/Unit/（10）

```
bmunit bmitemuse bmusailment classchg-main classchg-data classchg-event
classchg-handler classchg-menuconfirm classchg-menuselect classchg-postconfirm
classchg-sel monstergen
```

### Core/Save/（9）

```
bmsave bmsave-bwl bmsave-gmap bmsave-lib bmsave-misc bmsave-multiarena bmsave-xmap
savemenu savedraw
```

### Core/Prep/（17）

```
prep_atmenu prep_itemlist prep_itemscreen prep_itemsupply prep_itemtrade
prep_itemuse prep_itemusemind prep_menuproc prep_menuscroll prep_sallycir
prep_sallycursor prep_unitselect prep_unitswap prep_utils prep_wm_sell
prep_80A0760 prepscreen
```

### Core/WorldMap/（26）

```
worldmap_80C0834 worldmap_automu worldmap_basemenu worldmap_cursor worldmap_draw
worldmap_effect worldmap_entry worldmap_face worldmap_gmapunit worldmap_hb
worldmap_line_fade worldmap_main worldmap_mapmu worldmap_node_data
worldmap_palette_fade worldmap_path worldmap_player_interface worldmap_radar
worldmap_rm worldmap_screen worldmap_screen2 worldmap_scrollmanage
worldmap_shop_data worldmap_sogu worldmap_status_ui worldmap_text
worldmap_timemons worldmap_tm_confront
```

### Core/Sio/（20）

```
sio_804B920 sio_bat sio_battlemap sio_core sioerror sio_event sio_main sio_main2
sio_menu sio_menudef sio_mu sio_multiboot sio_points sio_postbattle sio_result
sio_rulesettings sio_tactician sio_teamlist sio_term sio_uiutils
```

### Core/Events/（22）

```
event eventcall eventcmd_warp eventinfo eventwarpfx eventscr eventscr2 eventscr3
eventscr_gmap eventscr_utils events_info events_script events_script_utils
events_shoplist events_trapdata events_udefs events_wm eventfx-boomfog
eventfx-earthquake eventfx-fade eventfx-shinningstone eventfx-stoneshatter
```

> `events_udefs.c`（75k 行）与 5 个 `eventfx-*` 是否拆开，见 §6 D2。

### Core/Ending/（4）

```
ending_credits ending_details gameoverbgfx gamerankings
```

### Battle/（87）

```
banim-*（79） bb bmbattle cp_battle cp_data cp_decide cp_order cp_perform
cp_phase cp_script cp_staff cp_utility cpextra_80407F0
ekr*（若有独立文件，归此处）
```

### MapAnim/（20）

```
mapanim mapanim_api mapanim_debug mapanim_eventbattle mapanim_eventcall
mapanim_expbar mapanim_infobox mapanim_lvup mapanim_lvupfx mapanim_scanline
mapanim_specialeffect mapanim_spellassoc mapanim_spellassocfx mapanim_staffeffect
mapanim_summon danceringfx emitstarfx lightrunefx minefx whitecircularfadefx
```

### UI/（40）

```
uimenu uiutils uiconfig uidebug uiarena uichapterstatus uiselecttarget uisupport
unitlistscreen statscreen statbar unitinfowindow menuitempanel menu_def
helpbox popup popup2 chapter_title classdisplayfont titlescreen mural_background
extramenu_unk difficultymenu soundroom bonusclaim bonusclaim_helpbox cg cgtext
convoymenu chapterintrofx chapterintrofx_title
opanim-main opanimfx opanim-healthsafetyscreen opinfo opsubtitle
```

### Common/（12）

```
bmlib bmlib-hblank bmlib-math fontgrp fontgrp-data icon msg m4a m4a_tables
soundwrapper spellassoc spellassoc-data
```

### Data/（33）

```
data_characters data_classes data_items data_terrains data_itembonus data_itemuse
data_supports data_battlequotes data_banimconf data_banimconfunk data_event_trigger
data_debug_creditinfo data_debugpinfo banim_data banim_terrain_data banim_pal_chara
monstergen_data portrait_data unit_icon_move_data unit_icon_wait_data
soundroom_data savemenu_data chap_title_data bmreliance-data spellassoc-data
bmdifficulty-ewram m4a_tables ...
```

> 注：`bmdifficulty-ewram` 放这里是因为它只有 EWRAM 数据定义；`spellassoc-data`、
> `bmreliance-data` 同理。**这一档需要你确认命名**（见 §6 D1）。

### Debug/（3）

```
code_8086934 code_80AC6AC fe3_dummy
```

---

## 4. 执行机制

### 4.1 移动脚本 `tools/refactor/move_src.py`

对每个文件做四件事：

1. `git mv src/<old>.c src/<New>/<old>.c`
2. 改写三个链接脚本：`src/<old>.o` → `src/<New>/<old>.o`（**逐行替换，不改变行序**）
3. 改写 `#include`：只改指向 `src/` 下头文件的引用（目前很少）
4. 改写 `Makefile`：`C_SUBDIR` 相关通配目前是 `$(wildcard src/*.c)`，需要改成递归 `find`

### 4.2 Makefile 需要同步改的地方

```make
# 现在
CFILES := $(wildcard $(C_SUBDIR)/*.c)
# 改成
CFILES := $(shell find $(C_SUBDIR) -name '*.c')
```

`src/data/` 下是 `.h` / `.inc` / `.json`，不参与编译，但要确保 include 路径仍然有效。

### 4.3 验证

每移动一个目录就跑一次：

```bash
make -j16 && make check     # 必须 fireemblem8.gba: OK
```

预期：**字节完全一致**。若不一致，说明对象顺序被破坏或某处路径没改到。

### 4.4 分批顺序

| 批 | 内容 | 文件数 |
|---|---|---|
| 1 | `Data/` | 33 |
| 2 | `Debug/` + `Boot/` + `Common/` | 30 |
| 3 | `Core/Save/` `Core/Unit/` `Core/Ending/` | 23 |
| 4 | `Core/Prep/` `Core/WorldMap/` `Core/Sio/` | 63 |
| 5 | `Core/Events/` `MapAnim/` | 42 |
| 6 | `UI/` | 40 |
| 7 | `Battle/` | 87 |
| 8 | `Core/Map/` | 44 |

先做文件少、前缀统一的批次，积累信心再做大的。

---

## 5. 风险

| # | 风险 | 缓解 |
|---|---|---|
| 1 | 链接脚本漏改一处 → 该对象被 `/DISCARD/` 丢掉 | 移动后跑 `make check`；另外逐文件确认 `ldscript.txt` 里有对应对象 |
| 2 | 对象顺序被改动 → 地址漂移 | 脚本只做字符串替换，不重排；diff 里不应出现行移动 |
| 3 | `#include` 相对路径失效 | 目前 `src/` 下没有相互 include 的 `.c`；`src/data/*.h` 用 `src/data/...` 绝对式引用，移动 `src/data` 时会受影响 |
| 4 | Makefile 通配改错 → 漏编译文件 | 对比移动前后 `objects.lst` 的行数与内容 |
| 5 | 一次性移动 359 个文件难以 review | 按 §4.4 分 8 批，每批独立验证 |

---

## 6. 待你确认的点

| # | 决策 | 我的建议 |
|---|---|---|
| D1 | `Data/` 与 `Common/` 的边界：`data_*` 全进 `Data/`，但 `banim_data.c`、`m4a_tables.c` 这类「属于某模块的数据」是跟着模块走还是进 `Data/` | **跟模块走**：`banim_data.c` → `Battle/`，`m4a_tables.c` → `Common/`，其余 `data_*` → `Data/` |
| D2 | `events_udefs.c`（75k 行）现在拆不拆 | **先不拆**，只跟着 `Core/Events/` 移动；拆分留到 M4b |
| D3 | `bmdebug.c` / `bmcommanddbg.c` / `uidebug.c` / `mapanim_debug.c` 归 `Debug/` 还是各自模块 | **各自模块**（它们虽然是调试用，但依赖模块内部数据），不单独建 `Debug/` 域；`Debug/` 只留 3 个 decomp 遗留文件 |
| D4 | 目录名大小写 | 沿用 fe6 的大驼峰（`Battle/` `Core/` `UI/`），`UI` 大写缩写 |
| D5 | 是否同时重命名文件（如 `bmlib-math.c` → `bmlib_math.c`） | **不重命名**，M4a 只做移动；重命名会同时改符号引用，风险高，留到 M4b |
| D6 | 头文件目录 `include/` 是否同步分类 | **M4a 不动 `include/`**，只动 `src/`；头文件与源码的相对关系目前是 `-I include` 平铺，动它风险高，单独排一步 |

---

## 7. 需要你回复

一句即可，例如：

- 「按方案做，D1-D6 按建议」——我按 8 批执行，每批跑 `make check`，全部完成后汇报（不提交）
- 「D2 现在就拆」——把 `events_udefs.c` 的拆分并入 M4a
- 「目录名改成小写」——`battle/` `core/map/` 这种
