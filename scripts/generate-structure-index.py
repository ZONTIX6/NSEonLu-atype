"""Build a source-level C struct/union dictionary using only the Python stdlib.

This is a lexical inventory, not a preprocessor: mutually exclusive branches
and literal macro templates are retained, and arbitrary macro calls are not expanded.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from dataclasses import dataclass, field
import os
from pathlib import Path
import re
import sys
import tempfile
from urllib.parse import quote


EXCLUDED = {".git", ".cache", "__pycache__", "node_modules", "build", "output",
            ".venv", "venv", ".idea", ".playwright-cli"}
LEXEMES = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\[\s\S]|[^"\\])*"|\'(?:\\[\s\S]|[^\'\\])*\'')
TOKENS = re.compile(r"[A-Za-z_]\w*|\d+|[^\s]")
IDENT = re.compile(r"^[A-Za-z_]\w*$")
KINDS = {"struct": "struct", "union": "union", "__PACKED_STRUCT": "struct", "__PACKED_UNION": "union"}
QUALIFIERS = {"const", "volatile", "restrict", "__restrict", "__IO", "__I", "__O",
              "__IOM", "__IM", "__OM", "__packed", "__packed__", "__PACKED"}
PRIMITIVES = {"void", "char", "short", "int", "long", "float", "double", "unsigned", "signed", "_Bool", "bool"}

# These are search hints by module, not invented descriptions of individual fields.
PURPOSES = [
    ("application/chassis", "底盘 运动 控制 轮速"),
    ("application/gimbal", "云台 俯仰 偏航 控制"),
    ("application/shoot", "发射 射击 摩擦轮 拨弹"),
    ("application/cmd", "指令 遥控 模式切换"),
    ("application/robot_def", "机器人 跨模块 控制指令 上传反馈 底盘 云台 发射"),
    ("application/board", "开发板 硬件 装配 外设 标定"),
    ("application/runtime/ins_bridge", "姿态 桥接 惯性导航"),
    ("application/runtime/tuning", "在线调参 整定 参数策略"),
    ("application/runtime/robot_task", "实时系统 任务 调度 周期"),
    ("motor", "电机 电流 转速 角度 控制器"),
    ("power_meter", "功率计 电压 电流 实测功率"),
    ("chassis_power", "底盘功率 功控 预算 限幅 缓冲能量"),
    ("power_model", "功率模型 损耗 预测 功控"),
    ("chassis_kinematics", "底盘 运动学 轮速 速度 解算"),
    ("chassis_velocity", "底盘 速度 轮速 闭环 控制"),
    ("chassis_spin", "底盘 小陀螺 旋转 自旋 功率分配"),
    ("chassis_motion", "底盘 平移 旋转 运动"),
    ("vehicle_safety", "整车 安全 启停 遥控失联"),
    ("super_cap", "超级电容 电容 电压 储能 功率"),
    ("referee", "裁判系统 比赛状态 血量 功率 热量 图形交互"),
    ("heat_control", "热量 射击 冷却 发射限制"),
    ("remote", "遥控器 遥控通道 拨杆 键盘 鼠标"),
    ("vtx", "图传 键鼠 键盘 鼠标"),
    ("can_comm", "双板通信 CAN 收发"),
    ("imu", "惯性测量 姿态 陀螺仪 加速度"),
    ("bmi088", "惯性测量 姿态 陀螺仪 加速度"),
    ("mpu6500", "惯性测量 姿态 陀螺仪 加速度 温度"),
    ("master_process", "视觉 自瞄 上位机 通信"),
    ("vision", "视觉 自瞄 目标"),
    ("usb", "USB 虚拟串口 遥测 调参 数据传输"),
    ("tuning", "在线调参 PID 自动整定 参数保存"),
    ("telemetry", "遥测 在线调参 参数保存 数据采样"),
    ("pid_autotune", "PID 自动整定 闭环反馈"),
    ("controller", "PID 控制器 滤波 误差 积分"),
    ("kalman", "卡尔曼滤波 状态估计 矩阵"),
    ("quaternion", "四元数 姿态 扩展卡尔曼"),
    ("algorithm", "算法 运动学 滤波 控制 计算"),
    ("daemon", "守护 离线检测 看门狗"),
    ("message", "消息中心 发布订阅 模块通信"),
    ("usart", "串口 UART DMA 接收 发送"),
    ("uart", "串口 UART 接收 发送 日志"),
    ("can", "CAN 总线 接收 发送"),
    ("spi", "SPI 外设 总线 通信"),
    ("iic", "I2C 外设 总线 通信"),
    ("i2c", "I2C 外设 总线 通信"),
    ("pwm", "PWM 定时器 占空比 输出"),
    ("gpio", "GPIO 引脚 中断 电平"),
    ("dwt", "计时 时钟 时间间隔"),
    ("key", "按键 输入 状态"),
    ("buzzer", "蜂鸣器 告警 提示音"),
    ("oled", "显示屏 OLED 图像"),
    ("tfmini", "测距 激光 距离"),
    ("bluetooth", "蓝牙 无线通信"),
    ("hc05", "蓝牙 无线通信"),
    ("cmsis", "ARM 内核 寄存器 DSP 数学 矩阵"),
    ("stm32", "STM32 外设 寄存器 HAL 配置 句柄"),
    ("freertos", "实时操作系统 任务 队列 信号量 定时器 链表"),
    ("segger", "RTT 调试 日志 通道"),
]


@dataclass
class Member:
    name: str
    declaration: str
    line: int
    comment: str = ""
    condition: str = ""
    type_name: str = ""
    nested: str | None = None


@dataclass
class Record:
    name: str
    kind: str
    path: str
    line: int
    anchor: str
    aliases: list[str] = field(default_factory=list)
    comment: str = ""
    condition: str = ""
    members: list[Member] = field(default_factory=list)
    parent: str | None = None
    start: int = 0
    opening: int = 0
    closing: int = 0
    end: int = 0
    tag: str = ""
    instance: str = ""
    alias_locations: dict[str, list[tuple[str, int]]] = field(default_factory=dict)


def clean_comment(text):
    text = re.sub(r"^/\*+!?<?|\*/$|^//[/!<]*", "", text.strip())
    lines = [re.sub(r"^\s*\*\s?", "", line).strip() for line in text.splitlines()]
    lines = [re.sub(r"^[@\\]brief\s*", "", line) for line in lines
             if not re.match(r"^[@\\](file|author|date|version|copyright|ingroup|defgroup|addtogroup)\b", line)]
    return " ".join(line for line in lines if line.strip("*/ -@{}"))


class Source:
    def __init__(self, text, path):
        self.text, self.path = text, path
        self.lines = [0] + [m.end() for m in re.finditer("\n", text)]
        self.comments = []
        masked = list(text)
        for match in LEXEMES.finditer(text):
            if match.group().startswith(("//", "/*")):
                self.comments.append((match.start(), match.end(), clean_comment(match.group())))
            masked[match.start():match.end()] = ["\n" if c == "\n" else " " for c in match.group()]
        self.conditions = {}
        self.macro_ends = {}
        self.macros = {}
        stack = []
        offset = 0
        physical = text.splitlines(keepends=True)
        i = 0
        while i < len(physical):
            line = physical[i]
            number = i + 1
            self.conditions[number] = " / ".join(stack)
            # Recognize directives in the comment/string-masked text.
            visible = "".join(masked[offset:offset + len(line)])
            if visible.lstrip().startswith("#"):
                begin, logical = offset, visible
                while logical.rstrip().endswith("\\") and i + 1 < len(physical):
                    offset += len(physical[i])
                    i += 1
                    self.conditions[i + 1] = " / ".join(stack)
                    logical += "".join(masked[offset:offset + len(physical[i])])
                end = offset + len(physical[i])
                directive = re.sub(r"\\\s*\n", " ", logical).strip()
                command = re.match(r"#\s*(\w+)\s*(.*)", directive, re.S)
                if command:
                    op, value = command.groups()
                    if op in {"if", "ifdef", "ifndef"}:
                        stack.append(f"#{op} {value.strip()}")
                    elif op in {"elif", "else"} and stack:
                        stack[-1] = stack[-1].split(" → ")[0] + f" → #{op} {value.strip()}".rstrip()
                    elif op == "endif" and stack:
                        stack.pop()
                    # Keep explicit record bodies in macro templates, never expand calls.
                    macro = re.match(r"[ \t]*#\s*define\s+(\w+)(?:\([^\n]*?\))?\s*", logical)
                    definition = re.match(r"[ \t]*#\s*define\s+(\w+)(?:\(([^\n]*?)\))?[ \t]*", logical)
                    if definition:
                        name, parameters = definition.groups()
                        body = re.sub(r"\\\s*\n", " ", logical[definition.end():]).strip()
                        self.macros.setdefault(name, []).append((parameters, body, " / ".join(stack), number))
                    literal = macro and re.search(r"\b(struct|union|__PACKED_STRUCT|__PACKED_UNION)\b[^;{}]*\{", logical[macro.end():])
                    cutoff = begin + macro.end() if literal else end
                    for pos in range(begin, cutoff):
                        if masked[pos] != "\n":
                            masked[pos] = " "
                    if literal:
                        self.macro_ends[begin + macro.end()] = end
                        for n in range(number, i + 2):
                            self.conditions[n] = " / ".join(stack + [f"宏模板 #define {macro.group(1)}"])
                        for pos in range(cutoff, end):
                            if masked[pos] == "\\":
                                masked[pos] = " "
            offset += len(physical[i])
            i += 1
        self.masked = "".join(masked)
        self.tokens = list(TOKENS.finditer(self.masked))

    def line(self, offset):
        return bisect_right(self.lines, offset)

    def condition(self, offset):
        return self.conditions.get(self.line(offset), "")

    def comment(self, start, end):
        inline = [c[2] for c in self.comments if start <= c[0] < end]
        line_end = self.text.find("\n", end)
        if line_end < 0:
            line_end = len(self.text)
        trailing = [c[2] for c in self.comments if end <= c[0] < line_end
                    and not self.text[end:c[0]].strip()]
        if inline or trailing:
            return " ".join(inline + trailing)
        previous = [c for c in self.comments if c[1] <= start]
        if previous:
            begin, finish, comment = previous[-1]
            line_start = self.text.rfind("\n", 0, begin) + 1
            if not self.text[line_start:begin].strip() and not self.masked[finish:start].strip():
                return comment
        return ""

    def fail(self, offset, message):
        raise ValueError(f"{self.path}:{self.line(offset)}: {message}")


def closing_token(source, start):
    pairs = {"{": "}", "(": ")", "[": "]"}
    opening = source.tokens[start].group()
    stack = [pairs[opening]]
    for i in range(start + 1, len(source.tokens)):
        value = source.tokens[i].group()
        if value in pairs:
            stack.append(pairs[value])
        elif value in {"}", ")", "]"}:
            if value != stack.pop():
                source.fail(source.tokens[i].start(), "括号不匹配")
            if not stack:
                return i
    source.fail(source.tokens[start].start(), "定义的括号未闭合")


def without_attributes(text):
    return re.sub(r"\b(?:__attribute__|__declspec)\s*\((?:[^()]|\([^()]*\))*\)", " ", text)


def declarator_name(text):
    text = without_attributes(text)
    callback = re.search(r"\(\s*\*\s*(?:const\s+)?([A-Za-z_]\w*)\s*\)", text)
    if callback:
        return callback.group(1)
    prefix = re.split(r"[\[(:=]", text, maxsplit=1)[0]
    names = re.findall(r"\b[A-Za-z_]\w*\b", prefix)
    if ":" in text and all(n in PRIMITIVES | QUALIFIERS or re.fullmatch(r"u?int\d+_t", n) for n in names):
        return "<未命名位域>"
    return names[-1] if names else ""


def split_declarations(source, begin, end, separator):
    start, i = begin, begin
    while i < end:
        token = source.tokens[i]
        if token.group() in {"{", "(", "["}:
            i = closing_token(source, i)
        elif token.group() == separator:
            if start < i:
                yield start, i
            start = i + 1
        i += 1
    if start < end:
        yield start, end


def expand_field_macro(source, name, arguments, active=()):
    """Expand local declaration macros/X-macro field lists, retaining variants.

    This deliberately accepts only declarations or a sequence of macro calls.
    Token pasting, stringification and unknown templates fail visibly.
    """
    if name in active or name not in source.macros:
        raise ValueError(f"{source.path}: 字段宏无法展开: {' -> '.join(active + (name,))}")
    for parameters, body, condition, line in source.macros[name]:
        if parameters is not None:
            params = [p.strip() for p in parameters.split(",") if p.strip()]
            if arguments is None or len(params) != len(arguments):
                raise ValueError(f"{source.path}:{line}: 字段宏参数不匹配: {name}")
            replacements = dict(zip(params, arguments))
            body = re.sub(r"\b[A-Za-z_]\w*\b", lambda m: replacements.get(m.group(), m.group()), body)
        trace = f"{name}（定义第 {line} 行）"
        if not body:
            continue
        if any(c in body for c in "#={}"):
            raise ValueError(f"{source.path}:{line}: 不支持的字段宏模板: {name}")
        if ";" in body:
            yield body, condition, trace
            continue
        calls = list(re.finditer(r"\b(\w+)\s*\(([^()]*)\)", body))
        if not calls or re.sub(r"\b\w+\s*\([^()]*\)", "", body).strip():
            raise ValueError(f"{source.path}:{line}: 不支持的字段宏模板: {name}")
        for call in calls:
            args = [a.strip() for a in call.group(2).split(",") if a.strip()]
            for text, child_condition, child_trace in expand_field_macro(source, call.group(1), args, active + (name,)):
                yield text, " / ".join(dict.fromkeys(c for c in [condition, child_condition] if c)), trace + " → " + child_trace


def field_macro_members(source, opening, closing):
    """Return expanded members and token ranges removed from normal declarations."""
    tokens, members, removed = source.tokens, [], set()
    i = opening + 1
    while i < closing:
        token = tokens[i]
        name = token.group()
        line_start = source.masked.rfind("\n", 0, token.start()) + 1
        own_line = not source.masked[line_start:token.start()].strip()
        boundary = i == opening + 1 or tokens[i - 1].group() == ";" or i - 1 in removed
        end, arguments = i + 1, None
        if (own_line or boundary) and IDENT.fullmatch(name):
            if end < closing and tokens[end].group() == "(":
                end = closing_token(source, end) + 1
                argument_text = source.masked[tokens[i + 1].end():tokens[end - 1].start()]
                arguments = [a.strip() for a in argument_text.split(",") if a.strip()]
            line_end = source.masked.find("\n", tokens[end - 1].end())
            if line_end < 0:
                line_end = len(source.masked)
            standalone = not source.masked[tokens[end - 1].end():line_end].strip()
            variants = source.macros.get(name, [])
            declaration_macro = any(";" in body or parameters is not None for parameters, body, _, _ in variants)
            empty_macro = variants and all(not body for _, body, _, _ in variants)
            unknown_call = arguments is not None and not variants and name not in PRIMITIVES | QUALIFIERS | {"__attribute__", "__declspec"} and not any("*" in a for a in arguments)
            if ((boundary or standalone) and declaration_macro) or (standalone and empty_macro) or (boundary and unknown_call):
                for text, condition, trace in expand_field_macro(source, name, arguments):
                    synthetic = parse_source("struct MacroFields { " + text + " };", source.path)
                    if len(synthetic) != 1:
                        source.fail(token.start(), "字段宏含无法定位的嵌套类型: " + name)
                    for member in synthetic[0].members:
                        member.line = source.line(token.start())
                        member.condition = " / ".join(dict.fromkeys(c for c in [source.condition(token.start()), condition] if c))
                        member.comment = "；".join(filter(None, [member.comment, source.comment(token.start(), tokens[end - 1].end()), "字段宏 " + trace]))
                        members.append(member)
                removed.update(range(i, end))
                i = end
                continue
        if name in {"{", "(", "["}:
            i = closing_token(source, i)
        i += 1
    return members, removed


def parse_source(text, path):
    source = Source(text, path)
    tokens, records, spans = source.tokens, [], {}
    for i, token in enumerate(tokens):
        if token.group() not in KINDS:
            continue
        p, tag = i + 1, ""
        while p < len(tokens):
            value = tokens[p].group()
            if value in {"__attribute__", "__declspec"} and p + 1 < len(tokens) and tokens[p + 1].group() == "(":
                p = closing_token(source, p + 1) + 1
            elif value in QUALIFIERS:
                p += 1
            elif IDENT.fullmatch(value) and not tag:
                tag, p = value, p + 1
            else:
                break
        # HAL switches between tagged and anonymous headers before one body.
        # Keep both source variants, including the tag needed by callbacks.
        q = p
        while q + 1 < len(tokens) and tokens[q].group() == "typedef" and tokens[q + 1].group() in KINDS:
            q += 2
            if q < len(tokens) and IDENT.fullmatch(tokens[q].group()):
                q += 1
        if q != p and q < len(tokens) and tokens[q].group() == "{":
            p = q
        if p >= len(tokens) or tokens[p].group() != "{":
            continue
        close = closing_token(source, p)
        macro_limit = next((end for begin, end in source.macro_ends.items() if begin <= token.start() < end), None)
        tail_end = close + 1
        while tail_end < len(tokens) and tokens[tail_end].group() not in {";", "="} and (macro_limit is None or tokens[tail_end].start() < macro_limit):
            if tokens[tail_end].group() in {"[", "("}:
                tail_end = closing_token(source, tail_end)
            elif tokens[tail_end].group() in {"{", "}"}:
                source.fail(token.start(), "无法确定结构体声明结尾")
            tail_end += 1
        if tail_end == len(tokens) and macro_limit is None:
            source.fail(token.start(), "结构体定义缺少声明结尾")
        boundary = max(source.masked.rfind(c, 0, token.start()) for c in ";{}") + 1
        prefix = source.masked[boundary:token.start()]
        is_typedef = bool(re.search(r"\btypedef\b", prefix))
        tail_position = min(tokens[tail_end].start(), macro_limit) if tail_end < len(tokens) and macro_limit else (macro_limit if macro_limit else tokens[tail_end].start())
        tail = source.masked[tokens[close].end():tail_position]
        names = [declarator_name(part) for part in re.split(r",(?![^()]*\))", without_attributes(tail))]
        names = [n for n in names if n and n not in QUALIFIERS]
        aliases = names if is_typedef else []
        line = source.line(token.start())
        column = token.start() - source.lines[line - 1] + 1
        anchor = "r-" + re.sub(r"[^a-zA-Z0-9_-]", "-", path) + f"-{line}-{column}"
        name = aliases[0] if aliases else tag or (names[0] if names else f"匿名{KINDS[token.group()]}@L{line}C{column}")
        record = Record(name, KINDS[token.group()], path, line, anchor, aliases,
                        source.comment(boundary, token.start()), source.condition(token.start()),
                        start=token.start(), opening=tokens[p].start(), closing=tokens[close].start(),
                        end=tail_position, tag=tag, instance=names[0] if names else "")
        records.append(record)
        spans[record.anchor] = (p, close)

    for record in records:
        parents = [r for r in records if r.opening < record.start < r.closing]
        if parents:
            parent = max(parents, key=lambda r: r.opening)
            record.parent = parent.anchor
            if not record.aliases and not record.tag:
                record.name = parent.name + "." + record.name
    for record in records:
        # A tag may have a separate typedef before or after its definition.
        if record.tag:
            pattern = r"\btypedef\s+" + record.kind + r"\s+" + re.escape(record.tag) + r"\s+\*?\s*(\w+)\s*;"
            for alias in re.findall(pattern, source.masked):
                if alias not in record.aliases:
                    record.aliases.append(alias)
        opening, closing = spans[record.anchor]
        expanded, removed = field_macro_members(source, opening, closing)
        record.members.extend(expanded)
        for begin, end in split_declarations(source, opening + 1, closing, ";"):
            while begin < end and begin in removed:
                begin += 1
            if begin == end:
                continue
            start_pos, end_pos = tokens[begin].start(), tokens[end].end() if end < closing else tokens[end - 1].end()
            declaration = source.masked[start_pos:tokens[end].start()]
            children = [r for r in records if r.parent == record.anchor and start_pos <= r.start < end_pos]
            nested = children[0] if children else None
            field_begin = begin
            if nested:
                _, nested_close = spans[nested.anchor]
                field_begin = nested_close + 1
            parts = list(split_declarations(source, field_begin, end, ","))
            if not parts and nested:
                parts = [(end, end)]
            comment = source.comment(start_pos, end_pos)
            base = declaration.split("{", 1)[0]
            first_part = next(iter(split_declarations(source, field_begin, end, ",")), None)
            first_text = source.masked[tokens[first_part[0]].start():tokens[first_part[1]].start()] if first_part else ""
            first_name = declarator_name(first_text)
            if first_name and IDENT.fullmatch(first_name):
                base = re.split(r"\b" + re.escape(first_name) + r"\b", base, maxsplit=1)[0]
            base_names = re.findall(r"\b[A-Za-z_]\w*\b", without_attributes(base))
            type_name = next((n for n in base_names if n not in QUALIFIERS | PRIMITIVES | {"struct", "union", "enum"}), "")
            if re.search(r"\(\s*\*", first_text) or "enum" in base_names:
                type_name = ""
            display = declaration
            if nested:
                a, b = nested.opening - start_pos, nested.closing - start_pos
                display = display[:a] + "{ … }" + display[b + 1:]
            display = " ".join(display.split()) + ";"
            for a, b in parts:
                fragment = source.masked[tokens[a].start():tokens[b].start()] if a < b else ""
                name = declarator_name(fragment) if fragment.strip() else "<匿名成员>"
                if not name:
                    source.fail(start_pos, "无法解析成员: " + display)
                record.members.append(Member(name, display, source.line(tokens[a].start()) if a < b else source.line(start_pos),
                                             comment, source.condition(start_pos), type_name,
                                             nested.anchor if nested else None))
        record.members.sort(key=lambda m: m.line)
    return records


def source_paths(root):
    for directory, dirs, files in os.walk(root):
        dirs[:] = sorted(d for d in dirs if d not in EXCLUDED and not d.startswith("cmake-build-"))
        for name in sorted(files):
            if Path(name).suffix.lower() in {".c", ".h"}:
                yield Path(directory) / name


def read_source(path):
    data = path.read_bytes()
    try:
        return data.decode("utf-8-sig")
    except UnicodeDecodeError:
        try:
            return data.decode("gb18030")
        except UnicodeDecodeError:
            try:
                return data.decode("cp1252")
            except UnicodeDecodeError as error:
                raise ValueError(f"{path}: 不能按 UTF-8、GB18030 或 Windows-1252 解码") from error


def group(path):
    first = path.split("/", 1)[0]
    return {"Drivers": "芯片驱动与 CMSIS", "Middlewares": "第三方中间件",
            "portable_modules": "便携副本", "tests": "测试与测试桩"}.get(first, "项目源码")


def purpose(path):
    lowered = path.lower()
    words = []
    for key, hints in PURPOSES:
        if key in lowered:
            words.extend(hints.split())
    if not words:
        words = ["项目数据" if group(path) == "项目源码" else group(path)]
    return " ".join(dict.fromkeys(words))


def cell(text):
    return str(text).replace("&", "&amp;").replace("|", "&#124;").replace("<", "&lt;").replace(">", "&gt;").replace("`", "&#96;").replace("\n", " ")


def code(text):
    text = str(text).replace("|", r"\|").replace("\n", " ")
    fence = "`" * (max((len(m.group()) for m in re.finditer(r"`+", text)), default=0) + 1)
    return fence + " " + text + " " + fence if "`" in text else fence + text + fence


def link(record):
    return f"[{cell(record.name)}](#{record.anchor})"


def source_link(path, line):
    if line is None:
        return f"[{cell(path)}](../{quote(path, safe='/')})"
    return f"[{cell(path)}:{line}](../{quote(path, safe='/')}#L{line})"


def preferred_targets(path, candidates):
    same_file = [r for r in candidates if r.path == path]
    same_dir = [r for r in candidates if Path(r.path).parent == Path(path).parent]
    same_group = [r for r in candidates if group(r.path) == group(path)]
    return same_file or same_dir or same_group or candidates


def type_targets(owner, member, records, by_name):
    if member.nested:
        return [records[member.nested]]
    return preferred_targets(owner.path, by_name.get(member.type_name, []))


def load_repository(root):
    """Return the source/type inventory shared by the type and instance indexes."""
    root = Path(root)
    paths = sorted(source_paths(root), key=lambda p: p.relative_to(root).as_posix())
    all_records, counts, aliases = [], [], []
    for path in paths:
        relative = path.relative_to(root).as_posix()
        source_text = read_source(path)
        found = parse_source(source_text, relative)
        source_view = Source(source_text, relative)
        for match in re.finditer(r"\btypedef\s+(?:(?:struct|union)\s+)?([A-Za-z_]\w*)\s+\*?\s*([A-Za-z_]\w*)\s*;", source_view.masked):
            aliases.append((*match.groups(), relative, source_view.line(match.start())))
        all_records.extend(found)
        counts.append((relative, len(found), sum(len(r.members) for r in found)))
    by_id = {r.anchor: r for r in all_records}
    if len(by_id) != len(all_records):
        raise ValueError("重复的源码位置标识")
    by_name = {}
    for record in all_records:
        for name in set(record.aliases + [record.name, record.tag]) - {""}:
            by_name.setdefault(name, []).append(record)
    # Resolve simple aliases even when their forward typedef lives in a header
    # and the opaque record is privately defined in a .c file.
    pending = list(aliases)
    while pending:
        remaining, changed = [], False
        for original, alias, path, line in pending:
            if original in by_name:
                targets = by_name.setdefault(alias, [])
                for record in preferred_targets(path, by_name[original][:]):
                    if record not in targets:
                        targets.append(record)
                        changed = True
                    if alias not in record.aliases:
                        record.aliases.append(alias)
                    locations = record.alias_locations.setdefault(alias, [])
                    if (path, line) not in locations:
                        locations.append((path, line))
            else:
                remaining.append((original, alias, path, line))
        if not changed:
            break
        pending = remaining
    return paths, all_records, counts, by_id, by_name


def generate(root):
    root = Path(root)
    paths, all_records, counts, by_id, by_name = load_repository(root)
    groups = ["项目源码", "便携副本", "芯片驱动与 CMSIS", "第三方中间件", "测试与测试桩"]
    all_records.sort(key=lambda r: (groups.index(group(r.path)), r.path, r.line, r.start))
    nstruct = sum(r.kind == "struct" for r in all_records)
    nmember = sum(len(r.members) for r in all_records)
    out = ["# 结构体与成员索引", "",
           f"扫描 **{len(paths)}** 个 C/H 文件，收录 **{nstruct}** 个结构体、**{len(all_records) - nstruct}** 个联合体、**{nmember}** 条直接成员声明（含匿名成员与未命名位域）。", "",
           "要查常用实例指针、访问表达式与宏，请先看[常用指针与宏速查](../.Doc/指针查找索引.md)；本文件用于查字段类型、注释和定义。", "",
           "在本文件用 **Ctrl+F** 搜索中文用途、类型名或任意成员名。先从[中文用途导航](#中文用途导航)找模块，或从[成员名反查](#成员名反查)找字段，再跳转到类型详情和源码。", "",
           "更新：在仓库目录执行 `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1`，或双击 `scripts/update-structure-index.cmd`。解释器不能自动发现时传入 `-Python <python.exe 完整路径>`。", "",
           "本文件由 `scripts/generate-structure-index.py` 自动生成，请勿直接编辑。中文用途关键词在脚本 `PURPOSES` 中维护；字段含义优先使用源码注释，没有注释时明确标注，不推测语义。", "",
           "覆盖仓库全部展开的 `.c`/`.h` 源文件，包括私有/局部定义、匿名嵌套、HAL/CMSIS、第三方库、便携副本和测试桩。排除 Git 数据、build/output 构建产物、node_modules、虚拟环境和缓存；不读取压缩包。", "",
           "这是源码级清单：保留所有条件分支和宏中显式写出的类型模板；同文件的声明宏及 X-macro 字段列表会展开为可反查成员，并标明宏定义行与调用位置。未知字段宏、递归、字符串化或拼接模板会报错，不静默冒充字段。其他任意宏调用不作完整预处理；不表示这些定义都参与当前固件链接。仅前向声明不重复计数。联合体一并收录以保留完整嵌套路径。", "",
           "成员表保留原声明（逗号声明的多个字段共享原声明与注释）；类型关系按同文件、同目录、同来源组的优先级解析，同级有多个候选时全部列出。匿名字段以父类型路径命名；条件编译条件沿用源码，不计算是否生效。", "",
           "## 中文用途导航", "", "| 中文用途 / 搜索词 | 模块与来源 | 类型数 |", "| --- | --- | --- |"]
    modules = {}
    for record in all_records:
        module = str(Path(record.path).parent).replace("\\", "/")
        modules.setdefault(module, []).append(record)
    for module, entries in sorted(modules.items(), key=lambda p: (groups.index(group(p[1][0].path)), p[0])):
        out.append(f"| {cell(purpose(entries[0].path))} | [{cell(module)}](#{entries[0].anchor})（{group(entries[0].path)}） | {len(entries)} |")
    out += ["", "## 覆盖统计", "", "| 来源 | C/H 文件 | 结构体 | 联合体 | 直接成员 |", "| --- | --- | --- | --- | --- |"]
    for name in groups:
        entries = [r for r in all_records if group(r.path) == name]
        out.append(f"| {name} | {sum(group(p) == name for p, _, _ in counts)} | {sum(r.kind == 'struct' for r in entries)} | {sum(r.kind == 'union' for r in entries)} | {sum(len(r.members) for r in entries)} |")
    out += ["", "## 类型总表", "", "| 类型 | 中文用途关键词 | 源码说明 | 来源与位置 |", "| --- | --- | --- | --- |"]
    for record in all_records:
        out.append(f"| {link(record)}（{record.kind}） | {cell(purpose(record.path))} | {cell(record.comment) or '源码未注释'} | {group(record.path)} · {source_link(record.path, record.line)} |")
    out += ["", "## 成员名反查", "", "同名成员按来源与所属类型分别列出。嵌套字段可从所属类型沿父类型/嵌套关系向上或向下追踪。", "",
            "| 成员名 | 所属类型 | 成员说明 | 中文用途关键词 | 源码位置 |", "| --- | --- | --- | --- | --- |"]
    members = [(m, r) for r in all_records for m in r.members]
    for member, record in sorted(members, key=lambda p: (p[0].name.casefold(), groups.index(group(p[1].path)), p[1].path, p[0].line)):
        out.append(f"| {code(member.name)} | {link(record)} | {cell(member.comment) or '源码未注释'} | {cell(purpose(record.path))} | {source_link(record.path, member.line)} |")
    out += ["", "## 类型详情", ""]
    for record in all_records:
        out += [f'<a id="{record.anchor}"></a>', "", f"### {cell(record.name)}", "",
                f"- 类型：{record.kind}；来源：{group(record.path)}；定义：{source_link(record.path, record.line)}。",
                f"- 中文用途关键词：{cell(purpose(record.path))}。",
                f"- 源码说明：{cell(record.comment) or '源码未注释'}。"]
        if record.aliases or record.tag:
            out.append("- 名称与别名：" + "、".join(code(n) for n in dict.fromkeys(record.aliases + ([record.kind + " " + record.tag] if record.tag else []))) + "。")
        if record.alias_locations:
            out.append("- 别名声明：" + "；".join(code(name) + "：" + "、".join(source_link(path, line) for path, line in locations)
                       for name, locations in record.alias_locations.items()) + "。")
        if record.condition:
            out.append(f"- 源码条件：{code(record.condition)}。")
        if record.parent:
            out.append(f"- 父类型：{link(by_id[record.parent])}。")
        out += ["", "| 成员 | 声明 | 成员说明 | 源码位置 |", "| --- | --- | --- | --- |"]
        for member in record.members:
            note = cell(member.comment) or "源码未注释"
            if member.condition != record.condition:
                note += "；条件：" + code(member.condition)
            out.append(f"| {code(member.name)} | {code(member.declaration)} | {note} | {source_link(record.path, member.line)} |")
        relations = [(m, type_targets(record, m, by_id, by_name)) for m in record.members]
        relations = [(m, targets) for m, targets in relations if targets]
        out += ["", "嵌套关系：", ""]
        if not relations:
            out.append("无可链接的仓库内结构体/联合体成员。")
        for member, targets in relations:
            refs = "、".join(f"{link(t)}（{cell(t.path)}:{t.line}）" for t in targets)
            out.append(f"- {code(record.name + '.' + member.name)} → {refs}。声明中的 `*` 表示指针关系。")
        out.append("")
    out += ["## 文件扫描明细", "", "每个扫描文件均列出，包括没有类型定义的文件，便于核对覆盖范围。", "",
            "| 文件 | 来源 | 结构体与联合体 | 直接成员 |", "| --- | --- | --- | --- |"]
    for path, nrecords, nmembers in counts:
        out.append(f"| {source_link(path, 1 if (root / path).stat().st_size else None)} | {group(path)} | {nrecords} | {nmembers} |")
    return "\n".join(out) + "\n"


def write_document(output, markdown):
    """Publish a complete UTF-8 document without truncating its previous version."""
    output.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=output.name + ".", suffix=".tmp", dir=output.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(markdown.encode("utf-8"))
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, output)
    finally:
        Path(temporary).unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true", help="exit nonzero if the index is missing or stale; do not write")
    args = parser.parse_args()
    output = args.root / "docs/structure-index.md"
    try:
        markdown = generate(args.root)
        if args.check:
            if not output.exists() or output.read_text(encoding="utf-8") != markdown:
                print("Structure index is stale; run scripts/update-structure-index.ps1", file=sys.stderr)
                return 1
            print("Structure index is up to date.")
        else:
            write_document(output, markdown)
            print(f"Updated {output}")
        return 0
    except (ValueError, OSError) as error:
        print(f"Structure index failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    raise SystemExit(main())
