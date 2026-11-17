# tools/extract —— 反编译提取工具（归档）

本目录是 decomp 阶段用来从 `baserom.gba` 里提取素材、分析数据的一次性脚本。
**不参与构建**，`make` 不会碰这里。

## 为什么保留而不是删掉

其中有几项在 M4c（`contents/` 素材化 + incbin 外置）还要用：

| 文件 | M4c 用途 |
|---|---|
| `extract_file.py` + `graphic_extract.yml` | 按 yml 批量导出素材到 `graphics/` |
| `dump_banim*.py`（约 12 个） | 战斗动画素材导出 |
| `gfxtools/lzss_*.py`（在 `tools/scripts/gfxtools/`） | LZ77/LZSS 压缩分析 |
| `elf2sym.py` | **M5 生成 `.sym` 要用** |
| `reports/*.csv` | 重新生成 `banim_img_rules.mk` 的输入 |

## 目录说明

```
tools/extract/
├── dump_*.py / dump_*.sh     # 各类数据提取（角色、职业、道具、地图、立绘…）
├── reports/                  # banim 素材普查 CSV（被 tools/scripts/update_banim_img_rules.py 读）
├── graphic_extract.yml       # 820 KB，素材地址表，被 extract_file.py 读
├── compress_text/            # 文本压缩树（decomp 遗留）
├── match_samples/            # 音源匹配结果
├── dump_sound/               # C# 音源导出工具（未入库，gitignore）
└── 其他一次性脚本
```

## 注意

- 这些脚本大多依赖**仓库根目录**作为工作目录，且直接读 `baserom.gba`。
- 路径常量已随本次迁移更新（`reports/` → `tools/extract/reports/`，
  `graphic_extract.yml` → `tools/extract/graphic_extract.yml`）。
- 真正参与构建的脚本在 `tools/scripts/`，不要混淆。
