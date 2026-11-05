"""Generate soft WiFi HAL stubs from I*.h pure virtuals (Bn* inherits BnCInterface<I*>)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

GEN_INCLUDE = Path(sys.argv[1])
OUT = Path(sys.argv[2])

# Only real binder interfaces we need stub base classes for.
INTERFACES = [
    "IWifi",
    "IWifiChip",
    "IWifiStaIface",
    "IWifiApIface",
    "IWifiNanIface",
    "IWifiP2pIface",
    "IWifiRttController",
    "IWifiEventCallback",
    "IWifiChipEventCallback",
    "IWifiStaIfaceEventCallback",
    "IWifiNanIfaceEventCallback",
    "IWifiRttControllerEventCallback",
]

METHOD_RE = re.compile(
    r"virtual ::ndk::ScopedAStatus\s+([A-Za-z0-9_]+)\((.*?)\)\s*(?:override\s*)?=\s*0;",
    re.S,
)
SKIP = {"getInterfaceVersion", "getInterfaceHash"}


def strip_attributes(text: str) -> str:
    out: list[str] = []
    i = 0
    while i < len(text):
        if text.startswith("__attribute__", i):
            j = text.find("((", i)
            if j < 0:
                out.append(text[i])
                i += 1
                continue
            depth = 0
            k = j
            while k < len(text):
                if text[k] == "(":
                    depth += 1
                elif text[k] == ")":
                    depth -= 1
                    if depth == 0:
                        k += 1
                        break
                k += 1
            i = k
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def parse_params(param_blob: str) -> list[tuple[str, str]]:
    if not param_blob.strip():
        return []
    params: list[str] = []
    depth = 0
    cur: list[str] = []
    for ch in param_blob:
        if ch == "<":
            depth += 1
            cur.append(ch)
        elif ch == ">":
            depth -= 1
            cur.append(ch)
        elif ch == "," and depth == 0:
            part = "".join(cur).strip()
            if part:
                params.append(part)
            cur = []
        else:
            cur.append(ch)
    part = "".join(cur).strip()
    if part:
        params.append(part)
    out: list[tuple[str, str]] = []
    for p in params:
        p = re.sub(r"\s+", " ", p).strip().split("=")[0].strip()
        m = re.match(r"^(.*\S)\s+([A-Za-z_][A-Za-z0-9_]*)$", p)
        if not m:
            raise SystemExit(f"bad param: {p!r}")
        out.append((m.group(1).strip(), m.group(2).strip()))
    return out


def default_out(typ: str, name: str) -> str:
    if "*" not in typ or typ.strip().startswith("const"):
        return ""
    inner = typ.rsplit("*", 1)[0].strip()
    if "shared_ptr" in inner:
        return f"    if ({name}) {name}->reset();\n"
    if "vector" in inner:
        return f"    if ({name}) {name}->clear();\n"
    if "string" in inner:
        return f"    if ({name}) {name}->clear();\n"
    if re.search(r"\bbool\b", inner):
        return f"    if ({name}) *{name} = false;\n"
    return f"    if ({name}) *{name} = {{}};\n"


wifi_dir = GEN_INCLUDE / "aidl/android/hardware/wifi"
parts = [
    "// Auto-generated soft WiFi HAL stubs. Do not edit.\n",
    "#pragma once\n",
    "#include <array>\n",
    "#include <memory>\n",
    "#include <string>\n",
    "#include <vector>\n",
]

n = 0
for iface in INTERFACES:
    iface_h = wifi_dir / f"{iface}.h"
    bn_h = wifi_dir / f"Bn{iface[1:]}.h"  # IWifi -> BnWifi
    if not iface_h.is_file() or not bn_h.is_file():
        print(f"skip missing {iface}", file=sys.stderr)
        continue
    text = strip_attributes(iface_h.read_text())
    methods = list(METHOD_RE.finditer(text))
    if not methods:
        print(f"skip no methods {iface}", file=sys.stderr)
        continue
    bn = f"Bn{iface[1:]}"
    cls = f"Stub{bn}"
    rel_bn = f"aidl/android/hardware/wifi/{bn}.h"
    parts.append(f"#include <{rel_bn}>\n")
    parts.append(f"class {cls} : public ::aidl::android::hardware::wifi::{bn} {{\npublic:\n")
    for meth in methods:
        name, blob = meth.group(1), meth.group(2)
        if name in SKIP:
            continue
        params = parse_params(blob)
        sig = ", ".join(f"{t} {n}" for t, n in params)
        parts.append(f"  ::ndk::ScopedAStatus {name}({sig}) override {{\n")
        for t, pname in params:
            parts.append(default_out(t, pname))
        parts.append("    return ::ndk::ScopedAStatus::ok();\n")
        parts.append("  }\n")
    parts.append("};\n\n")
    n += 1

OUT.write_text("".join(parts))
print(f"wrote {OUT} ({n} classes)")
