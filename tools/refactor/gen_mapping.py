#!/usr/bin/env python3
"""生成 M4a 的 文件 -> 目录 映射表。

规则集中在 RULES 与 EXPLICIT 里，改这里就能调整归属。
用法：
    tools/refactor/gen_mapping.py            # 打印分组统计
    tools/refactor/gen_mapping.py --emit     # 输出 batch 文件到 /tmp/m4a/
"""

import argparse
import os
import re
import sys

# 精确指定（覆盖下面的前缀规则）
EXPLICIT = {
    # ---- Boot ----
    "main": "Boot", "irq": "Boot", "hardware": "Boot", "rng": "Boot",
    "time": "Boot", "agb_sram": "Boot", "ramfunc": "Boot", "sysutil": "Boot",
    "ap": "Boot", "proc": "Boot", "gamecontrol": "Boot", "animedrv": "Boot",
    "scene": "Boot", "face": "Boot", "ctc": "Boot", "m4a": "Boot",
    "m4a_tables": "Boot", "soundwrapper": "Boot",
    # ---- Core ----
    "chapterdata": "Core",
    # ---- Core/Map ----
    "bm": "Core/Map", "bmmap": "Core/Map", "bmarch": "Core/Map",
    "bmcamadjust": "Core/Map", "bmcontainer": "Core/Map", "bmdebug": "Core/Map",
    "bmgold": "Core/Map", "bmitem": "Core/Map", "bmmind": "Core/Map",
    "bmphase": "Core/Map", "bmreliance": "Core/Map", "bmshop": "Core/Map",
    "bmtarget": "Core/Map", "bmtrade": "Core/Map", "bmtrap": "Core/Map",
    "bmtrick": "Core/Map", "bmusemind": "Core/Map", "bmusort": "Core/Map",
    "bmxfade": "Core/Map", "bmudisp": "Core/Map", "bmidoten": "Core/Map",
    "bmpatharrowdisp": "Core/Map", "bmcommanddbg": "Core/Map",
    "bmdifficulty": "Core/Map", "bmdifficulty-ewram": "Core/Map",
    "bmguide": "Core/Map", "bmguide_config": "Core/Map", "bmarena": "Core/Map",
    "bksel": "Core/Map", "cursor_hand": "Core/Map", "fogmap": "Core/Map",
    "minimap": "Core/Map", "trapfx": "Core/Map", "spinning_arrow": "Core/Map",
    "unit_facing": "Core/Map", "playerphase": "Core/Map",
    "phasechangefx": "Core/Map", "mu": "Core/Map", "muctrl": "Core/Map",
    "koido": "Core/Map", "player_interface": "Core/Map",
    "bmreliance-data": "Core/Map",
    # ---- Core/Unit ----
    "bmunit": "Core/Unit", "bmitemuse": "Core/Unit", "bmusailment": "Core/Unit",
    "monstergen": "Core/Unit", "monstergen_data": "Core/Unit",
    # ---- Core/Save ----
    "savemenu": "Core/Save", "savedraw": "Core/Save",
    "savemenu_data": "Core/Save",
    # ---- Core/Prep ----
    "prepscreen": "Core/Prep", "prep_80A0760": "Core/Prep",
    # ---- Core/Ending ----
    "gameoverbgfx": "Core/Ending", "gamerankings": "Core/Ending",
    # ---- Battle ----
    "bb": "Battle", "bmbattle": "Battle", "cpextra_80407F0": "Battle",
    "banim_data": "Battle",
    # ---- MapAnim ----
    "danceringfx": "MapAnim", "emitstarfx": "MapAnim", "lightrunefx": "MapAnim",
    "minefx": "MapAnim", "whitecircularfadefx": "MapAnim",
    # ---- UI ----
    "uimenu": "UI", "uiutils": "UI", "uiconfig": "UI", "uidebug": "UI",
    "uiarena": "UI", "uichapterstatus": "UI", "uiselecttarget": "UI",
    "uisupport": "UI", "unitlistscreen": "UI", "statscreen": "UI",
    "statbar": "UI", "unitinfowindow": "UI", "menuitempanel": "UI",
    "menu_def": "UI", "helpbox": "UI", "popup": "UI", "popup2": "UI",
    "chapter_title": "UI", "chap_title_data": "UI",
    "classdisplayfont": "UI", "titlescreen": "UI",
    "mural_background": "UI", "extramenu_unk": "UI", "difficultymenu": "UI",
    "soundroom": "UI", "soundroom_data": "UI", "bonusclaim": "UI",
    "bonusclaim_helpbox": "UI", "cg": "UI", "cgtext": "UI",
    "convoymenu": "UI", "chapterintrofx": "UI", "chapterintrofx_title": "UI",
    "opanim-main": "UI", "opanimfx": "UI",
    "opanim-healthsafetyscreen": "UI", "opinfo": "UI", "opsubtitle": "UI",
    # ---- Common ----
    "bmlib": "Common", "bmlib-hblank": "Common", "bmlib-math": "Common",
    "fontgrp": "Common", "fontgrp-data": "Common", "icon": "Common",
    "msg": "Common", "spellassoc": "Common", "spellassoc-data": "Common",
    # ---- Debug ----
    "code_8086934": "Debug", "code_80AC6AC": "Debug", "fe3_dummy": "Debug",
    # ---- 补充 ----
    "colorfade": "Boot", "spline": "Common", "sprite-animfx": "Common",
    "portrait_data": "Common",
    "unit_icon_move_data": "Core/Map", "unit_icon_wait_data": "Core/Map",
    "msg_data": "Common",
}

# 前缀规则（EXPLICIT 未覆盖时按顺序匹配）
RULES = [
    (r"^data_", "Data"),
    (r"^banim", "Battle"),
    (r"^ekr", "Battle"),
    (r"^efx", "Battle"),
    (r"^cp_", "Battle"),
    (r"^classchg", "Core/Unit"),
    (r"^worldmap", "Core/WorldMap"),
    (r"^sio", "Core/Sio"),
    (r"^prep_", "Core/Prep"),
    (r"^mapanim", "MapAnim"),
    (r"^event", "Core/Events"),
    (r"^eventscr", "Core/Events"),
    (r"^events_script", "Core/Events"),
    (r"^eventinfo", "Core/Events"),
    (r"^bmsave", "Core/Save"),
    (r"^ending", "Core/Ending"),
    (r"^ui", "UI"),
    (r"^bm", "Core/Map"),
]


def classify(name):
    if name in EXPLICIT:
        return EXPLICIT[name]
    for pat, dst in RULES:
        if re.match(pat, name):
            return dst
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    args = ap.parse_args()

    names = sorted(os.path.basename(p)[:-2]
                   for p in __import__("glob").glob("src/*.c"))
    mapping, missing = {}, []
    for n in names:
        d = classify(n)
        if d:
            mapping[n] = d
        else:
            missing.append(n)

    from collections import Counter
    cnt = Counter(mapping.values())
    print("文件总数 %d，已归类 %d，未归类 %d" % (len(names), len(mapping), len(missing)))
    for d, c in sorted(cnt.items()):
        print("  %-18s %3d" % (d, c))
    if missing:
        print("未归类:", missing)
        return 1

    if args.emit:
        outdir = "/tmp/m4a"
        os.makedirs(outdir, exist_ok=True)
        batches = [
            ("1", ["Data"]),
            ("2", ["Debug", "Boot", "Common"]),
            ("3", ["Core/Save", "Core/Unit", "Core/Ending"]),
            ("4", ["Core/Prep", "Core/WorldMap", "Core/Sio"]),
            ("5", ["Core/Events", "MapAnim"]),
            ("6", ["UI"]),
            ("7", ["Battle"]),
            ("8", ["Core/Map", "Core"]),
        ]
        for idx, dirs in batches:
            lines = ["%s %s" % (n, d) for n, d in sorted(mapping.items()) if d in dirs]
            with open("%s/batch%s.txt" % (outdir, idx), "w") as f:
                f.write("\n".join(lines) + "\n")
            print("batch%s: %d files -> %s" % (idx, len(lines), dirs))
    return 0


if __name__ == "__main__":
    sys.exit(main())
