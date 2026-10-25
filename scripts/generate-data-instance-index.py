"""Index real C data holders, pointer bindings and member access expressions."""
from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from functools import lru_cache
import importlib.util
from pathlib import Path
import re
import sys


_spec = importlib.util.spec_from_file_location("structure_inventory", Path(__file__).with_name("generate-structure-index.py"))
types = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = types
_spec.loader.exec_module(types)

STORAGE = {"static", "extern", "register", "auto", "_Thread_local"}
GROUPS = ["项目源码", "便携副本", "芯片驱动与 CMSIS", "第三方中间件", "测试与测试桩"]


@dataclass
class Assignment:
    line: int
    expression: str
    target: str
    function: str = ""
    offset: int = 0


@dataclass
class Variable:
    name: str
    type_name: str
    targets: list
    path: str
    line: int
    pointers: int
    arrays: list[str]
    storage: str
    function: str
    scope_start: int
    scope_end: int
    start: int
    end: int
    declaration: str
    initializer: str = ""
    comment: str = ""
    condition: str = ""
    pointer_to_array: bool = False
    assignments: list[Assignment] = field(default_factory=list)

    @property
    def anchor(self):
        return "v-" + re.sub(r"[^A-Za-z0-9_-]", "-", self.path) + f"-{self.line}-{self.start}-{self.name}"


@dataclass
class Inventory:
    variables: list[Variable]
    records: list
    by_id: dict
    by_name: dict
    sources: dict
    aliases: dict


def compact(text, limit=240):
    text = " ".join(text.split())
    return text if len(text) <= limit else text[:limit] + " …（完整内容见源码）"


def pair_delimiters(source):
    """Pair delimiters across alternative prefixes such as #if A f( #else g(."""
    masked = list(source.text)
    for match in types.LEXEMES.finditer(source.text):
        masked[match.start():match.end()] = ["\n" if c == "\n" else " " for c in match.group()]
    directives = [(m.start(), "directive", m.group(1)) for m in re.finditer(
        r"(?m)^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b", "".join(masked))]
    events = sorted(directives + [(t.start(), "token", i) for i, t in enumerate(source.tokens)])
    stack, branches, pairs = [], [], {}
    for offset, kind, value in events:
        if kind == "directive":
            if value in {"if", "ifdef", "ifndef"}:
                branches.append((stack[:], []))
            elif value in {"elif", "else"} and branches:
                baseline, alternatives = branches[-1]
                alternatives.append(stack[:])
                stack = baseline[:]
            elif value == "endif" and branches:
                _, alternatives = branches.pop()
                alternatives.append(stack[:])
                shapes = {tuple(item[0] for item in branch) for branch in alternatives}
                if len(shapes) > 1:
                    source.fail(offset, "条件分支的括号层级不同，无法可靠确定实例作用域")
                stack = [(alternatives[0][j][0], frozenset().union(*(branch[j][1] for branch in alternatives)))
                         for j in range(len(alternatives[0]))]
            continue
        text = source.tokens[value].group()
        if text in {"{", "(", "["}:
            stack.append((text, frozenset({value})))
        elif text in {"}", ")", "]"}:
            if not stack or stack[-1][0] != {"}": "{", ")": "(", "]": "["}[text]:
                source.fail(offset, "无法确定实例作用域：括号不匹配")
            _, openings = stack.pop()
            for opening in openings:
                pairs[opening] = value
    if stack:
        source.fail(source.tokens[next(iter(stack[-1][1]))].start(), "无法确定实例作用域：括号未闭合")
    return pairs


def closing(source, index):
    return source.delimiters[index]


def split_parts(source, begin, end, separator):
    start, i = begin, begin
    while i < end:
        if source.tokens[i].group() in {"{", "(", "["}:
            i = closing(source, i)
        elif source.tokens[i].group() == separator:
            if start < i:
                yield start, i
            start = i + 1
        i += 1
    if start < end:
        yield start, end


def candidates_for(path, candidates):
    same_file = [r for r in candidates if r.path == path]
    same_dir = [r for r in candidates if Path(r.path).parent == Path(path).parent]
    if same_file or same_dir:
        return same_file or same_dir
    origin = types.group(path)
    if origin == "测试与测试桩":
        suite = "/".join(path.split("/")[:2]) + "/"
        local_suite = [r for r in candidates if r.path.startswith(suite)]
        if local_suite:
            return local_suite
    else:
        own_group = [r for r in candidates if types.group(r.path) == origin]
        if own_group:
            return own_group
    firmware = [r for r in candidates if types.group(r.path) not in {"测试与测试桩", "便携副本"}]
    return firmware or (candidates if origin in {"测试与测试桩", "便携副本"} else [])


def primitive_shadow_names(source, begin, end):
    p, tokens = begin, source.tokens
    while p < end and tokens[p].group() in STORAGE | types.QUALIFIERS:
        p += 1
    if p >= end or not (tokens[p].group() in types.PRIMITIVES or tokens[p].group() in source.declaration_types or re.fullmatch(r"u?int\d+_t|size_t", tokens[p].group())):
        return []
    p += 1
    while p < end and tokens[p].group() in types.PRIMITIVES | types.QUALIFIERS:
        p += 1
    return [types.declarator_name(source.masked[tokens[a].start():tokens[b].start()])
            for a, b in split_parts(source, p, end, ",")]


def parse_declaration(source, begin, end, scope, function, by_name, records):
    tokens = source.tokens
    if begin >= end or tokens[begin].group() in {"typedef", "return", "sizeof", "case", "goto"}:
        return []
    p, flags = begin, set()
    while p < end and tokens[p].group() in STORAGE | types.QUALIFIERS:
        flags.add(tokens[p].group())
        p += 1
    if p >= end:
        return []
    type_name, targets = tokens[p].group(), []
    if type_name in {"struct", "union", "__PACKED_STRUCT", "__PACKED_UNION"}:
        literal = next((r for r in records if r.start == tokens[p].start()), None)
        if literal:
            targets, type_name = [literal], literal.name
            p = next(i for i in range(p, end) if tokens[i].start() == literal.closing) + 1
        elif p + 1 < end:
            p += 1
            type_name = tokens[p].group()
            targets = candidates_for(source.path, by_name.get(type_name, []))
            p += 1
    else:
        # Anonymous object/display names are useful in the type dictionary but
        # are not C typedef names. Otherwise `mailbox.id = id` becomes a fake
        # declaration of a variable `id` with the mailbox's anonymous type.
        targets = candidates_for(source.path, [r for r in by_name.get(type_name, []) if type_name in r.aliases])
        p += 1
    if not targets:
        return []
    result = []
    for a, b in split_parts(source, p, end, ","):
        equals = b
        i = a
        while i < b:
            if tokens[i].group() in {"{", "(", "["}:
                i = closing(source, i)
            elif tokens[i].group() == "=":
                equals = i
                break
            i += 1
        part = source.masked[tokens[a].start():tokens[equals].start()]
        if not re.match(r"\s*(?:\*|\(|[A-Za-z_])", part):
            continue
        name = types.declarator_name(part)
        if not name or not types.IDENT.fullmatch(name):
            continue
        named = next((i for i in range(a, equals) if tokens[i].group() == name), None)
        if named is None:
            continue
        # A function declaration or a callback variable is not a stored data instance.
        if named + 1 < equals and tokens[named + 1].group() == "(":
            continue
        if re.search(r"\(\s*\*[^)]*\)\s*\(", part):
            continue
        arrays = re.findall(r"\[([^]]*)\]", part)
        pointers = part[:part.find(name)].count("*")
        storage = "extern" if "extern" in flags else ("local static" if function and "static" in flags else "local" if function else "static" if "static" in flags else "global")
        initializer = source.text[tokens[equals].end():tokens[b].start()].strip() if equals < b else ""
        result.append(Variable(name, type_name, targets, source.path, source.line(tokens[named].start()), pointers,
                               arrays, storage, function, scope[0], scope[1], tokens[named].start(), tokens[end].end(),
                               compact(source.text[tokens[begin].start():tokens[end].end()]), initializer,
                               source.comment(tokens[begin].start(), tokens[end].end()), source.condition(tokens[named].start()),
                               bool(re.search(r"\(\s*\*[^)]*\)\s*\[", part))))
    return result


def discover_variables(source, records, by_name):
    source.delimiters = pair_delimiters(source)
    tokens, variables, parameters, functions = source.tokens, [], [], []
    record_openings = {r.opening for r in records}

    def initializer_prefix(begin, end):
        # Equality/assignment inside a control-condition parentheses belongs to
        # `if`/`for`, not to the block's declaration initializer.
        i = begin
        while i < end:
            if tokens[i].group() in {"(", "[", "{"}:
                i = closing(source, i)
            elif tokens[i].group() == "=" and (i == begin or tokens[i - 1].group() not in {"=", "!", "<", ">"}) and (i + 1 == end or tokens[i + 1].group() != "="):
                return True
            i += 1
        return False

    def scan(begin, end, scope, function=""):
        start, i = begin, begin
        while i < end:
            value = tokens[i].group()
            if value in {"(", "["}:
                close = closing(source, i)
                if value == "(" and i > begin and tokens[i - 1].group() == "for":
                    semi = next((j for j in range(i + 1, close) if tokens[j].group() == ";"), None)
                    if semi is not None and close + 1 < end:
                        body = close + 1
                        body_end = closing(source, body) if tokens[body].group() == "{" else next((j for j in range(body, end) if tokens[j].group() == ";"), body)
                        loop_scope = (tokens[i - 1].start(), tokens[body_end].end())
                        variables.extend(parse_declaration(source, i + 1, semi, loop_scope, function, by_name, records))
                        parameters.extend((name, *loop_scope, tokens[i + 1].start()) for name in primitive_shadow_names(source, i + 1, semi))
                i = close
            elif value == "{":
                close = closing(source, i)
                if tokens[i].start() in record_openings or initializer_prefix(start, i):
                    i = close
                else:
                    name, signature = function, None
                    if not function:
                        j = start
                        while j < i:
                            if tokens[j].group() == "(":
                                stop = closing(source, j)
                                previous = tokens[j - 1].group() if j > start else ""
                                if types.IDENT.fullmatch(previous) and previous not in {"__attribute__", "__declspec"}:
                                    name, signature = previous, (j + 1, stop)
                                j = stop
                            j += 1
                    inner_scope = (tokens[i].start(), tokens[close].end())
                    if signature:
                        functions.append((*inner_scope, name))
                        for a, b in split_parts(source, *signature, ","):
                            text = source.masked[tokens[a].start():tokens[b].start()]
                            parameter = types.declarator_name(text)
                            if parameter and parameter not in types.PRIMITIVES:
                                parameters.append((parameter, *inner_scope, tokens[i].start()))
                    scan(i + 1, close, inner_scope, name)
                    i = close
                    start = i + 1
            elif value == ";":
                variables.extend(parse_declaration(source, start, i, scope, function, by_name, records))
                if function:
                    parameters.extend((name, *scope, tokens[start].start()) for name in primitive_shadow_names(source, start, i))
                start = i + 1
            i += 1

    scan(0, len(tokens), (0, len(source.text)))
    # Bind only direct assignments to a visible declaration; a callback parameter
    # with the same name must not become evidence about a persistent global.
    names = {v.name for v in variables}
    if names:
        assignment = re.compile(r"(?<![\w.>*&])(" + "|".join(re.escape(n) for n in sorted(names, key=len, reverse=True)) + r")\s*((?:\[[^]\n]+\]\s*)*)=(?!=)")
        for match in assignment.finditer(source.masked):
            position = match.start(1)
            candidates = [v for v in variables if v.name == match.group(1) and v.scope_start <= position < v.scope_end and v.start <= position]
            if not candidates:
                continue
            variable = min(candidates, key=lambda v: v.scope_end - v.scope_start)
            if any(n == variable.name and declared <= position and a < position < b and b - a < variable.scope_end - variable.scope_start for n, a, b, declared in parameters):
                continue
            if variable.start <= position < variable.end:
                continue
            stop = source.masked.find(";", match.end())
            if stop < 0:
                continue
            expression = compact(source.text[match.end():stop])
            function = next((name for a, b, name in functions if a < position < b), "")
            variable.assignments.append(Assignment(source.line(position), expression, match.group(1) + match.group(2).strip(), function, position))
    return variables


def inventory(root):
    root = Path(root)
    paths, records, _, by_id, by_name = types.load_repository(Path(root))
    variables, sources, aliases, declaration_types = [], {}, {}, set()
    file_records = {}
    for record in records:
        file_records.setdefault(record.path, []).append(record)
        declaration_types.update(record.aliases)

    def collect_aliases(text, base, relative, allowed=None):
        fragment = types.Source(text, relative)
        for a, b in types.split_declarations(fragment, 0, len(fragment.tokens), ","):
            stop = fragment.tokens[b].start() if b < len(fragment.tokens) else len(fragment.masked)
            part = fragment.masked[fragment.tokens[a].start():stop]
            name = types.declarator_name(part)
            if not name or not types.IDENT.fullmatch(name) or (allowed is not None and name not in allowed):
                continue
            declaration_types.add(name)
            if re.search(r"\)\s*\(", part):
                continue  # callback typedefs do not point at a data record
            depth, arrays, pointer_array = member_shape(name, part)
            aliases.setdefault(name, []).append((base, operations(depth, arrays, pointer_array), relative))

    for path in paths:
        relative = path.relative_to(root).as_posix()
        source = types.Source(types.read_source(path), relative)
        sources[relative] = source
        for record in file_records.get(relative, []):
            collect_aliases(source.masked[record.closing + 1:record.end], "@" + record.anchor, relative, record.aliases)
        for match in re.finditer(r"\btypedef\s+(?:(?:struct|union)\s+)?(\w+)\s+([^;{}]+);", source.masked):
            collect_aliases(match.group(2), match.group(1), relative)
    pending = [(name, base, location) for name, entries in aliases.items() for base, _, location in entries if not base.startswith("@")]
    while pending:
        remaining, changed = [], False
        for name, base, location in pending:
            if base not in by_name:
                remaining.append((name, base, location))
                continue
            targets = by_name.setdefault(name, [])
            for record in candidates_for(location, by_name[base][:]):
                if record not in targets:
                    targets.append(record)
                    changed = True
                if name not in record.aliases:
                    record.aliases.append(name)
        if not changed:
            break
        pending = remaining
    for relative, source in sources.items():
        source.declaration_types = declaration_types
        variables.extend(discover_variables(source, file_records.get(relative, []), by_name))
    return Inventory(variables, records, by_id, by_name, sources, aliases)


def operations(depth, arrays, pointer_array=False):
    if pointer_array:
        return ("deref",) + ("index",) * len(arrays) + ("deref",) * (depth - 1)
    return ("index",) * len(arrays) + ("deref",) * depth


def alias_operations(data, name, path, seen=()):
    if name in seen:
        return ()
    entries = data.aliases.get(name, [])
    nearby = [e for e in entries if e[2] == path] or [e for e in entries if types.group(e[2]) == types.group(path)] or entries
    counts = {shape + alias_operations(data, base, location, seen + (name,)) for base, shape, location in nearby}
    if len(counts) > 1:
        raise ValueError(f"{path}: 类型别名 {name} 的声明形状存在多个候选，无法生成确定的数据访问路径")
    return next(iter(counts), ())


def alias_pointers(data, name, path):
    return alias_operations(data, name, path).count("deref")


def apply_operations(expression, shape):
    operator = "->" if shape and shape[-1] == "deref" else "."
    for operation in shape[:-1] if operator == "->" else shape:
        expression = "(*" + expression + ")" if operation == "deref" else expression + "[0]"
    return expression, operator


def access_base(variable, data):
    shape = operations(variable.pointers, variable.arrays, variable.pointer_to_array) + alias_operations(data, variable.type_name, variable.path)
    return apply_operations(variable.name, shape)


@lru_cache(maxsize=None)
def member_shape(name, declaration):
    if name == "<匿名成员>":
        return 0, (), False
    declaration = re.sub(r"\{[^{}]*\}", "", declaration)
    source = types.Source(declaration, "<member>")
    for a, b in types.split_declarations(source, 0, len(source.tokens), ","):
        end = source.tokens[b].start() if b < len(source.tokens) else len(source.masked)
        fragment = source.masked[source.tokens[a].start():end].rstrip("; ")
        if types.declarator_name(fragment) == name:
            prefix = re.split(r"\[", fragment, maxsplit=1)[0]
            names = list(re.finditer(r"\b" + re.escape(name) + r"\b", prefix))
            if names:
                return prefix[:names[-1].start()].count("*"), tuple(re.findall(r"\[([^]]*)\]", fragment)), bool(re.search(r"\(\s*\*[^)]*\)\s*\[", fragment))
    raise ValueError(f"无法确定成员声明形状: {name}: {declaration}")


def member_paths(variable, data):
    expression, operator = access_base(variable, data)

    def descend(record, prefix, op, seen):
        for member in record.members:
            if member.name == "<未命名位域>":
                continue
            anonymous = member.name == "<匿名成员>"
            path = prefix if anonymous else prefix + op + member.name
            yield member, path, record, ""
            targets = [data.by_id[member.nested]] if member.nested else candidates_for(record.path, data.by_name.get(member.type_name, []))
            if not targets:
                continue
            if len(targets) > 1:
                yield member, path, record, "多个候选定义；见类型索引，停止推断更深路径"
                continue
            target = targets[0]
            if target.anchor in seen:
                yield member, path, record, "递归类型，停止沿此路径展开"
                continue
            depth, arrays, pointer_array = member_shape(member.name, member.declaration)
            shape = operations(depth, arrays, pointer_array) + alias_operations(data, member.type_name, record.path)
            child_path, child_operator = apply_operations(path, shape)
            yield from descend(target, child_path, op if anonymous else child_operator, seen + (target.anchor,))

    if len(variable.targets) != 1:
        return [(member, expression + operator + member.name, record, "多个候选定义；本行仅表示此候选的直接成员，见类型索引")
                for record in variable.targets for member in record.members
                if member.name not in {"<匿名成员>", "<未命名位域>"}]
    return list(descend(variable.targets[0], expression, operator, (variable.targets[0].anchor,)))


def variable_link(variable):
    return f"[{types.cell(variable.name)}](#{variable.anchor})"


def type_link(record):
    return f"[{types.cell(record.name)}](structure-index.md#{record.anchor})"


def purpose(variable):
    hints = [types.purpose(variable.path)] + [types.purpose(r.path) for r in variable.targets]
    return " ".join(dict.fromkeys(" ".join(hints).split()))


def assignment_source(variable, assignment, data):
    match = re.fullmatch(r"&?([A-Za-z_]\w*)", assignment.expression)
    if not match:
        return None
    candidates = [v for v in data.variables if v.path == variable.path and v.name == match.group(1)
                  and v.start <= assignment.offset and v.scope_start <= assignment.offset < v.scope_end]
    return min(candidates, key=lambda v: v.scope_end - v.scope_start) if candidates else None


def generate(root):
    data = inventory(Path(root))
    data.variables.sort(key=lambda v: (GROUPS.index(types.group(v.path)), v.path, v.line, v.start))
    persistent = [v for v in data.variables if v.storage != "local"]
    temporary = [v for v in data.variables if v.storage == "local"]
    out = ["# 数据实例与指针索引", "",
           "查实际保存数据的入口：用 **Ctrl+F 搜索中文用途、指针变量名或成员名**。例如搜索 `speed_aps`，查看 `motor_lf->measure.speed_aps`；搜索 `power_w`，查看 `power_meter->status.power_w`。", "",
           f"扫描 {len(data.sources)} 个 C/H 文件，发现 **{len(persistent)}** 个持久实例/外部声明与 **{len(temporary)}** 个局部临时实例。主表优先列出文件级指针、指针数组和保存数据的结构体对象；函数参数和函数声明不作为独立数据入口。", "",
           "实际运行地址由固件装载及分配决定；本文给出变量名和 C 访问表达式。`static` 或局部变量需在对应源码/函数作用域查看；`extern` 是声明，不代表本文件定义存储。数组路径中的 `[0]` 是示例下标，实际使用时按有效范围选择。指针在相应初始化完成后才可能可用。", "",
           "直接赋值证据只表示当前文件里按词法作用域找到的 `变量 = 表达式`，不把成员写入、参数赋值当成根指针重新绑定，不分析任意函数调用的副作用。条件编译分支、便携副本和测试桩保留并按来源区分，不表示全部参与当前固件。", "",
           "类型有多个候选或遇到递归环时明确标记并停止深层路径推断。成员说明来自源码注释；没有注释时不猜测含义。类型定义与完整字段声明见[结构体与成员索引](structure-index.md)。", "",
           "更新本完整表：运行 `python scripts/generate-data-instance-index.py --full`；检查使用 `--full --check`。默认一键脚本只更新常用速查与类型索引。", "",
           "## 项目实例指针速查", "", "优先查看项目代码中保存数据的持久指针；更多结构体对象、extern 声明及其他来源见后续完整表。", "",
           "| 中文用途 | 指针入口 | 声明初始化 / 直接赋值 | 定义位置 |", "| --- | --- | --- | --- |"]
    for variable in persistent:
        if types.group(variable.path) != "项目源码" or variable.storage == "extern" or variable.pointers + alias_pointers(data, variable.type_name, variable.path) == 0:
            continue
        binding = variable.initializer or "；".join(a.expression for a in variable.assignments) or "见初始化代码"
        out.append(f"| {types.cell(purpose(variable))} | {variable_link(variable)} | {types.code(compact(binding))} | {types.source_link(variable.path, variable.line)} |")
    out += ["", "## 持久数据入口", "", "| 中文用途 | 实例变量 / 指针 | 保存或指向的类型 | 属性 / 作用域 | 定义位置 |", "| --- | --- | --- | --- | --- |"]
    for variable in persistent:
        out.append(f"| {types.cell(purpose(variable))} | {variable_link(variable)} | {'、'.join(type_link(r) for r in variable.targets)} | {types.cell(variable.storage + (' / ' + variable.function if variable.function else ' / 文件作用域'))} | {types.source_link(variable.path, variable.line)} |")
    out += ["", "## 成员名反查与访问路径", "", "只展开持久数据入口；局部临时实例另列。成员声明位置指向类型定义，实例定义位置指向持有该数据的变量。", "",
            "| 成员名 | 数据访问表达式 | 实例入口 | 中文用途 / 成员说明 | 成员声明位置 |", "| --- | --- | --- | --- | --- |"]
    paths = [(v, m, path, record, note) for v in persistent for m, path, record, note in member_paths(v, data)]
    for variable, member, path, record, note in sorted(paths, key=lambda row: ((row[1].name if row[1] else "").casefold(), GROUPS.index(types.group(row[0].path)), row[0].path, row[0].line, row[2])):
        description = "；".join(filter(None, [purpose(variable), member.comment if member else "", note]))
        out.append(f"| {types.code(member.name) if member else '类型歧义'} | {types.code(path)} | {variable_link(variable)} | {types.cell(description)} | {types.source_link(record.path, member.line) if member else '见类型候选'} |")
    out += ["", "## 实例定义与赋值证据", ""]
    for variable in persistent:
        expression, _ = access_base(variable, data)
        out += [f'<a id="{variable.anchor}"></a>', "", f"### {types.cell(variable.name)} · {types.cell(variable.path)}", "",
                f"- 中文用途：{types.cell(purpose(variable))}；源码说明：{types.cell(variable.comment) or '源码未注释'}。",
                f"- 属性：{types.cell(variable.storage)}；作用域：{types.cell(variable.function or '文件')}；定义：{types.source_link(variable.path, variable.line)}。",
                "- 类型：" + "、".join(type_link(r) for r in variable.targets) + "。",
                f"- 声明：{types.code(variable.declaration)}。",
                f"- 数据入口表达式：{types.code(expression)}" + (f"；对象地址：{types.code('&' + expression)}" if variable.pointers + alias_pointers(data, variable.type_name, variable.path) == 0 else "") + "。"]
        if variable.condition:
            out.append(f"- 源码条件：{types.code(variable.condition)}。")
        if variable.initializer:
            out.append(f"- 声明初始化：{types.code(compact(variable.initializer))}。")
        for assignment in variable.assignments:
            out.append(f"- 直接赋值：{types.code(assignment.target + ' = ' + assignment.expression)}；{types.cell(assignment.function)}；{types.source_link(variable.path, assignment.line)}。")
            origin = assignment_source(variable, assignment, data)
            if origin and origin is not variable:
                out.append(f"- 赋值来源变量：{variable_link(origin)}（{types.cell(origin.function or '文件作用域')}）；{types.source_link(origin.path, origin.line)}" + (f"；初始化：{types.code(compact(origin.initializer))}" if origin.initializer else "") + "。")
        if not variable.initializer and not variable.assignments:
            out.append("- 当前文件未找到直接初始化/赋值证据；函数写入、跨文件赋值及运行状态需查看源码。")
        out.append("")
    out += ["## 局部临时实例", "", "这些变量受函数/块作用域限制，可能是初始化中的临时指针、配置值或快照，不作为默认长期观测入口。", "",
            "| 变量 | 函数 | 类型 | 初始化 | 源码位置 |", "| --- | --- | --- | --- | --- |"]
    for variable in temporary:
        out.append(f'| <a id="{variable.anchor}"></a>{types.code(variable.name)} | {types.cell(variable.function)} | ' + f"{'、'.join(type_link(r) for r in variable.targets)} | {types.code(compact(variable.initializer)) if variable.initializer else '未显式初始化'} | {types.source_link(variable.path, variable.line)} |")
    return "\n".join(out) + "\n"


MOTOR_COMMON = [("角速度", "measure.speed_aps"), ("电流反馈", "measure.real_current"), ("累计角度", "measure.total_angle")]
COMMON_DATA = [
    ("遥控器", "application/cmd/robot_cmd.c", ["rc_data"],
     [("左摇杆横向", "rc.rocker_l_"), ("键盘", "key[0].keys")]),
    ("图传键鼠", "application/cmd/robot_cmd.c", ["vt02_data"],
     [("鼠标横向", "mouse.x"), ("键盘", "key[0].keys"), ("在线", "online")]),
    ("底盘电机", "application/chassis/chassis.c", ["motor_lf", "motor_rf", "motor_lb", "motor_rb"], MOTOR_COMMON),
    ("云台电机", "application/gimbal/gimbal.c", ["yaw_motor", "pitch_motor"], MOTOR_COMMON),
    ("发射电机", "application/shoot/shoot.c", ["friction_l", "friction_r", "loader"], MOTOR_COMMON),
    ("功率计", "modules/power_meter/power_meter.c", ["power_meter"],
     [("功率", "status.power_w"), ("电流", "status.current_a"), ("电压", "status.bus_voltage_v"), ("在线", "status.online")]),
    ("超级电容", "modules/super_cap/super_cap.c", ["cap"],
     [("剩余能量比例", "snapshot.status.energy_ratio"), ("输出使能", "snapshot.status.output_enabled"),
      ("故障码", "snapshot.status.error_code"), ("输入功率", "snapshot.input_power.input_power_w")]),
    ("裁判系统", "modules/referee/referee_task.c", ["referee_recv_info"],
     [("机器人 ID", "GameRobotState.robot_id"), ("血量", "GameRobotState.current_HP"),
      ("底盘功率上限", "GameRobotState.chassis_power_limit"), ("枪管热量", "PowerHeatData.shooter_17mm_barrel_heat")]),
    ("IMU 姿态", "application/gimbal/gimbal.c", ["gimbal_IMU_data"],
     [("偏航", "Yaw"), ("俯仰", "Pitch"), ("横滚", "Roll")]),
    ("底盘速度", "application/chassis/chassis.c", ["chassis_velocity"],
     [("前后速度", "actual.vx_mps"), ("左右速度", "actual.vy_mps"), ("旋转速度", "actual.wz_rpm")]),
]


COMMON_MACROS = [
    ("键盘状态索引", "KEY_PRESS", "modules/remote/remote_control.h", "例：`rc_data->key[KEY_PRESS].w`"),
    ("遥控器串口", "BOARD_REMOTE_UART", "Inc/board_config.h", "DBUS 使用 USART1"),
    ("图传串口", "BOARD_VTX_UART", "Inc/board_config.h", "图传键鼠使用 UART8"),
    ("裁判串口句柄", "BOARD_REFEREE_UART", "Inc/board_config.h", "由 `REFEREE_UART_PORT` 选择"),
    ("裁判系统开关", "REFEREE_ENABLE", "Inc/board_config.h", "0 关闭，1 开启"),
    ("裁判串口编号", "REFEREE_UART_PORT", "Inc/board_config.h", "6 为 USART6，7 为 UART7"),
    ("功率计开关", "POWER_METER_ENABLE", "modules/algorithm/chassis_power_config.h", "0 关闭，1 开启"),
    ("功控阶段", "CHASSIS_POWER_CONTROL_STAGE", "modules/algorithm/chassis_power_config.h", "0 原控制与诊断，1 快保护，2 启动保护，3 完整慢环"),
    ("底盘供电模式", "CHASSIS_POWER_SOURCE_MODE", "modules/algorithm/chassis_energy_config.h", "DIRECT=0 直供，SUPERCAP=1 超级电容"),
    ("缓冲能量增补", "CHASSIS_BUFFER_POWER_ENABLE", "modules/algorithm/chassis_energy_config.h", "0 关闭，1 开启"),
    ("小陀螺模式", "CHASSIS_SPIN_MODE", "modules/algorithm/chassis_spin_config.h", "FIXED=0 固定，ADAPTIVE=1 自适应，SINE=2 正弦"),
    ("固定自旋目标", "CHASSIS_SPIN_FIXED_WZ", "modules/algorithm/chassis_spin_config.h", "Wz 命令尺度，非 rpm 或物理角速度"),
    ("跟随档策略", "CHASSIS_FOLLOW_MODE", "application/robot_def.h", "0 车身跟转，1 云台朝向平移且不主动跟转"),
    ("云台底盘对齐", "YAW_CHASSIS_ALIGN_ECD", "application/robot_def.h", "单位：编码器计数"),
    ("俯仰水平标定", "PITCH_HORIZON_ECD", "application/robot_def.h", "单位：编码器计数"),
    ("USB 遥测开关", "USB_TELEMETRY_ENABLE", "modules/telemetry/usb_telemetry_config.h", "0 关闭，1 开启；USB 视觉需关闭遥测"),
    ("USB 遥测周期", "USB_TELEMETRY_PERIOD_MS", "modules/telemetry/usb_telemetry_config.h", "单位：ms"),
    ("日志输出", "LOG_OUTPUT", "Inc/board_config.h", "0 关闭，1 RTT，2 USART2，3 两者"),
]


COMMON_MODE_MACROS = [
    ("底盘轮型", "CHASSIS_DRIVE_TYPE", "application/robot_def.h", "MECANUM=0 麦克纳姆轮，OMNI_X=1 X 型全向轮"),
    *(s for s in COMMON_MACROS if s[1] in {"CHASSIS_SPIN_MODE", "CHASSIS_FOLLOW_MODE", "CHASSIS_POWER_SOURCE_MODE"}),
]
OPTIONAL_MODE_MACROS = [
    ("底盘控制方式", "CHASSIS_CONTROL_MODE", "modules/algorithm/chassis_control_config.h", "SPEED=0 速度环，FORCE=1 直接力控"),
    ("力控退出方式", "CHASSIS_FORCE_EXIT_MODE", "modules/algorithm/chassis_control_config.h", "COAST=0 滑行，BRAKE=1 制动"),
    ("力控自旋策略", "CHASSIS_FORCE_SPIN_MODE", "modules/algorithm/chassis_control_config.h", "FIXED_TORQUE=0 固定力矩，POWER_ADAPTIVE=1 功率自适应"),
]
COMMON_MODES = [
    ("底盘", "chassis_mode_e", "chassis_cmd_send.chassis_mode",
     [("CHASSIS_ZERO_FORCE", "零力"), ("CHASSIS_ROTATE", "小陀螺"),
      ("CHASSIS_NO_FOLLOW", "不跟随"), ("CHASSIS_FOLLOW_GIMBAL_YAW", "跟随档")]),
    ("云台", "gimbal_mode_e", "gimbal_cmd_send.gimbal_mode",
     [("GIMBAL_ZERO_FORCE", "零力"), ("GIMBAL_FREE_MODE", "自由模式，当前仍用 IMU 反馈"), ("GIMBAL_GYRO_MODE", "陀螺仪反馈")]),
    ("发射总开关", "shoot_mode_e", "shoot_cmd_send.shoot_mode",
     [("SHOOT_OFF", "关闭"), ("SHOOT_ON", "开启")]),
    ("摩擦轮", "friction_mode_e", "shoot_cmd_send.friction_mode",
     [("FRICTION_OFF", "关闭"), ("FRICTION_ON", "开启")]),
    ("拨弹", "loader_mode_e", "shoot_cmd_send.load_mode",
     [("LOAD_STOP", "停止"), ("LOAD_REVERSE", "反转保留值，当前按停止处理"), ("LOAD_1_BULLET", "单发"),
      ("LOAD_3_BULLET", "三发"), ("LOAD_BURSTFIRE", "连发")]),
]


def common_macro_rows(root, sources, specifications=COMMON_MACROS):
    rows = []
    for purpose, name, path, usage in specifications:
        source = sources.get(path)
        if source is None:
            source = types.Source(types.read_source(Path(root) / path), path)
            sources[path] = source
        definitions = source.macros.get(name, [])
        if not definitions:
            raise ValueError(f"{path}: 常用宏 {name} 未找到定义，请更新 COMMON_MACROS")
        # Hide the file guard and the selected macro's #ifndef fallback wrapper;
        # retain conditions that actually choose among different definitions.
        comments_removed = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/", "", source.text)
        guard = re.match(r"\s*(?:#\s*pragma\s+once\s*)?#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b", comments_removed)
        hidden = {f"#ifndef {name}"}
        if guard:
            hidden.add(f"#ifndef {guard.group(1)}")
        values, locations, spellings = [], [], []
        for parameters, value, condition, line in definitions:
            spelling = name + (f"({parameters})" if parameters is not None else "")
            if spelling not in spellings:
                spellings.append(spelling)
            condition = " / ".join(part for part in condition.split(" / ") if part not in hidden)
            values.append(types.code(" ".join(value.split())) + (f"（{types.code(condition)}）" if condition else ""))
            locations.append(types.source_link(path, line))
        rows.append(f"| {purpose} | {'、'.join(types.code(s) for s in spellings)} | {'<br>'.join(values)}<br>{usage} | {'<br>'.join(locations)} |")
    return rows


def common_mode_rows(sources):
    path = "application/robot_def.h"
    source = sources[path]
    enums = {}
    for i, token in enumerate(source.tokens):
        if token.group() != "enum":
            continue
        opening = i + 1
        if opening < len(source.tokens) and types.IDENT.fullmatch(source.tokens[opening].group()):
            opening += 1
        if opening >= len(source.tokens) or source.tokens[opening].group() != "{":
            continue
        closing = types.closing_token(source, opening)
        end = closing + 1
        while end < len(source.tokens) and source.tokens[end].group() != ";":
            end += 1
        aliases = [t.group() for t in source.tokens[closing + 1:end]]
        members = {source.tokens[a].group(): source.line(source.tokens[a].start())
                   for a, _ in types.split_declarations(source, opening + 1, closing, ",")}
        for alias in aliases:
            if types.IDENT.fullmatch(alias):
                enums[alias] = members
    rows = []
    for purpose, enum, field, choices in COMMON_MODES:
        members = enums.get(enum, {})
        for name, _ in choices:
            if name not in members:
                raise ValueError(f"{path}: 模式 {enum} 的常量 {name} 未找到，请更新 COMMON_MODES")
        options = "<br>".join(types.code(name) + "：" + meaning for name, meaning in choices)
        rows.append(f"| {purpose} | {types.code(field)} | {options} | {types.source_link(path, members[choices[0][0]])} |")
    return rows


def generate_common(root):
    data = inventory(Path(root))
    out = ["# 常用指针与宏速查", "",
           "用 **Ctrl+F** 搜中文用途、指针名、成员名或宏名。只列日常调试常用的数据入口和宏。", "",
           "| 数据 | 常用指针 | 常看成员 / 访问示例 | 定义位置 |", "| --- | --- | --- | --- |"]
    for label, path, names, fields in COMMON_DATA:
        variables = [v for name in names for v in data.variables if v.path == path and v.name == name and not v.function and v.storage != "extern"]
        if not variables:
            continue
        examples = []
        for variable in variables:
            base, operator = access_base(variable, data)
            available = {expression for _, expression, _, _ in member_paths(variable, data)}
            for meaning, suffix in fields:
                expression = base + operator + suffix
                if expression not in available:
                    raise ValueError(f"{path}: 常用指针 {variable.name} 的成员路径已变化: {expression}")
                if variable is variables[0]:
                    examples.append(types.cell(meaning) + "：" + types.code(expression))
        definitions = sorted({(v.path, v.line) for v in variables})
        out.append(f"| {label} | {'、'.join(types.code(v.name) for v in variables)} | {'<br>'.join(examples)} | "
                   + "、".join(types.source_link(p, line) for p, line in definitions) + " |")
    out += ["", "电机同组共用 `measure` 成员，示例中的指针可替换成同一行的其他电机指针。`lf/rf/lb/rb` 分别为左前/右前/左后/右后；`yaw/pitch` 为偏航/俯仰；`friction_l/friction_r/loader` 为左摩擦轮/右摩擦轮/拨弹电机。", "",
            "`rc_data` 指向控制任务中的遥控器快照，示例查看当前数据。`static` 指针在对应源码作用域查看，并需完成相应初始化。", "",
            "## 模式切换", ""]
    # Only expose optional mode switches after their owner integrates them into
    # the shared build-option list. A new header alone may be unfinished work.
    options_path = Path(root) / "config/atype-options.txt"
    exposed = set(options_path.read_text(encoding="utf-8").split()) if options_path.is_file() else set()
    mode_macros = COMMON_MODE_MACROS + [s for s in OPTIONAL_MODE_MACROS if s[1] in exposed and (Path(root) / s[2]).is_file()]
    if mode_macros:
        out += ["编译时模式：修改后需重新构建、烧录。以下是源码默认值，不表示当前固件已启用对应功能。", "",
                "| 中文用途 | 宏名 | 源码默认值 / 用法 | 定义位置 |", "| --- | --- | --- | --- |"]
        out.extend(common_macro_rows(root, data.sources, mode_macros))
        out.append("")
    out += ["运行时模式值是枚举常量，赋给对应命令字段。例：`chassis_cmd_send.chassis_mode = CHASSIS_ROTATE;`（在命令模块内使用）。", "",
            "| 模式 | 赋值字段 | 可选模式值 | 定义位置 |", "| --- | --- | --- | --- |"]
    out.extend(common_mode_rows(data.sources))
    out += ["", "## 常用宏", "",
            "以下为**源码默认值**及分支条件，构建参数可覆盖配置宏，不代表当前固件的实际值。", "",
            "| 中文用途 | 宏名 | 源码默认值 / 用法 | 定义位置 |", "| --- | --- | --- | --- |"]
    mode_names = {s[1] for s in mode_macros}
    out.extend(common_macro_rows(root, data.sources, [s for s in COMMON_MACROS if s[1] not in mode_names]))
    out += ["", "更新：双击 `scripts/update-structure-index.cmd`。指针、宏和模式名单在生成脚本的 `COMMON_DATA`、`COMMON_MACROS`、`COMMON_MODE_MACROS`、`OPTIONAL_MODE_MACROS`、`COMMON_MODES` 中维护；更新会刷新源码值、条件与位置。", ""]
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--full", action="store_true", help="generate the optional exhaustive index instead of the common-data quick reference")
    args = parser.parse_args()
    output = args.root / ("docs/data-instance-index-full.md" if args.full else ".Doc/指针查找索引.md")
    try:
        markdown = generate(args.root) if args.full else generate_common(args.root)
        if args.check:
            if not output.exists() or output.read_text(encoding="utf-8") != markdown:
                command = "python scripts/generate-data-instance-index.py --full" if args.full else "scripts/update-structure-index.ps1"
                print(f"Data instance index is stale; run {command}", file=sys.stderr)
                return 1
            print("Data instance index is up to date.")
        else:
            types.write_document(output, markdown)
            print(f"Updated {output}")
        return 0
    except (ValueError, OSError) as error:
        print(f"Data instance index failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    raise SystemExit(main())
