# tools/emu —— 无头 GBA 测试

用 mGBA 的核心库直接跑 ROM，不开窗口、不装模拟器，在纯命令行的 Linux 上验证
「改完之后游戏还能正常启动、进 testmenu、菜单导航和 case 都能跑通」。

不需要 root，也不需要 SDL / X11 / VNC。

## 一次性准备

```bash
./tools/emu/build.sh
```

首次会下载 mGBA 0.10.5 源码并只编译核心库（关掉 Qt/SDL/libretro），大约 2~3 分钟；
产物在 `tools/emu/.build/`（已 gitignore），之后 `build.sh` 只重编 `gbarun`，
耗时约 1 秒。依赖：`gcc cmake ninja curl zlib1g-dev libpng-dev libelf-dev`。

## 跑测试

```bash
make TEST=1                        # 先出测试构建 + .sym
./tools/emu/testmenu-tests.sh      # 全部 case
./tools/emu/testmenu-tests.sh --list
./tools/emu/testmenu-tests.sh --case 3
./tools/emu/testmenu-tests.sh --clean
```

退出码 0 = 全过，非 0 = 有失败项（逐条列出）。日志和截图留在
`tools/emu/.build/out-testmenu/`。

| case | 断言 |
|---|---|
| 1 top-menu | 开机进 testmenu；探针 magic/menu=TOP/state=MENU；画面非黑 |
| 2 test-root | 顶层 → Test → Back → 顶层；探针 menu 1→2→1 |
| 3 build-info | Test → System → Build Info → 一路 Back；探针走完 CATEGORY→CASE(DONE)→…→TOP |
| 4 start-game | 顶层选 Start Game；探针清零并回到正常开局流程 |
| 5 test-chapter | Test → Game → Test Chapter；`gPlaySt.chapterIndex = CHAPTER_TEST`，地图画面非黑 |

新增 case 的方法：在 `tools/emu/testmenu/cases/` 加一个 `case_NN_xxx()` 函数，
在 `testmenu-tests.sh` 的四个数组里登记；断言用 `lib.sh` 里的
`assert_probe` / `assert_screen` / `assert_no_crash`。

## 手动跑

```bash
GB=./tools/emu/.build/gbarun

# 跑到第 200 帧，截图 + 画面统计
$GB fireemblem8-test.gba --quiet --frames 240 --shot 200 --stats 200 \
    --sym fireemblem8-test.sym --out /tmp/shots

# 第 120 帧按 DOWN，第 180 帧按 A，看第二层菜单
$GB fireemblem8-test.gba --quiet --frames 480 \
    --hold 120:DOWN:6 --hold 180:A:6 --stats 250
```

### 参数

| 参数 | 说明 |
|---|---|
| `--frames N` | 跑多少帧（60 帧 = 1 秒游戏时间） |
| `--hold F:KEYS:LEN` | 第 F 帧起按住 KEYS 共 LEN 帧 |
| `--hold-every F0:F1:STEP:KEYS:LEN` | 从 F0 到 F1，每 STEP 帧按一次 |
| `--press-when "条件;KEYS;LEN[;间隔]"` | 状态满足时按键（比固定帧号稳） |
| `--shot F` / `--stats F` | 第 F 帧存 PNG / 打印画面统计 |
| `--hash F` / `--hash-every N` | 打印画面哈希，用于二分定位 |
| `--probe "SYM:WIDTH"` | 在每个 `--stats` 帧打印该地址的值 |
| `--sym FILE` | 符号表（`tools/extract/elf2sym.py` 或 nm 格式） |
| `--sav FILE` / `--save-out FILE` | 载入 / 导出 SRAM（32 KB） |
| `--gdb` / `--gdb-at N` | 挂 GDB stub，配合 `gdb-multiarch` 用 |
| `--out DIR` | 截图输出目录 |

条件语法 `<addr>:<width>:<op><value>`：`addr` 可以是符号名、`符号名+0xNN` 或
`0xNNNNNNNN`；`width` 取 8/16/32；`op` 取 `=` `&` `!` `>` `<`。多个条件用逗号
连接表示 AND。

## 出问题时怎么定位

- 崩溃：`gbarun` 打印 `CRASH signal=.. frame=..`，直接给帧号。
- 黑屏：`--stats` 看 `colors`（黑屏只有 1 种颜色），`--shot` 看具体画面。
- 行为变了：`--hash-every 30` 对比改动前后从第几帧开始分叉。
- 菜单状态不对：`--probe "gTestMenuProbe+0x4:8"` 等，字段含义见
  [src/Test/README.md](../../src/Test/README.md)。

## 清理

```bash
./tools/emu/testmenu-tests.sh --clean   # 只清测试输出
./tools/emu/build.sh clean              # 删掉整个 .build（约 140 MB）
```

注意 `make clean` **不会**动 `tools/emu/.build`，两者互不影响。

## 已知限制

- 没有真机 BIOS，走 mGBA 的 HLE BIOS，BIOS 层行为与真机不完全等价。
- 音频没有校验：只验证画面和流程。
- 存档按 32 KB SRAM 处理（`CONFIG_SAVE_USE_SRAM` 形态）。
- 目前覆盖 testmenu 框架、一个自检 case 和测试章节地图；存档、战斗、
  准备画面等流程的 case 随功能逐步补。
- 测试章节里的蓝方单位加载还有 EWRAM 覆盖问题，见
  [src/Test/README.md](../../src/Test/README.md) §9。
