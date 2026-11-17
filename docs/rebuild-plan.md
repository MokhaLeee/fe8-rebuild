# FE8-rebuild 重构任务规划

- 版本：v0.1（2026-11-17）
- 基线：`a53d6aa first commit`（fireemblem8u decomp 全量快照）
- 参考工程：`~/fe6-rebuild`
- 目标：把「与 ROM 字节一致的 decomp」改造成「面向二次开发的游戏源码工程」

---

## 0. 结论先行

1. **现状是干净的 decomp**：`make -j16` 可全量构建，`sha1sum -c checksum.sha1` 通过（本次已实测）。
   这意味着后面每一步都可以用「ROM sha1 不变」作为回归门禁——这是最值钱的资产，别过早扔掉。
2. **`make compare` 建议改名保留、而不是删掉**。它是阶段 1–4 唯一的自动化门禁。
   到「合并代码」那一步才正式退役，退役前必须先有 `tools/emu` 行为测试兜底（fe6 就是这么走的）。
3. **最大技术债是 `ldscript.txt` 的对象级布局**：1031 处 `src/xxx.o(.text)` 把每个目标文件钉死在固定地址。
   任何「移动文件 / 合并文件 / 新增文件」都会撞上它。所以顺序上必须：
   *先只移动（改路径），再放松链接脚本（按段聚合），最后才合并*。
4. **`baserom.gba` 不能马上删**：还有 1293 处 `.incbin "baserom.gba"`（约占 ROM 1.07 MB / 6.7%）。
   这些是尚未外置的原始数据块，要一并在「contents 素材化」里处理。
5. 建议把 6 个原始步骤重排为 8 个阶段（见 §3）：清理 → 宪法 → 风格 → 纯移动 → 素材外置 →
   测试基建 → 合并 → gcc 迁移。**测试基建要提到「合并」之前**，否则合并之后就没有回归手段了。

---

## 1. 现状体检（实测数据）

| 项 | 实测值 |
|---|---|
| 构建 | `make -j16` 通过；默认目标是 `compare`；产物 `fireemblem8.gba`，sha1 与 `checksum.sha1` 一致 |
| Git | 仅 1 个提交 `a53d6aa`，无 remote / 无分支 / 无 tag；跟踪文件 7028 个 |
| C 代码 | 359 个 `.c`，342,846 行 |
| 头文件 | 174 个，76,424 行 |
| 汇编 | 321,822 行；`data/*.s` 59 + `data/banim/*.s` 202 + `asm/` 4 + `src/*.s` 4 |
| 素材 | `graphics/` 4,655 文件（2,330 PNG）；`sound/` 1,142（589 MID）；`texts/` 2 |
| 反编译沉积 | `_site/`（1）、`reports/`（3 CSV/TXT）、`docs/`（5 篇 decomp 报告）、`scripts/` 128 个（绝大多数 `dump_*.py`） |
| 链接布局 | `ldscript.txt` 55 KB，1031 处对象级定位，涉及 362 个 `.o` |
| baserom 依赖 | 1293 处 `.incbin "baserom.gba"`，覆盖约 1.07 MB / 6.7% ROM |
| 工具链 | 系统 `arm-none-eabi-*`（gcc 9.2.1）+ 自带 `tools/agbcc`；`DEVKITARM` 未设置也能构建 |
| 格式工具 | `clang-format 10.0.0` 可用；无 `uncrustify` / `indent` |
| 编译器差异 | `tools/agbcc/lib/libgcc.a` 只有 `__udivsi3` 系列，**没有 `__aeabi_uidiv`**；gcc 写法必须走 klib |
| 混杂标记 | 少量 `NONMATCHING` 条件编译（`eventinfo.c`、`worldmap_path.c` 等） |
| 目录现状 | `src/` 平铺 359 个 `.c`，无子目录；`include/` 平铺 + `constants/`、`gba/` |

### 1.1 `src/` 文件族分布（决定分目录与合并粒度）

| 前缀 | 数量 | 归属 |
|---|---|---|
| `banim*` | 79 | 战斗动画脚本/逻辑 |
| 无前缀（逻辑代码） | 145 | 按模块细分 |
| `worldmap_*` | 28 | 世界地图 |
| `sio_*` | 19 | 联机 |
| `prep_*` | 16 | 准备画面 |
| `mapanim_*` | 14 | 地图动画 |
| `data_*` | 13 | 数值表 |
| `events*` | 12 | 事件脚本 |
| `cp_*` | 10 | 战斗流程 |
| `classchg*` | 8 | 转职 |
| `bmsave*` | 7 | 存档 |
| `eventfx*` | 5 | 事件特效 |
| `opanim*` | 3 | OP 动画 |

体量极端的文件：`events_udefs.c` 1.78 MB / 75,118 行、`data_terrains.c` 205 KB、
`data_classes.c` 180 KB、`data_characters.c` 153 KB、`bmbattle.c` 72 KB。

---

## 2. 目标形态

与 fe6-rebuild 对齐（括号内为 fe8 的差异说明）：

```
fe8-rebuild/
├── AGENTS.md              # 宪法（M1）
├── Makefile               # 单一入口：make all / check / clean / test
├── README.md
├── baserom.gba            # 构建/素材源，不入库；M4c 后可由 contents/ 取代
├── vanilla/               # 与原版字节一致的 agbcc 代码（M6 才出现）
├── src/                   # 重构代码，arm-none-eabi-gcc 编译
│   ├── Core/              # Battle/ Map/ Unit/ Phase/ Save/ Event/ WorldMap/ Sio/ Prep/
│   ├── Common/            # proc/ sprite/ gfx/ sound/ text/ klib/ crt0/
│   ├── UI/                # menu/ statscreen/ prep 界面/ helpbox/ popup
│   ├── Data/              # 数值表
│   ├── Events/            # 章节 eventinfo / eventscript
│   └── Test/              # testmenu / harness / cases / fixtures
├── include/               # constants/ hacks/ configs/ gba/
├── contents/              # 素材：banims/ portraits/ cg/ bg/ maps/ ui/ op/ …
├── gamedata/              # characters/ classes/ items/ terrains/ sounds/ …
├── gameinfo/              # chapters/ events/ menus
├── texts/                 # 文本（fe6 同样留在根）
├── lds/                   # 链接脚本
├── tools/                 # 构建工具 + emu + 素材工具 + 重构脚本
└── docs/                  # 开发文档 + 报告
```

**关键判断**：`src/` 的划分不要照抄 fe6 的 `Core/Common/SkillEffects`（那是 fe6 的功能域），
应改成 FE8 自己的模块域（battle / map / prep / worldmap / sio / mapanim / banim / ui / data）。

---

## 3. 执行顺序（与原始 6 步的映射）

```
M0 基线固化
 └─ M1 宪法
     └─ M2 瘦身清理
         └─ M3 代码风格（分批，compare 门禁）
             └─ M4a 目录重组（纯移动，compare 门禁）
                 ├─ M4c 素材 contents 化 + incbin 外置（compare 门禁）
                 │    └─ M5 测试基建 + testmenu（此时 compare 仍成立）
                 │        └─ M4b 合并/拆分（放弃 compare，改行为门禁）
                 │            └─ M6 agbcc → arm-none-eabi-gcc（逐模块）
```

| 原始步骤 | 阶段 | 说明 |
|---|---|---|
| 1 宪法 | M1 | 照搬 fe6 的 AGENTS.md，改构建/测试入口与 klib 生效时机 |
| 2 删反编译部分 | M2 | 含 `site`、`make compare`、`reports`、dump 脚本 |
| 3 代码风格 | M3 | Linux kernel style，分批 + sha1 门禁 |
| 4 代码整理 | M4a/M4c/M4b | 拆成「纯移动」「素材外置」「合并」三步，中间穿插 M5 |
| 5 测试 + testmenu | M5 | 移植 fe6 的 `tools/emu` |
| 6 agbcc → gcc | M6 | 双轨（vanilla + src），逐模块迁移 |

**为什么把「合并」排到测试之后**：合并会改变目标文件边界 → `ldscript.txt` 顺序变化 → ROM
不再字节一致。没有 `tools/emu` 就失去了唯一的正确性判据。而「纯移动」和「incbin 外置」
都能保持 sha1 不变，可以放心提前做。

---

## 4. 阶段任务分解

### M0 基线固化（0.5 天）✅ 已完成（2026-11-17）

| # | 任务 | 产出 | 状态 |
|---|---|---|---|
| 0.1 | 干净树全量构建并计时 | [baseline.md](baseline.md)（耗时、产物大小、sha1） | ✅ 45.8 s / sha1 OK |
| 0.2 | 记录工具链版本（agbcc、arm-none-eabi-gcc、python、gbagfx） | 同上 | ✅ |
| 0.3 | 打 tag `decomp-baseline` | 一个可回退的锚点 | ✅ |
| 0.4 | 可选：`git bundle` 备份到仓库外 | 离线备份 | ⬜ 未做 |

**实测收获**：干净树 `-j16` 全量构建仅 **45.8 秒**，迭代成本极低，
因此 M3（风格）与 M4a（目录）可以放心做到「每批一次字节门禁」。

**已澄清**：`baserom.gba` 不进版本库是 decomp 惯例（版权 + 体积），
`Makefile` 的 `clean_all` 也带了 `-e baserom.gba` 排除。不构成风险。
详见 [baseline.md §4.1](baseline.md)。

**验收**：`make clean && make -j16` 成功，`sha1sum -c checksum.sha1` → `OK`。

**风险**：仓库只有 1 个提交，`git clean`/批量删除不可恢复。**tag 必须在任何删除之前完成。**

### M1 宪法（1 天）✅ 已完成（2026-11-17）

从 `~/fe6-rebuild/AGENTS.md` 移植，逐条标注「照搬 / 改写 / 暂不适用」：

| 条款 | 处理 | 状态 |
|---|---|---|
| 思考与表达（中文） | 照搬 | ✅ |
| 提交与版本控制纪律 | 照搬，并加硬「提交前必须先停一次、列清单等确认」 | ✅ |
| 改动纪律、破坏性操作先确认 | 照搬（补 `git clean` / `make clean_all` 禁止项） | ✅ |
| 汇报与收尾 | 照搬 | ✅ |
| 代码风格 Linux kernel style | 照搬，并补 FE8 的具体细则 | ✅ |
| 除法/取模走 klib | **改写**：M6 起生效（M6 前 agbcc 自带 `__udivsi3`，不冲突） | ✅ |
| 报告对齐审查（日报/周报/月报） | 暂不启用，明确「代理不得自行新建」 | ✅ |
| 构建入口 | 改写：`make all` → `fe8re.gba` / `.elf` / `.sym` / `.map` | ✅ |
| 测试入口 | 先占位，M5 落地后回填 | ✅ |

配套落地：

| 文件 | 内容 | 状态 |
|---|---|---|
| `include/hacks/klib.h` | `k_udiv` / `k_umod` / `k_udiv10` / `k_u64div` / `k_u64mod` / `k_sdiv` / `perc_of` + 位操作内联，**只有接口** | ✅ |
| `include/configs/configs.h` | `CONFIG_USE_DEBUG` / `CONFIG_TEST_MENU` / `CONFIG_DEBUG_MENU` / 打印开关，默认全关 | ✅ |
| `include/configs/configs/config-debug.h` | debug 构建的覆盖层 | ✅ |
| `.clang-format` | 内核风格（Tab 8、`BreakBeforeBraces: Linux`、`ColumnLimit: 0`、`IndentCaseLabels: false`、不排 include） | ✅ |
| `.editorconfig` | Tab 8 / UTF-8 / LF | ✅ |
| `docs/reports/` | 暂不建 | ⬜ 待定 |

**验收**：`make -j16` + `sha1sum -c checksum.sha1` → `OK`（已跑）。
新增的 4 个头文件/配置**未被任何代码包含**，构建行为零变化。

**M1 发现的两个坑**：

1. **`include/.clang-format` 会覆盖根目录配置**（上游自带 Allman + 4 空格 + 不缩进 case）。
   M3 格式化 `include/` 之前必须先删它，否则 batch 6 会得到与宪法相反的风格。
2. **clang-format 10 不认若干新选项**（`IndentCaseBlocks`、`InsertBraces`、
   `RemoveBracesLLVM`），写进配置会直接报错退出，不是静默忽略。已移除。

### M2 瘦身清理（1–2 天）✅ 已完成（2026-11-17）

**A. 直接删除** ✅

| 目标 | 理由 | 状态 |
|---|---|---|
| `_site/` | GitHub Pages 文档站 | ✅ |
| `buddy.yml` | 上游 Buddy CI 配置 | ✅ |
| `CONTRIBUTING.md` | 上游 PR 规范 | ✅ |
| `asmdiff.sh` | 反汇编比对包装 | ✅ |
| `checksum.sha1` | sha1 移入 `Makefile` 的 `ROM_SHA1` | ✅ |
| `include/.clang-format` | 会劫持根目录格式化配置 | ✅ |

> 与初版清单的差异：`reports/`（3 CSV）、`graphic_extract.yml`、`docs/` 5 篇报告
> 从「删除」**降级为归档**——`reports/*.csv` 是 `update_banim_img_rules.py` 的输入，
> 删了以后无法重新生成 `banim_img_rules.mk`；其余在 M4c 素材外置时还有参考价值。

**B. 迁移/归档** ✅

| 目标 | 去处 |
|---|---|
| `scripts/` 构建必需闭包（20 个文件）：`arm_compressing_linker.py`、`compressor.py`、`mar_to_map.py`、`tmap2tsa.py`、`tsa_png.py`、`update_banim_img_rules.py`、`gfxtools/`、`texttools/` | `tools/scripts/` |
| `scripts/` 其余 108 个 + `dump_sound/` | `tools/extract/` |
| `reports/`（3 CSV） | `tools/extract/reports/` |
| `graphic_extract.yml`（820 KB） | `tools/extract/` |
| `docs/` 5 篇 decomp 报告 | `docs/decomp/` |
| `tools/{agbcc,gbagfx,bin2c,scaninc,aif2pcm,mid2agb,textencode,jsonproc}` | 留在原地 |
| `ldscript.txt`、`linker_script_banim.txt`、`linker_script_sound.txt` | 留给 M4a |

已同步的路径常量：`Makefile` 全部 `scripts/` 引用、`update_banim_img_rules.py` 的
`LABELS_CSV`/`ASSET_MAP_CSV`、`extract_file.py` 的 `graphic_extract.yml`、
`include/constants/msg.h` 的生成器注释。

**C. 重写** ✅

- `Makefile`：默认目标 `all`；`compare` → `check`（内嵌 `ROM_SHA1`）；删 `clean_all`、`tag`
- `README.md`：改成开发向（构建、目录、约束、路线）
- `compile_flags.txt`：修掉拼错的 `-WParenheses`，补 `-iquote`、`-std=gnu99`、`-mcpu`
- `.gitignore`：去重 28 条，`scripts/dump_sound/` → `tools/extract/dump_sound/`
- `tools/extract/README.md`：新增，说明归档用途

**验收结果**：`make -j16` → `make check` → `fireemblem8.gba: OK`。
根目录条目 36 → 28；`scripts/` 目录消失；跟踪文件 7,036 → 7,030（删 6 个）。
构建产物无泄漏（`make clean` 后 `git status` 干净）。

### M3 代码风格：Linux kernel style（4–6 天）✅ 已完成

| # | 任务 | 说明 |
|---|---|---|
| 3.1 | `.clang-format` 草案 | `UseTab: Always`、`TabWidth: 8`、`IndentWidth: 8`、`BreakBeforeBraces: Linux`、`PointerAlignment: Right`、`ColumnLimit: 0`、`IndentCaseLabels: false` |
| 3.2 | 试点 5 个文件 | 必须 `make check` 通过，证明「格式化不改代码生成」 |
| 3.3 | 分批格式化 | 每批 20–40 文件，独立 commit |
| 3.4 | 例外清单 | 33 个含 `asm(`/`INCBIN` 的 `.c`；`*.inc`；`songs.mk` 等 makefile |
| 3.5 | `tools/style/format.sh` | 只格式化指定文件，不递归全库 |
| 3.6 | `.git-blame-ignore-revs` | 记录纯格式化提交，保住 `git blame` |

**建议批次**：

1. `src/banim-*.c`（79）
2. `src/worldmap_*.c`（28）
3. `src/sio_*.c`（19）
4. `src/prep_*.c` + `src/mapanim_*.c`（30）
5. 其余逻辑代码（145）
6. `include/*.h`（174）
7. `src/events/*.h`（137，宏/表定义，风险低）
8. `src/data_*.c`（13，大表，需专门规则避免逐项折行）

**已知坑**：

- `#define` 续行反斜杠、`#if` 嵌套缩进
- 字符串里的全角空格 U+3000（fe6 宪法明令禁止）
- 内联汇编块、`asm` 语句不要被重排
- 数据表文件格式化后 diff 极大且无收益 → 建议用 `// clang-format off` 包住表体，只统一外层

**验收**：每批 `make check` 通过。

#### M3 实际结果（2026-11-17 完成）

| 项 | 结果 |
|---|---|
| 转换文件 | **355 / 359**（4 个大表例外，见 [AGENTS.md §0.3.4](../../AGENTS.md)） |
| 门禁 | 每批 `make check` 全部 OK |
| 最终扫描 | Allman 残留 0 / 函数大括号同行 0 / 行首空格 0 / 行尾空白 0 |
| 工具 | `tools/style/{indent-tabs,brace-kr,decl-blank,strip-trailing,token-check}.py` |

**四处与计划的偏差，都由实测证据推翻原方案**：

1. **clang-format 路线废弃**。以 35 个手改文件为基准做回归，clang-format 只复现 7 个；
   且会把 GCC 语句表达式 `({x;})` 加空格、把 `a < b >> c` 拆成模板参数，
   两者都让 agbcc 报错。详见 AGENTS.md §0.3.1。
2. **删大括号被否决**。实测 `src/mu.c` 的 `ResetMuAnims`：删掉单语句大括号后
   agbcc 寄存器分配改变（`push {r4-r7,lr}` → `push {r4,r5,lr}`），ROM 不再一致。
   M3 只改大括号**位置**。详见 AGENTS.md §0.3.3。
3. **4 个大表不转换**。它们本身 4/8 空格混排，产物是半 Tab 半空格，
   且 M4b 要重新组织这些表。详见 AGENTS.md §0.3.4。
4. **判定方法换掉**。`git diff -w` 不可用（K&R 大括号会合并行，`-w` 仍算增删）；
   改用 **token 级比对 + 函数计数 + ROM sha1** 三重校验。

**速度**：纯人工阶段每轮 5–18 个；工具链成型后每轮 60–121 个。

### M4a 目录重组：只移动，不改内容（2–3 天）

| # | 任务 | 说明 |
|---|---|---|
| 4a.1 | 定目录树 | 见 §2；`src/` 按 FE8 模块域分 |
| 4a.2 | `tools/refactor/move_src.py` | `git mv` + 自动改写 `ldscript.txt` 对象路径 + `#include` 路径 + Makefile 通配 |
| 4a.3 | 分批移动 | 先逻辑代码 → 再 `data/` → 再 `include/` |
| 4a.4 | `lds/` 迁移 | `ldscript.txt` → `lds/gba_cart.lds`（内容先原样保留） |

**核心约束**：`ldscript.txt` 里每个对象的**出现顺序不能变**（顺序即地址）。只改路径、不改顺序，
sha1 才会不变。

**验收**：`make check` 一致；`git ls-files | wc -l` 不变；`git log --follow` 可追溯。

### M4c 素材 contents 化（3–5 天）

| # | 任务 | 说明 |
|---|---|---|
| 4c.1 | 盘点 | 区分「源素材」（png/pal/tsa/oam/mid/mml）与「生成物」（4bpp/lz/bin/o） |
| 4c.2 | 建 `contents/<domain>/` | `banims/ portraits/ cg/ bg/ maps/ ui/ op/ statscreen/ unit_icon/ item_icon/ …` |
| 4c.3 | incbin 外置 | 1,293 处 `baserom.gba` 引用 → 逐块导出为 `contents/` 文件，`.s` 改指向新路径 |
| 4c.4 | 规则改写 | `graphics_file_rules.mk`、`songs.mk`、`json_data_rules.mk`、`graphics/banim/assets/img/banim_img_rules.mk` |
| 4c.5 | 素材文档 | `contents/README.md`：每个目录放什么、怎么生成 |

**验收**：`make check` 一致（内容不变 → sha1 不变）；`git grep -c 'baserom.gba'` 显著下降；
最终目标是构建不再依赖 `baserom.gba`（可作为可选项）。

### M5 测试基建 + testmenu（3–5 天）

| # | 任务 | 来源 |
|---|---|---|
| 5.1 | 移植 `tools/emu` | fe6：`gbarun.c`（mGBA 核心）、`build.sh`、`testmenu-tests.sh`、`testmenu/lib.sh` |
| 5.2 | 配置系统 | `CONFIG_TEST_MENU` / `CONFIG_DEBUG_MENU` |
| 5.3 | testmenu 骨架 | 顶层 `Start Game` / `Test`；基于 fe8 已有的 `bmdebug.c`、`menu_def.c`、debug 章节 |
| 5.4 | 核心 case | boot→ch1、地图菜单、save/load、battle banim（GDB 断点）、prep |
| 5.5 | runner | `--list` / `--case` / `--quick`；退出码 0/非 0；截图与日志 |
| 5.6 | CI（可选） | 本地 `make test` 起步 |

**验收**：`./tools/emu/testmenu-tests.sh` 全绿；此时 `make check` 仍然成立（双保险）。

### M4b 合并/拆分（3–5 天，正式放弃字节一致）

**前置**：链接脚本放松（对象级 → 按段聚合）。这一步技术上属于 M6，但 M4b 需要它。

| # | 任务 | 说明 |
|---|---|---|
| 4b.0 | `lds/gba_cart.lds` 放松 | `*(.text)` / `*(.rodata)` / `*(.data)` 聚合 + `objects.lst` 控制顺序 |
| 4b.1 | 拆 `events_udefs.c` | 1.78 MB / 75k 行，按章节拆到 `src/Events/` |
| 4b.2 | 合并 `banim-efxmagic-*.c` | 40+ 个 → 数个；处理 static 重名 |
| 4b.3 | `data_*.c` 搬家 | 大表移到 `gamedata/`，与 fe6 一致 |
| 4b.4 | 其他小文件合并 | `cp_*`、`prep_*`、`worldmap_*` 等按需 |

**原则**：合并前先跑一次 emu 全绿作为对照；合并后 emu 必须仍全绿。
**验收**：emu 全绿 + 符号表（`.sym`）齐全 + 关键地址的符号仍可解析。

### M6 agbcc → arm-none-eabi-gcc（15–25 天，持续）

| # | 任务 | 说明 |
|---|---|---|
| 6.1 | 双轨骨架 | `git mv src vanilla`；新建 `src/` 只放新代码；Makefile 同时支持 `vanilla/`（agbcc）与 `src/`（gcc） |
| 6.2 | klib 落地 | 从 fe6 移植 `include/hacks/klib.h` + `src/Common/klib/math.c`（`k_udiv`/`k_umod`/`k_udiv10`/`k_u64div`） |
| 6.3 | 编译参数与头文件层 | `-mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -fno-builtin -fno-jump-tables -std=gnu99 -Wall -Wextra -Werror`；`-nostdinc` 与 `include/gba/*` 的冲突处理 |
| 6.4 | 逐波迁移 | 见下表 |
| 6.5 | 差异清单维护 | 见 §7 风险 R5 |
| 6.6 | agbcc 退役 | 删除 `tools/agbcc/`、iconv 管道、`cc1` 规则 |

**迁移波次（从叶子到根）**：

| 波次 | 内容 | 文件量 | 风险 |
|---|---|---|---|
| W1 | 数值表：`data_*.c`、`*_data.c`、`banim-*` 数据脚本 | ~100 | 低（多为常量） |
| W2 | 基础工具：`rng`、`hardware`、`fontgrp`、`sysutil`、`bmlib-math`、`spline`、`colorfade`、`time` | ~10 | 低 |
| W3 | 核心系统：`proc`、`ap`、`icon`、`msg`、`event*`、`bm*`（不含 battle） | ~40 | 中 |
| W4 | 战斗：`bmbattle`、`banim-*`、`cp_*`、`bb` | ~100 | 高 |
| W5 | 界面：`uimenu`、`uiutils`、`statscreen`、`unitlistscreen`、`prep_*`、`helpbox`、`popup` | ~40 | 中 |
| W6 | 大模块与入口：`worldmap_*`、`sio_*`、`mapanim_*`、`opanim*`、`main`、`scene`、`savemenu` | ~70 | 高 |

**每文件验收**：编译通过 → `arm-none-eabi-nm` 无 `__aeabi_.*div` → 相关 emu case 通过 →
（可选）与 vanilla 对象做函数级 asm diff。

---

## 5. 门禁矩阵

| 阶段 | 主要判据 | 辅助判据 |
|---|---|---|
| M0 | 干净树构建成功 + sha1 OK | 构建耗时记录 |
| M1 | `make check` OK | `AGENTS.md` 生效 |
| M2 | `make check` OK | 跟踪文件数下降、目录树变干净 |
| M3 | 每批 `make check` OK | `git diff -w` 为空 |
| M4a | `make check` OK | `git ls-files` 数量不变 |
| M4c | `make check` OK | `baserom.gba` 引用数下降 |
| M5 | emu 测试全绿 | `make check` 仍 OK |
| M4b | emu 全绿 | 符号表完整 |
| M6 | 每文件编译 + emu 通过 | 无 `__aeabi_*div`；asm diff |

---

## 6. 需要拍板的决策点

| # | 决策 | 建议 |
|---|---|---|
| D1 | 是否保留「字节一致」作为 M0–M5 的门禁 | **保留**。这是纯重构阶段最强的回归信号 |
| D2 | `src/` 目录方案 | 按 FE8 模块域（battle/map/prep/worldmap/sio/mapanim/banim/ui/data），不照抄 fe6 的 Core/Common |
| D3 | 是否跟上游 decomp 同步 | **不常规跟**。M6 之后彻底脱离；需要时手工 cherry-pick 命名改进 |
| D4 | dump 脚本删还是留 | 先移到 `tools/extract/` 归档，M4c 完成后再删 |
| D5 | 是否启用日报/周报制度 | 看工作节奏；建议先启用日报，周/月报按需 |
| D6 | 产物名 | `fe8re.gba` / `.elf` / `.sym` / `.map` |
| D7 | 是否立刻执行 M0（打 tag + 基线构建） | 建议先做，成本半天，收益是可回退 |

---

## 7. 风险登记

| # | 风险 | 影响 | 缓解 |
|---|---|---|---|
| R1 | 仓库只有 1 个提交，误删不可恢复 | 高 | M0 打 tag + `git bundle` |
| R2 | `ldscript.txt` 对象级布局，移动/合并即错位 | 高 | 先纯移动并保持顺序；合并前放松 lds |
| R3 | 格式化破坏特殊构造（asm 块、宏续行、U+3000） | 中 | 例外清单 + 分批 + `make check` |
| R4 | 合并代码后无法再用 sha1 验证 | 中 | M5 先落地 emu 测试，形成新门禁 |
| R5 | gcc 与 agbcc 语义差异（除法、位域、结构体传参、内联、跳转表） | 高 | klib；`-fno-jump-tables`；逐文件 asm diff；NONMATCHING 分步 |
| R6 | `baserom.gba` 依赖残留 | 中 | M4c 逐块外置，`git grep` 计数作为进度指标 |
| R7 | 单次改动过大导致 review 失效 | 中 | 每阶段独立 commit，纯格式化提交进 `.git-blame-ignore-revs` |
| R8 | 工作量估算偏差（34 万行 C） | 中 | M3/M6 按批次推进，每批可独立验收与中断 |

---

## 8. 工作量估算（单人 + AI 辅助）

| 阶段 | 人日 |
|---|---|
| M0 基线 | 0.5 |
| M1 宪法 | 1 |
| M2 瘦身 | 1–2 |
| M3 风格 | 4–6 |
| M4a 目录 | 2–3 |
| M4c 素材 | 3–5 |
| M5 测试 | 3–5 |
| M4b 合并 | 3–5 |
| M6 gcc | 15–25 |
| **合计（至 M5）** | **约 2.5–3 周** |
| **合计（至全 gcc）** | **约 6–8 周** |

---

## 附录 A：常用命令

```bash
# 基线复现
make clean && make -j$(nproc)
sha1sum -c checksum.sha1

# 阶段门禁（M2 之后）
make check

# 单文件编译（改代码时的快速验证）
make src/bmlib.o

# 检查是否引入 arm eabi 除法
arm-none-eabi-nm fireemblem8.elf | grep '__aeabi_.*div'

# 素材/数据引用排查
git grep -c 'baserom.gba' -- '*.s'
```

## 附录 B：待确认清单

- [ ] D1–D7 决策
- [ ] 是否现在执行 M0
- [ ] M1 宪法是否包含日报/周报制度
- [ ] `contents/` 的目录命名（沿用 fe6 还是按 FE8 素材类型）
