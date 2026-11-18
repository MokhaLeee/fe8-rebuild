# Fire Emblem: The Sacred Stones —— 重构工程

从 [fireemblem8u](https://github.com/laqieer/fireemblem8u) decomp 起步的源代码级重构，
目标是让 GBA FE8 变成可以自由增改代码的项目。

当前状态：**与 ROM 字节一致**（重构前阶段），每次改动后用 `make check` 验证。

```
ROM_SHA1 = c25b145e37456171ada4b0d440bf88a19f4d509f
```

- 重构规划：[`docs/rebuild-plan.md`](docs/rebuild-plan.md)
- 构建基线：[`docs/baseline.md`](docs/baseline.md)
- 工作约定：[`AGENTS.md`](AGENTS.md)

---

## 构建

### 依赖

- `arm-none-eabi-*`（binutils + gcc）
- `python3`（构建脚本）
- `numpy`、`Pillow`（`tools/scripts/tsa_png.py`、`gfxtools/tsa_generator.py`）

`DEVKITPRO` / `DEVKITARM` **不需要设置**。

### 一次性准备

1. 把合法取得的 ROM 放到仓库根目录，命名为 `baserom.gba`（不入库）。
2. 编译工具链（二进制未入库）：

```bash
./tools/build_tools.sh
```

> agbcc 已随仓库提供（`tools/agbcc/`），无需额外安装。

### 日常

```bash
make -j$(nproc)    # 构建 fireemblem8.gba / .elf / .map
make check         # 字节门禁：sha1 必须等于 ROM_SHA1
make clean         # 清理
```

干净树全量构建约 **46 秒**（16 核）。

## 目录

| 目录 | 内容 |
|---|---|
| `src/` | 源码（359 个 `.c` + 6 个手写汇编：`Boot/` 4 个、`Common/` 2 个），agbcc 编译 |
| `include/` | 头文件；`constants/` 常量、`configs/` 编译开关、`hacks/` 新增接口、`macros/` 汇编宏 |
| `data/` | 数据段汇编（`.s`，含 `.incbin baserom.gba` 引用） |
| `contents/` | 素材源文件（PNG / PAL / TSA、MML / MID、音源）与生成物 |
| `texts/` | 文本表 |
| `tools/` | 构建工具（`agbcc`、`gbagfx` 等）+ `scripts/` 构建脚本 + `extract/` 反编译提取归档 |
| `docs/` | 文档；`decomp/` 存放 decomp 阶段报告 |
| `lds/` | 链接脚本（`ldscript.txt` + 3 个被 `INCLUDE` 的脚本，**对象级布局，见 AGENTS.md §5**） |
| `mk/` | make 规则片段（素材规则、音频规则） |

## 已知约束

- **`lds/ldscript.txt` 是对象级布局**：1,031 处 `src/xxx.o(.text)` 把每个目标文件钉死在固定地址。
  移动文件必须同步改路径并保持顺序；新增 `.c` 必须在脚本里安排位置，否则被 `/DISCARD/` 丢掉。
- **`baserom.gba` 是构建输入**：1,297 处 `.incbin` 引用它（约 1.07 MB）。这些原始数据块**暂不外置**——外置只是把 ROM 依赖换成文件依赖，数据仍是二进制；等数据整理阶段再评估。
- **源码必须 UTF-8**：构建管道会 `iconv -f UTF-8 -t CP932` 再喂给 agbcc。
- **目前不生成 `.sym`**，M5 补。

## 后续

见 [`docs/rebuild-plan.md`](docs/rebuild-plan.md) 的阶段划分（M0–M6）：
瘦身 → 代码风格 → 目录重组 → 素材外置 → 测试基建 → agbcc 迁移到 arm-none-eabi-gcc。
