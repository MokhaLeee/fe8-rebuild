# M2 瘦身清理：逐项清单（待确认）

- 状态：**清单待用户逐项确认，尚未删除任何文件**
- 当前跟踪文件：7,028
- 当前根目录条目：36

本清单把清理分成三档：**A 删除**、**B 归档**、**C 保留/改写**。
每项都标注了「构建影响」——已实际 grep 过 `Makefile`、`make_tools.mk`、
`graphics_file_rules.mk`、`json_data_rules.mk`、`songs.mk`、
`graphics/banim/assets/img/banim_img_rules.mk` 以及脚本之间的 import 链。

---

## A. 建议删除（无后续价值）

| # | 路径 | 体积 | 构建影响 | 理由 |
|---|---|---|---|---|
| A1 | `_site/` | 8 KB / 1 文件 | 无引用 | GitHub Pages 文档站占位页 |
| A2 | `buddy.yml` | 4 KB | 无引用 | 上游 CI（Buddy）配置 |
| A3 | `CONTRIBUTING.md` | 12 KB | 无引用 | 上游 PR 规范，与重构无关 |
| A4 | `asmdiff.sh` | 4 KB | 无引用 | 反汇编比对脚本，7 行，`cmp -l` 包装 |
| A5 | `checksum.sha1` | 58 B | 仅 `Makefile:107` 引用 | 内容改为 Makefile 变量 `ROM_SHA1`，见 C1 |
| A6 | `include/.clang-format` | 1 文件 | 无引用 | **会劫持**根目录 `.clang-format`，Allman+4 空格与宪法冲突 |

合计减少 6 个条目 / 约 24 KB。

> A5 说明：`checksum.sha1` 本身是门禁数据，删文件不等于删门禁——
> 把 sha1 常量写进 `Makefile` 的 `ROM_SHA1` 变量，`make check` 直接比对。
> 好处是根目录少一个游离文件；坏处是换基线要改 Makefile 而不是改数据文件。
> **如果你倾向保留该文件，A5 可以撤销，其余不变。**

## B. 建议归档到 `tools/extract/`（不删，移出主视线）

### B1 `scripts/` 里 108 个条目（保留 20 个构建必需）

构建必需闭包（**必须留在 `scripts/` 或按 C2 迁移**，20 个文件）：

```
scripts/arm_compressing_linker.py     # banim 链接
scripts/compressor.py                 # 被上面 os.system 调用
scripts/mar_to_map.py                 # %.bin: %.mar
scripts/tmap2tsa.py                   # 地图 TSA
scripts/tsa_png.py                    # BG TSA（需 numpy + PIL）
scripts/update_banim_img_rules.py     # 生成 banim_img_rules.mk（非构建期运行）
scripts/gfxtools/*                    # tsa_generator.py 及其 import
scripts/texttools/*                   # textprocess / textdecoder / huffman
```

其余 108 个（约 5 MB）在 decomp 期用于提取素材，**不参与构建**：

- `dump_*.py` / `dump_*.sh` 系列（约 45 个）
- `extract_file.py`、`fe8db.py`、`symbols.py`、`find_nearest_symbol.py`
- `calcrom.pl` / `calcrom.sh` / `calcfunc.sh` / `cfunc_extractor.sh`
- `asm2casm.py`、`gen_split_data.py`、`combine_bgs.py`、`cvtdata2jis.*` 等

**为什么不直接删**：其中一批在 M4c（`contents/` 素材化 + incbin 外置）还要用：

| 文件 | M4c 用途 |
|---|---|
| `scripts/extract_file.py` + `graphic_extract.yml` | 按 yml 批量导出素材 |
| `scripts/dump_banim*.py`（约 12 个） | 战斗动画素材导出 |
| `scripts/gfxtools/lzss_*.py` | LZ77/LZSS 压缩分析 |
| `scripts/elf2sym.py` | **M5 生成 `.sym` 要用**（fe6 的 `tools/scripts/elf2sym.py` 同源） |
| `reports/*.csv` | banim img 像素尺寸映射，重新生成 `banim_img_rules.mk` 的输入 |

### B2 `reports/` 3 个 CSV（328 KB）

| 文件 | 被谁读 | 结论 |
|---|---|---|
| `reports/data_banim_asset_map.csv` | `scripts/update_banim_img_rules.py`（`ASSET_MAP_CSV`） | **不能直接删** |
| `reports/data_banim_incbin_labels.csv` | 同上（`LABELS_CSV`） | **不能直接删** |
| `reports/data_banim_incbin_report.txt` | 无人引用 | 可删 |

注意：`update_banim_img_rules.py` **不在构建期运行**——它的产物
`graphics/banim/assets/img/banim_img_rules.mk` 是入库的。
所以删 CSV 不会让 `make` 失败，只会让「重新生成 img rules」这一步失去输入。
=> 归档到 `tools/extract/reports/`，并同步改脚本里的路径常量。

### B3 `graphic_extract.yml`（820 KB）

只被 `scripts/extract_file.py` 读取，不参与构建。归到 `tools/extract/`。

### B4 `docs/` 里 5 篇 decomp 文档（40 KB）

| 文件 | 建议 |
|---|---|
| `Background_TSA_Report.md` | 归档 `docs/decomp/` |
| `Banim_AnimScr_Decompilation_Report.md` | 归档 |
| `Banim_TSA_Preservation_Report.md` | 归档 |
| `banim_asset_extraction.md` | 归档（M4c 有参考价值） |
| `quickstart.md` | 归档（环境搭建说明，M2 重写 README 时吸收） |

保留：`docs/rebuild-plan.md`、`docs/baseline.md`。

---

## C. 建议保留 / 改写

| # | 路径 | 处理 | 说明 |
|---|---|---|---|
| C1 | `Makefile` | **重写** | 默认目标 `compare` → `all`；`compare` → `check`（内嵌 `ROM_SHA1`）；移除 `clean_all`（`git clean -dfx`）与 `tag`（生成 ctags/cscope，与构建无关，可留但建议删） |
| C2 | `scripts/` 构建闭包 | **迁到 `tools/scripts/`** | 与 `tools/emu`、`tools/reports` 并列，`scripts/` 目录整体消失 |
| C3 | `compile_flags.txt` | **改写** | 现有内容有过期项（`-WParenheses` 拼错、缺 `-iquote`），clangd/IDE 用；不是删除对象 |
| C4 | `README.md` | **重写** | 改成开发向：构建、目录、测试、门禁 |
| C5 | `.gitignore` | **整理** | 979 行，按类别归并；`.dep` 与 `/.deps/` 重复 |
| C6 | `build_tools.sh` / `clean_tools.sh` / `make_tools.mk` | **保留** | 工具二进制**未入库**（`git ls-files tools` 只有源码），首次构建必须靠它 |
| C7 | `tools/{agbcc,gbagfx,bin2c,scaninc,aif2pcm,mid2agb,textencode,jsonproc}` | **保留** | 构建依赖 |
| C8 | `ldscript.txt` 等 3 个链接脚本 | **M4a 再动** | 本阶段不改，避免与路径改写互相干扰 |

---

## 预期结果

| 指标 | 现在 | 清理后 |
|---|---|---|
| 根目录条目 | 36 | 29（−8 项 + `tools/extract/`） |
| `scripts/` 目录 | 128 文件 / 5 MB | 消失（闭包进 `tools/scripts/`，其余进 `tools/extract/`） |
| 跟踪文件 | 7,028 | 7,021（A 档删 7 个；B 档只改路径，数量不变） |
| 构建 | sha1 OK | **必须仍然 sha1 OK** |

---

## 确认方式

按下面的方式回复即可，不用逐条打字：

- 「全按 A/B/C 做」——执行全部建议
- 「A5 保留，其余照做」——撤销某一项
- 「B 只归档 X/Y，其余删」——调整归档范围

**在你确认之前不会执行任何删除或移动。**
