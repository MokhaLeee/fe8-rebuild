# 交接文档（HANDOFF）

> 写给下一个接手的 agent。读完本文 + `AGENTS.md` 就能直接干活。
>
> 最后更新：2026-11-18（M5 框架落地），工作区基线 HEAD = `ced46a9`

---

## 0. 一句话现状

从 decomp 起步的重构工程，**已完成 M0–M4c**：工程结构改成面向开发的样子，
**ROM 仍然与官方原版逐位一致**（`make check` → `fireemblem8.gba: OK`）。
**M5 已开工**：`tools/emu` 无头测试跑通，testmenu 框架落地（`make TEST=1`），
`./tools/emu/testmenu-tests.sh` 26 项断言全绿。下一步是往 testmenu 里加真实 case
（进章节 / 存档 / 战斗 / 准备画面）。

---

## 1. 接手先读什么

| 顺序 | 文件 | 作用 |
|---|---|---|
| 1 | [`AGENTS.md`](../AGENTS.md) | **宪法**。提交纪律、代码风格、构建验证、目录与链接脚本约定。违反会出错 |
| 2 | [`docs/rebuild-plan.md`](rebuild-plan.md) | 阶段规划 M0–M6，含各阶段实际结果与偏差 |
| 3 | [`docs/baseline.md`](baseline.md) | 构建基线（sha1、工具链、门禁定义） |
| 4 | 本文 | 当前状态与下一步 |

阶段方案文档（已完成，供追溯）：[m4a-directory-plan.md](m4a-directory-plan.md)、
[m4c-contents-plan.md](m4c-contents-plan.md)、[m2-cleanup.md](m2-cleanup.md)。

---

## 2. 硬约束速查（违反会立刻出错）

### 2.1 提交

**未经用户对当次提交的明确许可，不许 `git commit`。**
流程：列出「文件清单 + `git diff --stat` + 提交信息」→ 结束本轮 → 等确认。
「继续」「做吧」不算许可。详见 AGENTS.md §2。

### 2.2 字节门禁

```bash
make -j16 && make check     # 期望 fireemblem8.gba: OK
```

基线 sha1 在 `Makefile:115` 的 `ROM_SHA1`。**当前阶段所有改动都必须保持这个值**，
直到 M4b（合并代码）才放弃。

### 2.3 动 `src/` 文件时必须同步的地方

移动或新增 `.c` 会牵动 **5 类**东西，漏一个就失败：

1. **四个链接脚本**（不是三个），都在 `lds/`：`lds/ldscript.txt`、
   `lds/linker_script_sound.txt`、`lds/linker_script_banim.txt`、`lds/sym_iwram.txt`。
   **只能改路径，不能改行序**（行序即地址）。注意 `lds/ldscript.txt` 用
   `INCLUDE "lds/..."` 引用另外两个，路径是相对仓库根写的。
2. **`Makefile` 的逐文件覆盖**：`src/Boot/agb_sram.o`(-O1)、`src/Boot/m4a.o`(旧编译器)、
   `src/Core/Map/bmitem.o` 与 `src/UI/menu_def.o`(-Wno-error)。不命中会静默退回默认选项，代码生成变。
3. **`.gitignore`**：通配必须跟着目录层级走（`src/**/*.s` 而非 `src/*.s`）。
4. **陈旧产物**：`git mv` 不搬 `.o`/`.s`，要清「旁边没有同名 `.c`」的产物；
   **排除** `src/Boot/{rom_header,crt0,m4a_1,libagbsyscall}.s`（手写汇编源，是源码）。
5. **素材路径**：`data/*.s` 的 `incbin`、`mk/graphics_file_rules.mk`、
   `lds/linker_script_banim.txt`（671 处硬编码素材路径）。

### 2.4 代码风格

Linux kernel style 已全量落地，但有三条**本项目特有**的例外（实测得出，别改回去）：

- **不许删单语句大括号**：会让 agbcc 改变寄存器分配，ROM 不一致（AGENTS.md §0.3.3）。
- **4 个大表保持原缩进**：`src/Data/data_{characters,classes,terrains}.c`、
  `src/Core/Events/events_udefs.c`（AGENTS.md §0.3.4）。
- **不许用 clang-format**：会把 `a < b >> c` 拆成模板、把 GCC 语句表达式 `({...})`
  加空格，两者都让 agbcc 报错（AGENTS.md §0.3.1）。

---

## 3. 已完成阶段

| 阶段 | 内容 | 提交 |
|---|---|---|
| M0 | 基线固化（tag `decomp-baseline`、构建 45.8 s、sha1 记录） | — |
| M1 | 宪法 `AGENTS.md`、`include/configs/`、`include/hacks/klib.h`、`.clang-format` | （随 M1 若干提交） |
| M2 | 瘦身：删 `_site`/`reports`/`checksum.sha1`/`CONTRIBUTING` 等；`scripts/` 拆成 `tools/scripts`（构建）+ `tools/extract`（归档）；根目录 36 → 28 项 | `14b380d` `ef9018f` |
| M3 | 355/359 个 `.c` 转 kernel style（4 个大表除外）；工具链 5 个脚本 | `eff691f` … `204548e` |
| M4a | `src/` 按模块分 16 个目录（358 个文件纯移动） | `225b7ee` |
| M4c | `graphics/`→`contents/`、`sound/`→`contents/sounds/`（1,013 个文件纯移动）；incbin 外置**取消** | `71bd459` |

`git log` 共 26 个提交。tag `decomp-baseline` 指向最初的 decomp 快照 `a53d6aa`。

---

## 4. 当前结构

```
fe8-rebuild/
├── AGENTS.md                    宪法
├── baserom.gba                  构建必需输入（不入库，16 MiB）
├── src/                         359 个 .c + 6 个 .s / 334,800 行
│   ├── Battle/     92          战斗、战斗动画（banim/ekr/efx/cp/bb）
│   ├── Boot/       19 + 4 .s   启动运行时（main/crt0/rom_header/irq/hardware/rng/m4a）
│   ├── Common/     12          bmlib/fontgrp/icon/msg/soundwrapper
│   ├── Core/
│   │   ├── Map/    46          bm*/mu/cursor/地形/AI 行动
│   │   ├── Unit/   13          单位/道具/转职/monstergen
│   │   ├── Save/   10          存档
│   │   ├── Prep/   17          准备画面
│   │   ├── WorldMap/ 28        世界地图
│   │   ├── Sio/    20          联机
│   │   ├── Events/ 22          事件脚本与事件表
│   │   ├── Ending/  4          结局
│   │   └── chapterdata.c       单独一个，待归位
│   ├── Data/       13          数据表
│   ├── Debug/       3          decomp 遗留（code_*/fe3_dummy）
│   ├── MapAnim/    20          地图动画与特效
│   ├── UI/         38          菜单/状态画面/标题/OP
│   ├── Test/       —          （目录）testmenu 外壳 + 探针，只进 TEST=1 构建
│   ├── data/       —          （目录）chapter_settings.h（手工维护的章节表）
│   └── events/     —          （目录）章节 eventinfo/eventscript .h
├── contents/                    素材（26 个目录 + sounds/）
├── include/                     .h（constants/ configs/ hacks/ gba/）+ macros/*.inc
├── data/                        265 个 .s（数据段汇编，含 1,297 处 baserom incbin）
├── texts/                       文本
├── lds/                         ldscript.txt + 3 个被 INCLUDE 的脚本
├── mk/                          素材 / 音频规则片段
├── Makefile                     入口：all / check / clean（TEST=1 切换测试构建）
├── tools/
│   ├── agbcc/                   自带编译器（M6 前构建依赖）
│   ├── {gbagfx,bin2c,scaninc,textencode,jsonproc,mid2agb,aif2pcm}/  构建工具（二进制不入库）
│   ├── scripts/                 构建脚本（20 个，参与构建）
│   ├── extract/                 反编译提取脚本（归档，不参与构建）
│   ├── emu/                     无头 GBA 测试（gbarun + runner，.build 不入库）
│   ├── refactor/                M4a/M4c 用的搬运脚本
│   └── style/                   M3 用的风格工具
└── docs/                        规划、基线、各阶段方案
```

---

## 5. 环境事实

| 项 | 值 |
|---|---|
| 主机 | `mokha@10.116.89.94`，Linux，16 核 |
| `DEVKITPRO` / `DEVKITARM` | **未设置也能构建**。用系统 `arm-none-eabi-*`（2.34 / gcc 9.2.1）+ 仓库自带 `tools/agbcc` |
| `baserom.gba` | 构建必需，不入库。没有它构建直接失败 |
| 工具二进制 | 未入库，首次需 `./tools/build_tools.sh`（当前已构建好） |
| 全量构建 | 约 46 秒（`-j16`，干净树） |

常用命令：

```bash
make -j16            # normal 构建：fireemblem8.gba / .elf / .map（字节门禁有效）
make check           # 字节门禁
make TEST=1 -j16     # 测试构建：fireemblem8-test.{gba,elf,map,sym}，开机进 testmenu
make clean           # 清理
./tools/build_tools.sh     # 重建 tools/ 下的工具二进制
./tools/emu/build.sh       # 重建无头测试工具（mGBA 核心库 + gbarun）
./tools/emu/testmenu-tests.sh   # 跑 testmenu 自动化测试
```

两个档位共用同一批 `.o`，切换时 Makefile 在解析阶段自动清掉 C 对象重编
（`.build-flavor` 记录当前档位），不需要手动 `make clean`。

---

## 6. M5 测试基建 + testmenu（框架已落地）

**为什么现在做**：M4b（合并代码）会破坏字节一致，之后就没有判据了。
必须先有行为测试，才能安全地做 M4b 和 M6。

参考实现：`~/fe6-rebuild/tools/emu/`（已跑通的完整方案，可直接借鉴）。

### 6.1 任务清单

| # | 任务 | 说明 | 状态 |
|---|---|---|---|
| 5.1 | 移植 `tools/emu` | `gbarun.c`（mGBA 核心库无头运行）+ `build.sh` + GDB stub + EWRAM 探针 | ✅ 已移植 |
| 5.2 | 接线 `include/configs` | `configs.h` 已接进 `include/prelude.h`；`make TEST=1` 用 `-DCONFIG_USE_DEBUG=1` 打开 | ✅ 已接线 |
| 5.3 | 建 `src/Test/` | `testmenu/` + `harness/` + 探针；`cases/` `fixtures/` 随 case 再加 | ✅ 骨架 |
| 5.4 | testmenu 入口 | `CONFIG_TEST_MENU=1` 时开机进菜单，顶层 `Start Game` / `Test` | ✅ 框架 |
| 5.5 | 核心 case | ①开机 ②菜单导航 ③自检 case ④测试章节地图；存档 / 战斗 / 准备画面待做 | 🔨 部分 |
| 5.6 | `.sym` 生成 | test 构建已产出 `fireemblem8-test.sym`（`elf2sym.py`）；normal 构建仍不产 | ✅ 部分 |
| 5.7 | runner | `tools/emu/testmenu-tests.sh`（`--list` / `--case` / `--clean`），26 项断言 | ✅ 最小版 |

### 6.2 注意

- testmenu 会往 ROM 里加代码，**字节门禁会失效**。所以有两档构建：
  `make`（normal，`make check` 有效）与 `make TEST=1`（testmenu）。
  切换档位由 `.build-flavor` 自动处理（解析阶段清 C 对象重编）。
- test 构建的对象放在 ROM 空洞 `0x08EFB2E0..0x08FE0000`，可变数据放在 EWRAM
  `0x0203FF00` 起；normal 构建完全不受影响。细节见 `src/Test/README.md`。
- FE8 和 FE6 的启动流程不同：FE8 没有 fe6 的 `InitBmDisplay`/`DebugPutStr`，
  testmenu 的初始化用 `SetupBackgrounds()` + `LoadUiFrameGraphics()` + `ResetText()`，
  mural 用 `StartMuralBackground()`，菜单用 FE8 自己的 `struct MenuDef` /
  `StartMenuCore()`。不要照抄 fe6 的入口代码。
- `src/data/chapter_settings.h` 已从「JSON 生成」改为手工维护的源文件（表尾有
  `CONFIG_TEST_MENU` 的 `TEST` 条目，即 `CHAPTER_TEST = 0x4F`）；事件组由
  `chapterdata.c` 的 hook 返回，`worldmap_path.c` / `bm.c` 另有针对 0x4F 的分支。
  切章节走 `StartBattleMap()`，不要走 `RestartGameAndGoto8()`。
- 已知问题：蓝方单位（`FACTION_ID_BLUE`）加载会写坏 EWRAM，目前测试章节只放红方。
  细节见 `src/Test/README.md` §9。

---

## 7. 后续阶段要点（供规划用）

### M4b 合并/拆分（放弃字节门禁）

- 前置：**M5 完成**
- 前置：链接脚本放松（对象级 → 按段聚合），否则任何合并都会破坏布局
- 任务：拆 `events_udefs.c`（75k 行）、合并 `banim-efxmagic-*.c`（40+）、
  `data_*.c` 大表重写为可编辑结构
- 门禁：emu 行为测试

### M6 agbcc → arm-none-eabi-gcc

- 双轨：`vanilla/`（agbcc 保持）+ `src/`（gcc），逐个模块迁移
- **除法必须走 klib**（`include/hacks/klib.h` 已有接口，实现未做）：
  agbcc 的 `libgcc.a` 只有 `__udivsi3`，没有 `__aeabi_uidiv`
- 迁移顺序：数据表 → 叶子工具 → 核心 → 战斗 → 界面 → 入口

---

## 8. 已知遗留问题

| # | 问题 | 影响 | 建议 |
|---|---|---|---|
| 1 | `include/` 未分类（305 个 `.h` 平铺 + `constants/` `gba/` `configs/` `hacks/`） | 可读性 | 单独排一步，风险高（`-I include` 全局生效） |
| 2 | `src/Core/chapterdata.c` 单独一个文件，归位未定 | 小 | 决定放 `Core/` 还是 `Core/Map/` |
| 3 | `src/Data/` 下 `data_banimconf.c` 等其实是战斗数据，分类可再议 | 小 | M4b 时一起调整 |
| 4 | 1,297 处 `incbin "baserom.gba"` 仍在 | 构建依赖 ROM | **用户已明确不做外置** |
| 5 | normal 构建仍不产 `.sym`（test 构建已产） | 调试不便 | 需要时把 `elf2sym.py` 规则推广到 normal |
| 6 | `include/hacks/klib.h` 仍是死代码 | 无影响 | M6 接入；`include/configs/` 已在 M5 接线 |
| 7 | `tools/extract/` 里大量脚本仍指向 `graphics/` 旧路径 | 归档脚本，不影响构建 | 可不管，或批量 sed |
| 8 | `src/data/`、`src/events/` 两个头文件目录仍叫小写，与模块目录风格不一致 | 小 | 移动会牵动大量 `#include`，建议留在最后 |
| 9 | `docs/baseline.md` 里的构建耗时/产物数据是 M0 时期的 | 参考价值降低 | 需要时重测 |

---

## 9. 验证工具（M3 遗产，后面仍有用）

| 工具 | 用途 |
|---|---|
| `tools/style/token-check.py` | token 级比对，抓 `>>` 拆开、符号打错、语句丢失 |
| `tools/style/indent-tabs.py` | 行首空格 → Tab（带字符串保护） |
| `tools/style/brace-kr.py` | Allman → K&R |
| `tools/style/decl-blank.py` | 声明后空行、指针星号 |
| `tools/style/strip-trailing.py` | 去行尾空白 |
| `tools/refactor/gen_mapping.py` | `src/*.c` → 模块目录的映射规则（可重跑） |
| `tools/refactor/move_src.py` | 搬 `.c` + 改写四个链接脚本（保序） |
| `tools/refactor/move_graphics.py` | 搬素材目录 + 改写 incbin 路径 |

---

## 10. 一句话建议

**先跑一次 `make -j16 && make check` 确认基线绿灯**，再动任何代码。
本仓库所有「看起来只是个小改动」都曾经导致过 ROM 不一致或链接失败，
原因都记在 AGENTS.md §5.1 和本文 §2.3 里。
