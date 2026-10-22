"""Regenerate the source configuration inventory without evaluating conditionals."""
import argparse
import pathlib
import re

parser=argparse.ArgumentParser()
parser.add_argument("--date",required=True)
args=parser.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
files=[]
for directory in ["application","bsp","modules","Inc","Src"]:
    files.extend(p for p in (root/directory).rglob("*") if p.suffix in [".c",".h"])
files.append(root/"Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h")
sections=[];entries=0
for path in sorted(files):
    raw=path.read_bytes()
    try: source=raw.decode("utf-8-sig")
    except UnicodeDecodeError: source=raw.decode("gb18030",errors="replace")
    rows=[];config_depth=0
    for number,line in enumerate(source.splitlines(),1):
        stripped=line.strip()
        if re.match(r"#\s*(define|if|ifdef|ifndef|elif|else|endif)\b",stripped):
            rows.append((number,"预处理",stripped))
        if re.search(r"\b\w*(?:Config|config|conf|PID|Setting)\w*\s+\w+\s*=\s*\{",line):
            config_depth=max(0,line.count("{")-line.count("}"))
            rows.append((number,"配置初值",stripped))
        elif config_depth:
            if stripped and stripped not in ["{","}","};"]:
                rows.append((number,"配置初值",stripped))
            config_depth+=line.count("{")-line.count("}")
        elif re.search(r"\.Init(?:\.\w+)*\s*=|HAL_NVIC_(?:SetPriority|EnableIRQ|DisableIRQ)\(",line):
            rows.append((number,"HAL/NVIC",stripped))
    if not rows: continue
    relative=path.relative_to(root).as_posix()
    sections.append(f"## {relative}\n\n| 行 | 类型 | 源码 |\n| --- | --- | --- |")
    for number,kind,value in rows:
        value=value.replace("|","&#124;").replace("`","&#96;")
        sections.append(f"| [{number}](../{relative}#L{number}) | {kind} | `{value}` |")
    sections.append("");entries+=len(rows)
header=f'''# 完整宏与初始化参数索引

更新日期：{args.date}。当前目标为 STM32F427 A 板。配套说明：[配置指南](configuration-guide.md)、[宏迁移表](atype-macro-migration.md)。

由 `scripts/generate-configuration-index.py` 扫描 {len(files)} 个项目 C/H 文件，记录 {entries} 条预处理、配置初始化和 HAL/NVIC 项。预处理条目包含互斥分支与保护宏，不能将所有条目同时理解为有效配置。可从构建命令覆盖的选项见 `config/atype-options.txt`；普通源码 PID/功率模型初值也列在所属文件下。

`modules/imu/ins_task.c`、`BMI088driver.c`、`BMI088Middleware.c` 是未链接的旧 C 板实现；当前 INS 只链接 MPU6500 和公共数学接口。便携副本有独立 API，未加入固件链接。

'''
(root/"docs/configuration-index.md").write_text(header+"\n".join(sections),encoding="utf-8",newline="\n")
print(f"Indexed {len(files)} files, {entries} entries")
