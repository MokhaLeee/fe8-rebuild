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
make -j16 && sha1sum -c checksum.sha1   # 期望：fireemblem8.gba: OK
```

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
- 字符串和代码里不要使用日文全角空格（U+3000）；需要空格时用 ASCII 空格。
- 范围：适用于**新增代码和改动过的代码**。全仓库重排属于 M3 的批量任务，
  必须按下述方式分批进行，不要顺手格式化无关文件：
  - 每批 20–40 个文件，一批一次字节门禁（见 0.1）。
  - 含 `asm(` / `INCBIN` / 内联汇编块的文件进例外清单，单独处理。
  - 数据表文件（`src/data_*.c` 等）用 `/* clang-format off */` 保护表体。
  - 纯格式化提交记入 `.git-blame-ignore-revs`。

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

- **未经用户明确许可，禁止任何改变历史的操作**，包括但不限于：
  `git commit`、`git commit --amend`、`git push`、`git reset`、`git rebase`、
  `git revert`、`git cherry-pick`、`git tag`、`git stash drop/pop`。
- 用户说「提交 / commit」才算许可；询问、建议、沉默、说「继续做功能」都**不算**许可。
- **提交前必须先停一次**：输出「待提交文件清单 + `git diff --stat` + 完整提交信息」，
  然后结束本轮回复，等用户下一条明确回复。不允许在同一条回复里既列清单又执行。
- 即使上一条用户消息里出现过「提交 / commit / 做吧」等字样，只要不是对我列出的
  清单的确认，就**不算**授权；宁可再问一次。
- 只读的 `git status` / `git diff` / `git log` / `git show` / `git tag -l` 随时可用。
- 提交信息用英文，正文说清「为什么」而不是「改了哪些文件」。

## 3. 改动纪律

- 改代码前先读相关文件和上下文，不凭猜测改。
- 只做被要求的事：不顺手格式化、不重构无关代码、不删除或移动用户文件、
  不回滚未提交改动。
- 破坏性操作先确认，特别是**禁止**：
  - `git clean`（含 `-dfx`）——会清掉所有未跟踪文件
  - `make clean_all` ——本仓库该目标就是 `git clean -dfx`
- 不擅自新增依赖或升级工具链，除非用户要求。

## 4. 构建与验证

- 环境：`DEVKITPRO` / `DEVKITARM` **未设置也能构建**；实际使用系统
  `arm-none-eabi-*`（当前 2.34 binutils / gcc 9.2.1）加仓库自带的
  `tools/agbcc`。不要以为缺 `DEVKITARM` 就跑不起来。
- 构建入口：
  - `make -j16` —— 当前默认目标是 `compare`（构建 + 校验 sha1）；
    M2 之后改为默认 `all`、校验并入 `make check`。
  - 产物：`fireemblem8.gba` / `.elf` / `.map`（M2 后改名 `fe8re.*`）。
  - 默认**不生成** `.sym`（M5 补）。
- `baserom.gba`（16 MiB）是构建必需输入，**按设计不入库**（decomp 惯例），
  但本地必须存在；`Makefile` 的 `clean_all` 已带 `-e baserom.gba` 排除。
- 涉及 ROM 行为的改动，优先用 `tools/emu`（M5 落地后）验证；
  当前阶段用 0.1 的字节门禁。
- 汇报时写清楚：跑了什么、结果如何、没跑什么及原因。

## 5. 当前阶段与禁区（M1 期间）

阶段划分见 [`docs/rebuild-plan.md`](docs/rebuild-plan.md)。在 M1–M3 期间：

| 允许 | 禁止 |
|---|---|
| 改文档、脚本、`Makefile` | 合并 / 拆分 `src/*.c`（属于 M4b） |
| 按批次做风格重排（M3） | 全局一次性重排所有文件 |
| 新增 `include/` 下的头文件 | 改动既有 C 代码的语义 |
| 建目录骨架 | 移动 `src/*.c` 而不改 `ldscript.txt`（属于 M4a，且有脚本） |

**`ldscript.txt` 的硬约束**（M4a 之前一律不动；M4a 之后只改路径、不改顺序）：

- 该文件按**对象文件粒度**定位，1,031 处 `src/xxx.o(.text)` 把每个目标文件钉死在地址上。
- 对象出现的**顺序即地址**。移动文件时必须同步改路径并保持顺序，否则 ROM 不再一致。
- 新增 `.c` 文件必须同时在 `ldscript.txt` 里安排位置，否则会被 `/DISCARD/` 丢掉。

其他约定：

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
