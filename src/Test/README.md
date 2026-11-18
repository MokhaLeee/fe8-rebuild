# src/Test —— testmenu 与测试项组织

> 状态：**框架已落地（M5 阶段第一步）**。
> 顶层 `Start Game` / `Test`；`Test` → `System` → `Build Info` 已经是一条完整的
> 「分类 → case → 返回」链路，其余分类按下面的约定继续加。

## 1. 为什么单独做一套菜单

`tools/emu` 要在无人值守下验证「游戏还能不能正常跑」。固定帧号按键脚本一旦
流程变化几帧就失效，所以测试统一从 testmenu 进：菜单层级是稳定的，按键脚本按
菜单结构写，emu 通过 **一个 EWRAM 探针** 读状态，不依赖画面比对。

- 正常构建（`make`）保持与官方 ROM 逐位一致，`make check` 仍然有效。
- 测试构建（`make TEST=1`）才编进 `src/Test/`，开机直接进 testmenu。

## 2. 目录

```
src/Test/
├── README.md                 # 本文件
├── testmenu/                 # 菜单外壳
│   ├── testmenu.c            # 开机入口 + 顶层菜单（Start Game / Test）
│   ├── testmenu_common.c     # 菜单切换、图层清理等公共逻辑
│   └── testmenu_test.c       # Test 第二层：分类入口（目前 System）
└── harness/
    └── test_probe.c          # EWRAM 探针实现
```

公共头文件在 `include/hacks/`：`testmenu.h`、`testmenu_common.h`、`test_probe.h`。

后面按需增加 `cases/<领域>/`（每个领域一个 `MenuDef` + 回调）和 `fixtures/`
（测试专用章节事件 / 单位表）。先在第二层挂分类，再往分类里加 case。

## 3. 编译开关

```bash
make                    # 正常构建 → fireemblem8.gba，make check 有效
make TEST=1             # 测试构建 → fireemblem8-test.gba / .elf / .map / .sym
```

- `make TEST=1` 只是追加 `-DCONFIG_USE_DEBUG=1`；`include/configs/configs.h`
  用它打开 `configs/config-debug.h`，后者把 `CONFIG_TEST_MENU` 置 1。
- 两档共用同一批 `.o`。切换档位时 Makefile 在解析阶段删掉 C 对象（`.build-flavor`
  记录当前档位），避免链出「半新半旧」的 ROM。
- `make check` 在 `TEST=1` 下直接报错退出——测试构建本来就不与基线一致。

## 4. 探针

`struct TestMenuProbe`（`include/hacks/test_probe.h`）固定放在
**EWRAM `0x0203FF30`**，字段含义：

| 偏移 | 字段 | 说明 |
|---|---|---|
| 0x00 | `magic` | `0x54535450`（"TSTP"），`test_probe_reset()` 写入 |
| 0x04 | `menu` | `NONE` / `TOP` / `ROOT` / `CATEGORY` / `CASE` |
| 0x05 | `state` | `NONE` / `MENU` / `CASE_RUNNING` / `CASE_DONE` / `CASE_FAILED` |
| 0x06 | `category` | 分类 id（`TEST_CATEGORY_*`） |
| 0x07 | `case_id` | 分类内的 case id（`TEST_CASE_*`） |
| 0x08 | `step` | 多步 case 的步骤号 |
| 0x0A | `flags` | case 专用标志位 |
| 0x0C | `value[4]` | case 专用数值 |
| 0x1C | `frame` | 最近一次状态变化时的 `GetGameClock()` |

约定：

- 菜单层只写 `menu` / `state`；进入 case 时用 `test_probe_set_case()`，
  结束时用 `test_probe_finish(pass)`。
- 探针里不放指针、不放循环变量，只放 emu 需要断言的标量。

## 5. 布局（两个量身定做的位置）

test 构建的代码和数据放在两处，normal 构建不占用任何字节：

| 用途 | 位置 | 说明 |
|---|---|---|
| 代码 / 只读数据 | ROM `0x08EFB2E0..0x08FE0000` | banim 调色板之后的空洞，原版是 `FILL(0xFF)` 空白，约 914 KB |
| 可变数据（`EWRAM_DATA`） | EWRAM `0x0203FF00..0x02040000` | `gLoadUnitBuffer` 之后；代价是它的可用空间从 0x1048 缩到 0xF48 字节 |

两边都在 `lds/ldscript.txt` 里用 `src/Test/*.o(...)` 通配收集，配合文件末尾的
`/DISCARD/`：normal 构建没有这些对象，通配匹配不到输入段，不占地址。

## 6. 菜单切换的规矩

`uimenu.c` 的菜单 proc 在自己的回调返回后才会退出。在回调里直接 `StartMenu`
会让两个菜单 proc 同时存在并抢输入，所以一律走
`testmenu_defer_menu()`：先记下目标菜单，下一帧再由
`ProcScr_TestMenuOpen` 打开。

- 顶层菜单由 `onInit` 负责画标题、起两张脸，`onEnd` 负责收脸。
- 子菜单返回上一级也用同一个延迟机制，不要手写 `StartMenu`。
- 新增分类：在 `testmenu_test.c` 里加一条 `MenuItemDef` + 一个 `MenuDef`，
  在 `test_probe.h` 里加 `TEST_CATEGORY_*` / `TEST_CASE_*` 枚举。

## 7. 视觉约定

- 边框画在 BG2（字符基址 0，调色板 1），文字画在 BG0（调色板 0），
  底层 BG3 是滚动的 mural。
- 顶层两侧头像：Eirika（左）/ Ephraim（右），与 FE8 自己的 debug 继续菜单一致。
- 标题文字用独立字库（`BGCHR_TESTMENU_TITLE`），避免和菜单项共用
  `gDefaultFont` 的 `chr_counter`。

## 8. 验证

```bash
make TEST=1
./tools/emu/testmenu-tests.sh          # 26 项断言全绿
./tools/emu/testmenu-tests.sh --list   # 列出 case
make                                   # 切回正常构建
make check                             # fireemblem8.gba: OK
```
