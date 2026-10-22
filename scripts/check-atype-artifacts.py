"""Check linked memory layout and required (retained) module implementations."""
import argparse
import json
import pathlib
import re
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument("build_dir",type=pathlib.Path)
parser.add_argument("--modules",action="store_true")
args=parser.parse_args()
elf=args.build_dir/"basic_framework.elf"
raw=subprocess.check_output(["arm-none-eabi-nm",str(elf)],text=True)
symbols={}
for line in raw.splitlines():
    match=re.fullmatch(r"([0-9a-fA-F]+) ([A-Za-z]) (\S+)",line.strip())
    if match: symbols[match[3]]=(int(match[1],16),match[2])
assert not re.search(r"^\s+U ",raw,re.M), "Undefined strong symbols"
for name,address in {"_estack":0x20030000,"__tuning_bank0_start__":0x081c0000,
                     "__tuning_bank1_start__":0x081e0000}.items():
    assert symbols[name][0]==address,(name,symbols.get(name))
for name in ["INS_Init","INS_Task","RobotTask","OTG_FS_IRQHandler","UART8_IRQHandler"]:
    assert symbols[name][1] in "Tt",name
sections=subprocess.check_output(["arm-none-eabi-objdump","-h",str(elf)],text=True)
flash_end=0x08000000
ram_end=0x20000000
for line in sections.splitlines():
    match=re.match(r"\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)",line)
    if not match: continue
    name,size,vma,lma=match[1],int(match[2],16),int(match[3],16),int(match[4],16)
    if size and 0x08000000<=lma<0x08200000:
        assert lma+size<=0x081c0000,(name,"overlaps parameter flash")
        flash_end=max(flash_end,lma+size)
    if name in [".bss",".data"]:
        assert 0x20000000<=vma and vma+size<=0x20030000,(name,"outside DMA-accessible RAM")
    if 0x20000000<=vma<0x20030000:
        ram_end=max(ram_end,vma+size)
if args.modules:
    root=pathlib.Path(__file__).resolve().parents[1]
    for name in (root/"config/atype-link-symbols.txt").read_text().splitlines():
        assert name in symbols and symbols[name][1] in "Tt",("module was discarded",name)
assert elf.with_suffix(".hex").exists() and elf.with_suffix(".bin").exists()
assert elf.with_suffix(".bin").stat().st_size<=1792*1024
print(json.dumps({"build":str(args.build_dir),"flash_image_bytes":flash_end-0x08000000,
    "static_ram_bytes":symbols["_ebss"][0]-symbols["_sdata"][0],
    "ram_with_reserved_heap_stack_bytes":ram_end-0x20000000,
    "module_roots_verified":args.modules}))
