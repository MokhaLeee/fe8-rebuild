# AGENTS.md — FE8-rebuild 工作约定

本文件对本仓库（含任意子目录）启动的所有会话生效，与全局 `~/.dsh/AGENTS.md` 叠加；
两者冲突时，以本文件更具体的约定为准。

规划与阶段划分见 [`docs/rebuild-plan.md`](docs/rebuild-plan.md)，
构建基线见 [`docs/baseline.md`](docs/baseline.md)。

---

## 0. 宪法（最高优先级，覆盖本文件其余条款）

### 0.1 字节门禁（M1–M4a 期间最高优先级）

本仓库当前处于「与 ROM 字节一致」的重构阶段。基线：

- 提交 `a53d6aa`，tag `decomp-baseline`
- `fireemblem8.gba` sha1 = `c25b145e37456171ada4b0d440bf88a19f4d509f`

**任何改动之后都必须仍然满足：**

```bash
make -j16 && make check   # 期望：fireemblem8.gba: OK
```

- 门禁的 sha1 常量在 `Makefile` 的 `ROM_SHA1`（基线值
  `c25b145e37456171ada4b0d440bf88a19f4d509f`）。换基线时两处同步改：
  `Makefile` 与 `docs/baseline.md`。
- 该门禁使用到 **M4b（合并代码）之前**，之后由 `tools/emu` 行为测试接替。
- 干净树全量构建约 **46 秒**（`-j16`），没有理由跳过。
- 只有纯文档 / 纯脚本改动可以不跑构建，但汇报里必须写明「未跑构建及原因」。

### 0.2 除法 / 取模（从 M6 起生效）

- **现状（M6 之前）**：全仓库由 agbcc 编译，除法由 `tools/agbcc/lib/libgcc.a` 的
  `__udivsi3` 系列提供，正常写 `/`、`%` 即可，**不要**为了这条规则去改既有代码。
- **从 M6（迁移到 `arm-none-eabi-gcc`）起**，按下述规则：
  - `/` 改用 `k_udiv(a, b)`，`%` 改用 `k_umod(a, b)`
    （接口 `include/hacks/klib.h`、实现 `src/Common/klib/math.c`，随 M6 一起落地）。
  - 唯一例外：除数 / 模数是 **2 的幂常量**（`/2`、`/4`、`/8`、`%8` 等）时可直接写，
    编译器会生成 `lsr` / `and`，不产生库调用。
    **其他常量除数不算例外**：`/10`、`/100`、`/3` 在 `arm7tdmi` + Thumb 下
    仍会生成 `bl __aeabi_uidiv`；除以 10 用 `k_udiv10` / `k_umod10`，其余用 `k_udiv` / `k_umod`。
  - klib 接口是 `u32`：运行期操作数应确认是无符号 32 位；64 位用 `k_u64div()` / `k_u64mod()`。
  - 有符号数自行处理符号，例如 `-k_udiv(-x, n)`；禁止把负数直接传给无符号接口。
  - 除数 / 模数为 0 时 klib 返回 0，不要依赖该行为做分支判断。
- 判断口诀：**2 的幂常量 → 直接写；其余一律 `k_udiv` / `k_umod`（10 用 `k_udiv10`）。**
- 原因：`tools/agbcc/lib/libgcc.a` 只有 `__udivsi3`，没有 `__aeabi_uidiv` /
  `__aeabi_uidivmod`；devkitARM gcc 编译出的运行期除法 / 取模会引用后者而链接失败。
- 自查：`arm-none-eabi-nm fireemblem8.elf | grep '__aeabi_.*div'` 无输出
  （M2 改名后为 `fe8re.elf`）。

### 0.3 代码风格：Linux kernel style

以 Linux 内核 `Documentation/process/coding-style.rst` 为准，重点：

- 缩进一律用 **Tab（8 列）**，不要用空格缩进；续行也用 Tab 对齐。
- 大括号 K&R：函数左大括号独占一行；`if` / `for` / `while` / `switch`
  的左大括号与语句同行，`} else {` 写在同一行；`switch` 的 `case`
  与 `switch` 对齐，case 体再缩进一层。
- 指针星号贴近变量名（`struct Foo *bar`），运算符两侧留空格。
- 单条语句不加多余大括号；一行只做一件事。行宽不做强制要求，
  长行、长字符串保持原样即可，不要为了 80 列硬折。
- 注释优先 `/* ... */`；新增内部符号用 `lower_snake_case`，调用既有
  `CamelCase` API 时保持原名。
- **局部变量声明之后必须空一行再接代码**（`checkpatch.pl` 的
  `WARNING: Missing a blank line after declarations`）。注意这条规则**不在**
  `coding-style.rst` 正文里，只由 checkpatch 检查，容易漏：
  ```c
  void foo(void)
  {
  	int i;

  	for (i = 0; i < n; i++)
  		bar(i);
  }
  ```
- 字符串和代码里不要使用日文全角空格（U+3000）；需要空格时用 ASCII 空格。
- 范围：适用于**新增代码和改动过的代码**。全仓库重排属于 M3 的任务，规则如下。

#### 0.3.1 M3 禁止使用 clang-format（硬性）

**不允许**用 clang-format（或任何自动重排工具）批量处理本仓库源码。已在 2026-11-17
实测并回退两批，原因是它会改变语义或破坏代码形态：

1. `i < sp08 >> 0x13` 被重排成 `i<sp08> > 0x13` —— 移位被误判成模板参数，
   agbcc 直接报 `syntax error before '>'`。
2. GCC 语句表达式 `({(sp04 * x >> 0x14) + 1;})` 被当成初始化列表，
   改写成 `({ (sp04 * x >> 0x14) + 1; })`，agbcc 同样拒绝。
3. 宏前缀声明 `EWRAM_OVERLAY(banim) struct Anim *gAnims[4] = { NULL };`
   会被拆成两行（由 `ColumnLimit: 0` 触发，换大数字可避免，但前两条无法避免）。

**校验方法的坑**：不能用「剔除空白后比较」来验证重排是否安全——`>>` 和 `> >`
剔除空白后完全相同，最危险的一条恰好查不出来。唯一可信的门禁是编译 +
`make check`（机器码逐位一致）。

#### 0.3.2 手工重排的做法

- **一个文件一个文件**，逐行读、逐行改，改完读一遍 diff 再继续。
- 按行数从小到大推进，一批 20–30 个文件；每批跑一次 `make check`。
- 只改这些：缩进 Tab、大括号位置、运算符/关键字空格、指针星号位置。
- **不动**：控制流结构（加不加 `else`）、符号名、字符串内容、行宽
  （长行保持原样，不要为了列宽硬折）、`asm` / `INCBIN` 块内部。
- 单条语句去掉多余大括号（kernel style 本意），但不得为此调整分支逻辑。
- 含 `asm(` / `INCBIN` 的文件最后单独处理，不要混进普通批次。

#### 0.3.3 本阶段**不删多余大括号**（实测结论）

kernel style 要求单语句分支不加大括号，但**本仓库在 M3 阶段必须保留**。

实测（2026-11-17，`src/mu.c` 的 `ResetMuAnims`）：把

```c
if (sMuConfig[i].slot != 0) { ResetSpriteAnimClock(...); }
```

改成

```c
if (sMuConfig[i].slot != 0)
	ResetSpriteAnimClock(...);
```

之后 agbcc 生成的机器码**不同**（寄存器分配改变，`push {r4,r5,r6,r7,lr}` 变成
`push {r4,r5,lr}`），ROM 不再与基线一致。

- **M3 期间**：大括号原样保留，只改位置（Allman → K&R）。
- **M4b 之后**（已放弃字节一致、改为行为测试）：可以按 kernel style 去括号。
- 已经去过括号且通过 `make check` 的批次是安全的，不需要回退。

判断口诀：**动大括号位置安全，删大括号不安全。**

#### 0.3.4 大表文件例外（暂不重排）

以下 4 个文件是纯数据表，**保持上游原始缩进**，不在 M3 处理：

| 文件 | 行数 | 说明 |
|---|---|---|
| `src/data_characters.c` | 6,765 | 角色表 |
| `src/data_classes.c` | 7,062 | 职业表 |
| `src/data_terrains.c` | 7,415 | 地形表 |
| `src/events_udefs.c` | 75,118 | 事件 UDEF 表 |

原因：这几个文件的缩进本身就是.**4 空格（表项）与 8 空格（字段）混用**，
自动转换会得到一半 Tab 一半空格的混合体；全量重排会产生约 20 万行 diff，
而数据表本身已经在 M4b 的数据整理阶段要重新组织，此处重排没有收益。

要处理的话，等 M4b 拆分/重写这些表时按新结构一次性生成。

### 0.4 提交署名

凡由 AI 代理生成或参与的 commit，都加上署名：

```
Co-Authored-By: DeepSeek-V4.1-Flash <noreply@deepseek.com>
```

---

## 1. 思考与表达

- 思考、分析、解释、总结一律使用**中文**；代码、路径、命令、日志、标识符保持原文。
- 先给结论，再给必要依据；不复述用户已经知道的内容。

## 2. 提交与版本控制（硬性约束）

### 2.1 默认禁止提交（2026-11-17 用户指令）

**从 2026-11-17 起，未经用户对当次提交的明确许可，一律不执行 `git commit`。**

具体含义：

- 不做「顺手提交」「阶段性提交」「为了干净先提交一下」。
- 完成工作后把改动留在工作区，在汇报里说明「未提交」。
- 即使前一次得到过提交许可，也不延续到下一次。
- 用户说「继续」「下一步」「做吧」**不构成**提交许可。

### 2.2 提交的正确流程

1. 输出「待提交文件清单 + `git diff --stat` + 完整提交信息」；
2. **结束本轮回复**，等用户下一条明确回复；
3. 只有在用户明确同意**这一份清单**之后才执行；
4. 不允许在同一条回复里既列清单又执行。

### 2.3 其他禁止项

- **未经用户明确许可，禁止任何改变历史的操作**：
  `git commit --amend`、`git push`、`git reset`、`git rebase`、
  `git revert`、`git cherry-pick`、`git tag`、`git stash drop/pop`。
- 只读的 `git status` / `git diff` / `git log` / `git show` / `git tag -l` 随时可用。
- 提交信息用英文，正文说清「为什么」而不是「改了哪些文件」。

## 3. 改动纪律

- 改代码前先读相关文件和上下文，不凭猜测改。
- 只做被要求的事：不顺手格式化、不重构无关代码、不删除或移动用户文件、
  不回滚未提交改动。
- 破坏性操作先确认，特别是**禁止**：
  - `git clean`（含 `-dfx`）——会清掉所有未跟踪文件
  - 用一个 `git clean` 目标替代 `make clean`（`clean_all` 已于 M2 移除）
- 不擅自新增依赖或升级工具链，除非用户要求。

## 4. 构建与验证

- 环境：`DEVKITPRO` / `DEVKITARM` **未设置也能构建**；实际使用系统
  `arm-none-eabi-*`（当前 2.34 binutils / gcc 9.2.1）加仓库自带的
  `tools/agbcc`。不要以为缺 `DEVKITARM` 就跑不起来。
- 构建入口：
  - `make -j16` —— 默认目标 `all`，只构建 `fireemblem8.gba` / `.elf` / `.map`。
  - `make check` —— 字节门禁，比对 `Makefile` 里的 `ROM_SHA1`。
  - `make clean` / `make clean_fast` —— 清理。
  - 默认**不生成** `.sym`（M5 补）。
- 构建脚本在 `tools/scripts/`；`tools/extract/` 是反编译提取脚本归档，**不参与构建**。
- `baserom.gba`（16 MiB）是构建必需输入，**按设计不入库**（decomp 惯例），
  但本地必须存在。工具二进制同样未入库，首次构建前跑 `./build_tools.sh`。
- 涉及 ROM 行为的改动，优先用 `tools/emu`（M5 落地后）验证；
  当前阶段用 0.1 的字节门禁。
- 汇报时写清楚：跑了什么、结果如何、没跑什么及原因。

## 5. 当前阶段与禁区

阶段划分见 [`docs/rebuild-plan.md`](docs/rebuild-plan.md)。M0–M4a 已完成，
当前处于 **M4b 之前**：字节门禁仍然有效。

| 允许 | 禁止 |
|---|---|
| 改文档、脚本、`Makefile` | 合并 / 拆分 `src/*.c`（属于 M4b，会破坏字节一致） |
| 在现有模块目录内新增文件 | 移动文件而不改四个链接脚本 |
| 新增 `include/` 下的头文件 | 改动既有 C 代码的语义 |

### 5.1 目录与链接脚本（M4a 之后）

`src/` 已按 FE8 模块域分到子目录（`Battle/` `Boot/` `Common/` `Core/*` `Data/`
`Debug/` `MapAnim/` `UI/`）。移动或新增 `.c` 时必须同时处理以下几件事，
否则 ROM 会不一致或链接失败：

1. **四个链接脚本**都要改（不是三个）：
   - `ldscript.txt`
   - `linker_script_sound.txt`（被 `INCLUDE` 进 ldscript）
   - `linker_script_banim.txt`
   - `sym_iwram.txt`（被 `INCLUDE`，含 `src/*.o(.bss)`）
   只能做字符串替换，**绝不能改变行序**——行序即地址。
2. **Makefile 的逐文件覆盖**要跟着改：`src/Boot/agb_sram.o` 用 `-O1`、
   `src/Boot/m4a.o` 用 `CC1_OLD`、`src/Core/Map/bmitem.o` 与 `src/UI/menu_def.o`
   加 `-Wno-error`。路径不命中会静默退回默认选项，代码生成随之改变
   （实测 `agb_sram.o` +0x18、`m4a.o` +0x10，后面全部平移）。
3. **清理陈旧产物**：`git mv` 不搬 `.o`/`.s`。判断标准是「旁边没有同名 `.c`
   的 `.o` 或 `.s` 就是陈旧产物」——但**排除** `src/rom_header.s`、`src/crt0.s`、
   `src/m4a_1.s`、`src/libagbsyscall.s` 这四个手写汇编源（它们是源码）。
4. **`CFILES` 用 `find` 时注意顺序**：链接顺序影响少数未显式定位的段。
   当前用 `$(shell find ...)`，若发现字节不一致可改用按文件名排序。

定位这类问题的有效手段：`git archive HEAD | tar -x -C /tmp/base` 导出已提交版本
单独构建，diff 两张 `fireemblem8.map`，直接看出哪个对象的段尺寸不对。

### 5.2 其他约定

- 源码一律 UTF-8；构建管道会 `iconv -f UTF-8 -t CP932` 再喂给 agbcc，
  改编码会导致编译失败或乱码。
- 不要删 `tools/agbcc/`（M6 之前构建依赖它）。
- 报告制度（日报 / 周报 / 月报）**尚未启用**；启用与否由用户决定，代理不得自行新建。

**`.clang-format` 的目录覆盖坑**：

- `include/.clang-format` 是上游 decomp 自带的（Allman + 4 空格 + 不缩进 case），
  与 §0.3 的内核风格直接冲突，且它会**覆盖仓库根目录的 `.clang-format`**。
- 格式化 `include/` 下任何文件前必须先处理它（M2 清理清单里已列）。
- `tools/textencode/.clang-format` 只作用于该 C++ 工具，无冲突，保留。

## 6. 汇报与收尾

- 完成任务后说明：改了哪些文件、为什么、如何验证；不要自行提交。
- 不修改本文件，除非用户明确要求。
