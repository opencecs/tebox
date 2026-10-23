"""Generate soft Radio HAL stubs that reply REQUEST_NOT_SUPPORTED via response callbacks."""
from __future__ import annotations

import re
import sys
from pathlib import Path

GEN_INCLUDE = Path(sys.argv[1])
OUT = Path(sys.argv[2])

HALS = [
    ("config", "RadioConfig", "IRadioConfigResponse", "IRadioConfigIndication"),
    ("modem", "RadioModem", "IRadioModemResponse", "IRadioModemIndication"),
    ("network", "RadioNetwork", "IRadioNetworkResponse", "IRadioNetworkIndication"),
    ("sim", "RadioSim", "IRadioSimResponse", "IRadioSimIndication"),
    ("data", "RadioData", "IRadioDataResponse", "IRadioDataIndication"),
    ("messaging", "RadioMessaging", "IRadioMessagingResponse", "IRadioMessagingIndication"),
    ("voice", "RadioVoice", "IRadioVoiceResponse", "IRadioVoiceIndication"),
]

METHOD_RE = re.compile(
    r"virtual ::ndk::ScopedAStatus\s+([A-Za-z0-9_]+)\((.*?)\)\s*(?:override\s*)?=\s*0;",
    re.S,
)


def strip_attributes(text: str) -> str:
    """Remove __attribute__((...)) which can contain parentheses inside strings."""
    out = []
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


def response_args(resp_params: list[tuple[str, str]], serial_expr: str) -> str:
    args = []
    for typ, _name in resp_params:
        if "RadioResponseInfoModem" in typ:
            args.append("{}")
        elif "RadioResponseInfo" in typ:
            args.append(
                "::aidl::android::hardware::radio::RadioResponseInfo{"
                "::aidl::android::hardware::radio::RadioResponseType::SOLICITED, "
                f"{serial_expr}, "
                "::aidl::android::hardware::radio::RadioError::REQUEST_NOT_SUPPORTED}"
            )
        else:
            args.append("{}")
    return ", ".join(args)


parts = [
    "// Auto-generated soft Radio HAL stubs. Do not edit.\n",
    "#pragma once\n",
    "#include <aidl/android/hardware/radio/RadioError.h>\n",
    "#include <aidl/android/hardware/radio/RadioResponseInfo.h>\n",
    "#include <aidl/android/hardware/radio/RadioResponseType.h>\n",
    "#include <memory>\n",
]

for pkg, iface, resp_iface, ind_iface in HALS:
    ns = f"aidl::android::hardware::radio::{pkg}"
    bn_rel = f"aidl/android/hardware/radio/{pkg}/Bn{iface}.h"
    iface_rel = f"aidl/android/hardware/radio/{pkg}/I{iface}.h"
    resp_rel = f"aidl/android/hardware/radio/{pkg}/{resp_iface}.h"
    ind_rel = f"aidl/android/hardware/radio/{pkg}/{ind_iface}.h"
    iface_h = strip_attributes((GEN_INCLUDE / iface_rel).read_text())
    resp_h = strip_attributes((GEN_INCLUDE / resp_rel).read_text())
    resp_methods = {m.group(1): parse_params(m.group(2)) for m in METHOD_RE.finditer(resp_h)}
    iface_methods = list(METHOD_RE.finditer(iface_h))
    if not iface_methods:
        raise SystemExit(f"no methods in {iface_rel}")

    parts.append(f"#include <{bn_rel}>\n")
    parts.append(f"#include <{resp_rel}>\n")
    parts.append(f"#include <{ind_rel}>\n")
    cls = f"Qemu{iface}"
    parts.append(f"class {cls} : public {ns}::Bn{iface} {{\npublic:\n")

    for m in iface_methods:
        name, param_blob = m.group(1), m.group(2)
        if name in ("getInterfaceVersion", "getInterfaceHash"):
            continue
        params = parse_params(param_blob)
        sig_params = ", ".join(f"{t} {n}" for t, n in params)
        parts.append(f"  ::ndk::ScopedAStatus {name}({sig_params}) override {{\n")
        if name == "setResponseFunctions":
            parts.append(f"    response_ = {params[0][1]};\n")
            parts.append(f"    indication_ = {params[1][1]};\n")
            parts.append("    return ::ndk::ScopedAStatus::ok();\n")
        elif name == "responseAcknowledgement":
            parts.append("    return ::ndk::ScopedAStatus::ok();\n")
        else:
            serial = next(
                (n for t, n in params if t in ("int32_t", "int") and "serial" in n),
                None,
            )
            resp_name = name + "Response"
            if serial and resp_name in resp_methods:
                args = response_args(resp_methods[resp_name], serial)
                parts.append("    if (response_) {\n")
                parts.append(f"      response_->{resp_name}({args});\n")
                parts.append("    }\n")
            parts.append("    return ::ndk::ScopedAStatus::ok();\n")
        parts.append("  }\n")

    parts.append(f"  std::shared_ptr<{ns}::{resp_iface}> response_;\n")
    parts.append(f"  std::shared_ptr<{ns}::{ind_iface}> indication_;\n")
    parts.append("};\n\n")

OUT.write_text("".join(parts))
print(f"wrote {OUT}")
