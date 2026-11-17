# 构建基线（M0 固化）

- 记录时间：2026-11-17 14:42 +0800
- 基线提交：`a53d6aa354b3e2c1e4e0d9add99b9830538403bc`（`first commit`）
- 基线 tag：`decomp-baseline`
- 参考规划：[rebuild-plan.md](rebuild-plan.md)

本文件是重构前的「可复现事实」。后续每个阶段的门禁都以这里的数字为对照。

---

## 1. 环境

| 项 | 值 |
|---|---|
| 主机 | `mokha@10.116.89.94`，Linux，16 核 |
| `DEVKITPRO` | 未导出（`/opt/devkitpro` 存在但构建不依赖） |
| `DEVKITARM` | 未导出 |
| 汇编/链接 | `arm-none-eabi-as` / `arm-none-eabi-ld` 2.34 |
| C 编译器 | agbcc（仓库自带 `tools/agbcc/bin/{agbcc,old_agbcc,agbcc_arm}`） |
| 系统 gcc | `arm-none-eabi-gcc` 9.2.1 20191025（当前未参与 agbcc 构建） |
| Python | 3.10.18 |
| 格式化 | `clang-format` 10.0.0 |

> 注：构建**不需要** `DEVKITARM`。`Makefile` 里 `TOOLCHAIN ?= $(DEVKITARM)` 为空时
> `export PATH := /bin:$(PATH)`，实际用的是系统 `arm-none-eabi-*`。
> `_site`/`docs`/`reports` 与构建无关。

## 2. 构建

```bash
make clean
make -j16
```

| 指标 | 值 |
|---|---|
| 结果 | 成功，`sha1sum -c checksum.sha1` → `fireemblem8.gba: OK` |
| 墙钟时间 | **45.8 s**（`-j16`，干净树） |
| 峰值内存 | 107 MB |
| 编译单元 | 2,796 个 `.o` |
| 编译告警 | 0（`-Werror` 生效；仅 2 条汇编器 `Warning`：`monstergen_data.s` 段属性、`data_opanim_gfx.s` 文件尾换行） |
| 构建日志 | 11,927 行 |

**结论：迭代成本很低。** 每批改完后跑一次完整 `make` 只需 46 秒，
所以阶段 M3（风格）、M4a（目录）完全可以做到「每批一次 sha1 门禁」。

## 3. 产物

| 产物 | 大小 | 说明 |
|---|---|---|
| `fireemblem8.gba` | 16,777,216 B (16 MiB) | sha1 `c25b145e37456171ada4b0d440bf88a19f4d509f` |
| `fireemblem8.elf` | 46,426,764 B | 含调试信息，283,908 个符号 |
| `fireemblem8.map` | 2,650,581 B | 链接映射 |
| `.sym` | — | **默认不生成**（decomp 无此目标，M5 需补） |
| `.dep/` | 1.5 MB | 依赖文件 |
| 工作树（不含 `.git`） | 409 MB | 构建后；清理后 95 MB |

`arm-none-eabi-size`：

```
   text    data     bss     dec     hex
16776960       0  875844 17652804 10d5c44
```

> `data` 为 0 是因为所有数据都落在 `.text`（ROM 区）、EWRAM 覆盖段与 IWRAM 段内，
> 由 `ldscript.txt` 显式定位。

## 4. 仓库状态

| 项 | 值 |
|---|---|
| 提交数 | 1 |
| tag | `decomp-baseline`（本次新建） |
| 分支 | `master` |
| remote | 无 |
| 跟踪文件 | 7,028 |
| `.git` 体积 | 14 MB |

### 4.1 `baserom.gba` 不入库（decomp 惯例，非缺陷）

```
$ git ls-files --error-unmatch baserom.gba
error: pathspec 'baserom.gba' did not match any file(s) known to git
```

`baserom.gba`（16 MiB）是**构建必需输入**（1,293 处 `.incbin`），但**按设计不进版本库**：
decomp 项目的 ROM 涉及版权，且体积大，上游一律要求用户自备。
`.gitignore` 里也没有显式条目——它只是从未被 `git add`。

`Makefile` 的 `clean_all` 已经考虑到了这点：

```make
clean_all:
	git clean -dfx -e baserom.gba
```

`-e baserom.gba` 显式把 ROM 排除在清理之外。**开发时不需要额外备份**，
只要本地这一份在即可；换机器时自备 ROM 是 decomp 的常规前提。

> 该目标已在 M2 移除：它的伤害面太大（未跟踪文件全清），且 `make clean` 已足够。

## 5. 回归门禁（后续各阶段使用）

| 门禁 | 命令 | 期望 |
|---|---|---|
| G1 字节一致 | `make -j16 && make check` | `fireemblem8.gba: OK` |
| G2 构建成功 | `make clean && make -j16` | exit 0 |
| G3 无告警 | 构建日志中 `warning:` 计数 | 0 |
| G4 除法 | `arm-none-eabi-nm fireemblem8.elf \| grep '__aeabi_.*div'` | 无输出（M6 起生效） |
| G5 行为 | `./tools/emu/testmenu-tests.sh`（M5 起） | 全绿 |

G1 使用到 **M4b（合并代码）之前**；之后由 G5 接替。

## 6. 已知遗留问题（不阻塞，记录备查）

| # | 现象 | 归属阶段 |
|---|---|---|
| 1 | `data_opanim_gfx.s` 末尾缺换行（汇编器自动补） | M3 顺手修 |
| 2 | `monstergen_data.s` 的 `.data` 段属性告警 | M3 顺手修 |
| 3 | 默认不生成 `.sym`，调试不便 | M5 |
| 4 | 无 CI，无 `make test` | M5 |
| 5 | ~~`Makefile` 默认目标是 `compare`，不是 `all`~~ | M2 已修 |
| 6 | ~~`clean_all` 目标执行 `git clean -dfx`~~ | M2 已移除 |
| 7 | `ldscript.txt` 对象级布局，阻碍任何文件移动 | M4a（路径改写）/ M4b（放松） |
