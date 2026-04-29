#!/usr/bin/env python3
"""Generate a deterministic widget documentation drift report."""

import argparse
import json
import os
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple
from xml.etree import ElementTree as ET


OBSOLETE_WIDGETS = {
    "multiscroll",
    "frameset",
    "calendar",
    "spinner",
    "terminal",
    "alerter",
    "formbar",
}

SEVERITY_HIGH = "high"
SEVERITY_MEDIUM = "medium"
SEVERITY_LOW = "low"


class RuntimeSignal:
    def __init__(self, name: str) -> None:
        self.name = name
        self.origins = set()
        self.confidence = "heuristic"
        self.params = set()
        self.evidence = []
        self.param_evidence = {}

    def merge(self, other: "RuntimeSignal") -> None:
        self.origins.update(other.origins)
        self.params.update(other.params)
        self.confidence = pick_confidence(self.confidence, other.confidence)
        self.evidence.extend(other.evidence)
        for param_name, refs in other.param_evidence.items():
            self.param_evidence.setdefault(param_name, []).extend(refs)


class WidgetDoc:
    def __init__(self, widget: str) -> None:
        self.widget = widget
        self.properties = set()
        self.events = set()
        self.actions = set()
        self.action_params = {}
        self.has_any_child = False
        self.location = None
        self.property_locations = {}
        self.event_locations = {}
        self.action_locations = {}
        self.child_locations = {}


class RuntimeWidget:
    def __init__(self, widget: str) -> None:
        self.widget = widget
        self.events = {}
        self.actions = {}
        self.sources = []


def normalize_widget_name(name: str) -> str:
    text = (name or "").strip().lower()
    if text.startswith("widget/"):
        text = text.split("/", 1)[1]
    return text


def normalize_name(name: str) -> str:
    return (name or "").strip()


def pick_confidence(a: str, b: str) -> str:
    order = {"confirmed": 3, "strong": 2, "heuristic": 1}
    return a if order.get(a, 0) >= order.get(b, 0) else b


def slug_origin(origins: Set[str]) -> str:
    if origins == {"c"}:
        return "c"
    if origins == {"js"}:
        return "js"
    if origins == {"c", "js"}:
        return "c+js"
    return "unknown"


def sorted_list(values: Iterable[str]) -> List[str]:
    return sorted(set(values), key=lambda v: v.lower())


def line_of_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def make_ref(path: str, line: Optional[int], reason: str) -> Dict[str, object]:
    return {"path": path, "line": line, "reason": reason}


def fmt_ref(ref: Dict[str, object]) -> str:
    if ref.get("line"):
        return "`%s:%s` (%s)" % (ref["path"], ref["line"], ref["reason"])
    return "`%s` (%s)" % (ref["path"], ref["reason"])


def unique_refs(refs: Iterable[Dict[str, object]]) -> List[Dict[str, object]]:
    seen = set()
    out = []
    for ref in refs:
        key = (ref.get("path"), ref.get("line"), ref.get("reason"))
        if key in seen:
            continue
        seen.add(key)
        out.append(ref)
    return out


def markdown_link_for_ref(report_dir: Path, repo_root: Path, ref: Dict[str, object]) -> str:
    target_path = Path(str(ref.get("path", "")))
    abs_target = (repo_root / target_path).resolve()
    href = os.path.relpath(str(abs_target), str(report_dir.resolve())).replace("\\", "/")
    line = ref.get("line")
    if line:
        href = "%s#L%s" % (href, line)
    label = target_path.name
    if line:
        label = "%s:%s" % (label, line)
    return "[%s](%s) (%s)" % (label, href, ref.get("reason", "source"))


def normalize_js_widget_name(stem_name: str) -> str:
    text = normalize_widget_name(stem_name)
    aliases = {
        "componentdecl": "component-decl",
        "sys_osml": "sys-osml",
        "page_js15": "page",
        "checkbox_moz": "checkbox",
    }
    return aliases.get(text, text)


def extract_widget_doc_locations(path: Path) -> Dict[str, Dict[str, object]]:
    content = path.read_text(encoding="utf-8", errors="ignore")
    results = {}
    pattern = re.compile(r"<widget\b([^>]*)>", re.IGNORECASE)
    attr_re = re.compile(r'([A-Za-z_][A-Za-z0-9_-]*)\s*=\s*"([^"]*)"')
    prop_re = re.compile(r'<property\b[^>]*\bname\s*=\s*"([^"]+)"', re.IGNORECASE)
    event_re = re.compile(r'<event\b[^>]*\bname\s*=\s*"([^"]+)"', re.IGNORECASE)
    action_re = re.compile(r'<action\b[^>]*\bname\s*=\s*"([^"]+)"', re.IGNORECASE)
    child_re = re.compile(r'<child\b[^>]*\btype\s*=\s*"([^"]+)"', re.IGNORECASE)

    for match in pattern.finditer(content):
        attrs = dict(attr_re.findall(match.group(1)))
        widget_name = normalize_widget_name(attrs.get("name") or attrs.get("type") or "")
        if not widget_name:
            continue
        start = match.start()
        end = content.find("</widget>", start)
        if end < 0:
            end = len(content)
        block = content[start:end]
        base_line = line_of_offset(content, start)
        node = {
            "location": make_ref(
                "centrallix-doc/Widgets/widgets.xml", base_line, "widget definition"
            ),
            "properties": {},
            "events": {},
            "actions": {},
            "children": {},
        }
        for pm in prop_re.finditer(block):
            line = base_line + block.count("\n", 0, pm.start())
            node["properties"][normalize_name(pm.group(1))] = make_ref(
                "centrallix-doc/Widgets/widgets.xml", line, "documented property"
            )
        for em in event_re.finditer(block):
            line = base_line + block.count("\n", 0, em.start())
            node["events"][normalize_name(em.group(1))] = make_ref(
                "centrallix-doc/Widgets/widgets.xml", line, "documented event"
            )
        for am in action_re.finditer(block):
            line = base_line + block.count("\n", 0, am.start())
            node["actions"][normalize_name(am.group(1))] = make_ref(
                "centrallix-doc/Widgets/widgets.xml", line, "documented action"
            )
        for cm in child_re.finditer(block):
            line = base_line + block.count("\n", 0, cm.start())
            child_name = _normalize_child_type(cm.group(1))
            if child_name:
                node["children"][child_name] = make_ref(
                    "centrallix-doc/Widgets/widgets.xml", line, "documented child type"
                )
        results[widget_name] = node
    return results


def _normalize_child_type(child_type: str) -> Optional[str]:
    text = normalize_widget_name(child_type)
    if not text:
        return None
    if text in {"any", "formelement"}:
        return None
    if text.startswith("/"):
        return None
    if re.search(r"[\s()]", text):
        return None
    if not re.match(r"^[a-z0-9][a-z0-9_-]*$", text):
        return None
    return text


def extract_documented_widgets(path: Path) -> Tuple[Dict[str, WidgetDoc], Set[str]]:
    tree = ET.parse(path)
    root = tree.getroot()
    docs: Dict[str, WidgetDoc] = {}
    covered_widget_types: Set[str] = set()
    doc_locations = extract_widget_doc_locations(path)
    for widget in root.findall(".//widget"):
        widget_name = normalize_widget_name(
            widget.get("name") or widget.get("type") or ""
        )
        if not widget_name:
            continue
        node = WidgetDoc(widget=widget_name)
        covered_widget_types.add(widget_name)
        if widget_name in doc_locations:
            node.location = doc_locations[widget_name]["location"]
            node.property_locations = doc_locations[widget_name]["properties"]
            node.event_locations = doc_locations[widget_name]["events"]
            node.action_locations = doc_locations[widget_name]["actions"]
            node.child_locations = doc_locations[widget_name]["children"]

        for prop in widget.findall("./properties/property"):
            prop_name = normalize_name(prop.get("name") or "")
            if prop_name:
                node.properties.add(prop_name)

        for event in widget.findall("./events/event"):
            event_name = normalize_name(event.get("name") or "")
            if event_name:
                node.events.add(event_name)

        for action in widget.findall("./actions/action"):
            action_name = normalize_name(action.get("name") or "")
            if not action_name:
                continue
            node.actions.add(action_name)
            # widgets.xml currently does not encode structured params, so infer a
            # weak signal from quoted parameter names in prose.
            raw_text = "".join(action.itertext())
            quoted = re.findall(r"[\"']([A-Za-z_][A-Za-z0-9_]*)[\"']", raw_text)
            if quoted:
                node.action_params[action_name] = set(quoted)

        for child in widget.findall("./children/child"):
            raw_child_type = normalize_name(child.get("type") or "").lower()
            if raw_child_type == "any":
                node.has_any_child = True
            child_type = _normalize_child_type(raw_child_type)
            if child_type:
                covered_widget_types.add(child_type)

        docs[widget_name] = node
    return docs, covered_widget_types


def extract_widget_types_from_wgtr(
    path: Path,
) -> Tuple[Set[str], Dict[str, Set[str]], Dict[str, List[Dict[str, object]]]]:
    widget_types: Set[str] = set()
    families: Dict[str, Set[str]] = {}
    refs: Dict[str, List[Dict[str, object]]] = {}
    pattern = re.compile(r'wgtrAddType\(\s*[^,]+,\s*"([^"]+)"\s*\)')
    for c_file in sorted(path.glob("wgtdrv_*.c")):
        content = c_file.read_text(encoding="utf-8", errors="ignore")
        rel = "centrallix/wgtr/%s" % c_file.name
        file_types = []
        for m in pattern.finditer(content):
            file_types.append(normalize_widget_name(m.group(1)))
            widget = normalize_widget_name(m.group(1))
            if widget:
                refs.setdefault(widget, []).append(
                    make_ref(rel, line_of_offset(content, m.start()), "wgtrAddType")
                )
        file_set = set([name for name in file_types if name])
        for name in file_set:
            widget_types.add(name)
            families[name] = set(file_set)
    return widget_types, families, refs


def expand_any_child_coverage(
    docs: Dict[str, WidgetDoc],
    covered_widget_types: Set[str],
    widget_families: Dict[str, Set[str]],
) -> Set[str]:
    expanded = set(covered_widget_types)
    for widget_name, node in docs.items():
        if not node.has_any_child:
            continue
        expanded.update(widget_families.get(widget_name, set()))
    return expanded


def _ensure_runtime_widget(store: Dict[str, RuntimeWidget], widget: str) -> RuntimeWidget:
    if widget not in store:
        store[widget] = RuntimeWidget(widget=widget)
    return store[widget]


def extract_runtime_from_c(path: Path) -> Dict[str, RuntimeWidget]:
    runtime: Dict[str, RuntimeWidget] = {}
    widget_pat = re.compile(r'strcpy\(\s*drv->WidgetName\s*,\s*"([^"]+)"\s*\)')
    event_pat = re.compile(r'htrAddEvent\(\s*drv\s*,\s*"([^"]+)"\s*\)')
    action_pat = re.compile(r'htrAddAction\(\s*drv\s*,\s*"([^"]+)"\s*\)')
    param_pat = re.compile(
        r'htrAddParam\(\s*drv\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*[^)]+\)'
    )

    for c_file in sorted(path.glob("htdrv_*.c")):
        content = c_file.read_text(encoding="utf-8", errors="ignore")
        rel = "centrallix/htmlgen/%s" % c_file.name
        widget_match = widget_pat.search(content)
        if not widget_match:
            continue
        widget = normalize_widget_name(widget_match.group(1))
        node = _ensure_runtime_widget(runtime, widget)
        node.sources.append(
            make_ref(rel, line_of_offset(content, widget_match.start()), "C driver widget name")
        )

        for event_m in event_pat.finditer(content):
            event_name = normalize_name(event_m.group(1))
            signal = node.events.get(event_name) or RuntimeSignal(name=event_name)
            signal.origins.add("c")
            signal.confidence = pick_confidence(signal.confidence, "confirmed")
            signal.evidence.append(
                make_ref(rel, line_of_offset(content, event_m.start()), "htrAddEvent")
            )
            node.events[event_name] = signal

        for action_m in action_pat.finditer(content):
            action_name = normalize_name(action_m.group(1))
            signal = node.actions.get(action_name) or RuntimeSignal(name=action_name)
            signal.origins.add("c")
            signal.confidence = pick_confidence(signal.confidence, "confirmed")
            signal.evidence.append(
                make_ref(rel, line_of_offset(content, action_m.start()), "htrAddAction")
            )
            node.actions[action_name] = signal

        for param_m in param_pat.finditer(content):
            action_name = normalize_name(param_m.group(1))
            param_name = normalize_name(param_m.group(2))
            signal = node.actions.get(action_name) or RuntimeSignal(name=action_name)
            signal.origins.add("c")
            signal.confidence = pick_confidence(signal.confidence, "confirmed")
            if param_name:
                signal.params.add(param_name)
                signal.param_evidence.setdefault(param_name, []).append(
                    make_ref(rel, line_of_offset(content, param_m.start()), "htrAddParam")
                )
            node.actions[action_name] = signal
    return runtime


def _extract_js_action_params(content: str, fn_name: str) -> List[Tuple[str, int]]:
    params: List[Tuple[str, int]] = []
    body_pat = re.compile(
        rf"function\s+{re.escape(fn_name)}\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\{{",
        re.MULTILINE,
    )
    start = body_pat.search(content)
    if not start:
        return params
    arg_name = start.group(1)
    idx = start.end()
    depth = 1
    while idx < len(content) and depth > 0:
        ch = content[idx]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        idx += 1
    body = content[start.end() : max(start.end(), idx - 1)]
    for pm in re.finditer(
        rf"\b{re.escape(arg_name)}\.([A-Za-z_][A-Za-z0-9_]*)", body
    ):
        pname = pm.group(1)
        pline = line_of_offset(content, start.end() + pm.start())
        params.append((pname, pline))
    return params


def extract_runtime_from_js(path: Path) -> Dict[str, RuntimeWidget]:
    runtime: Dict[str, RuntimeWidget] = {}
    add_iface_pat = re.compile(
        r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*[^;]*ifcProbeAdd\(\s*(ifAction|ifEvent)\s*\)"
    )
    add_call_pat = re.compile(
        r"([A-Za-z_][A-Za-z0-9_]*)\.Add\(\s*[\"']([^\"']+)[\"']\s*(?:,\s*([A-Za-z_][A-Za-z0-9_]*))?"
    )

    for js_file in sorted(path.glob("htdrv_*.js")):
        widget = normalize_js_widget_name(js_file.stem.replace("htdrv_", "", 1))
        node = _ensure_runtime_widget(runtime, widget)
        content = js_file.read_text(encoding="utf-8", errors="ignore")
        rel = "centrallix-os/sys/js/%s" % js_file.name
        node.sources.append(make_ref(rel, 1, "JS driver file"))

        iface_vars: Dict[str, str] = {}
        for var_name, iface in add_iface_pat.findall(content):
            iface_vars[var_name] = iface

        for add_m in add_call_pat.finditer(content):
            var_name = add_m.group(1)
            signal_name = add_m.group(2)
            fn_name = add_m.group(3)
            iface = iface_vars.get(var_name)
            if iface not in {"ifAction", "ifEvent"}:
                continue
            signal_name = normalize_name(signal_name)
            if not signal_name:
                continue

            confidence = "strong" if fn_name else "heuristic"
            origins = {"js"}
            if iface == "ifEvent":
                signal = node.events.get(signal_name) or RuntimeSignal(name=signal_name)
                signal.origins.update(origins)
                signal.confidence = pick_confidence(signal.confidence, confidence)
                signal.evidence.append(
                    make_ref(rel, line_of_offset(content, add_m.start()), "ifEvent.Add")
                )
                node.events[signal_name] = signal
            else:
                signal = node.actions.get(signal_name) or RuntimeSignal(name=signal_name)
                signal.origins.update(origins)
                signal.confidence = pick_confidence(signal.confidence, confidence)
                if fn_name:
                    for param_name, param_line in _extract_js_action_params(content, fn_name):
                        signal.params.add(param_name)
                        signal.param_evidence.setdefault(param_name, []).append(
                            make_ref(rel, param_line, "action param use in JS")
                        )
                signal.evidence.append(
                    make_ref(rel, line_of_offset(content, add_m.start()), "ifAction.Add")
                )
                node.actions[signal_name] = signal
    return runtime


def merge_runtime(
    c_runtime: Dict[str, RuntimeWidget], js_runtime: Dict[str, RuntimeWidget]
) -> Dict[str, RuntimeWidget]:
    merged: Dict[str, RuntimeWidget] = {}
    all_widgets = set(c_runtime) | set(js_runtime)
    for widget in all_widgets:
        node = RuntimeWidget(widget=widget)
        c_node = c_runtime.get(widget)
        j_node = js_runtime.get(widget)
        for source in (c_node, j_node):
            if not source:
                continue
            node.sources.extend(source.sources)
            for name, signal in source.events.items():
                existing = node.events.get(name) or RuntimeSignal(name=name)
                existing.merge(signal)
                node.events[name] = existing
            for name, signal in source.actions.items():
                existing = node.actions.get(name) or RuntimeSignal(name=name)
                existing.merge(signal)
                node.actions[name] = existing
        merged[widget] = node
    return merged


def severity_for(origins: Set[str], category: str) -> str:
    if "c" in origins:
        return SEVERITY_HIGH
    if category in {"missing_widget_docs", "missing_action", "missing_event"}:
        return SEVERITY_MEDIUM
    return SEVERITY_LOW


def compute_drift(
    docs: Dict[str, WidgetDoc],
    documented_coverage: Set[str],
    widget_types: Set[str],
    widget_type_refs: Dict[str, List[Dict[str, object]]],
    runtime: Dict[str, RuntimeWidget],
    obsolete_widgets: Set[str],
) -> Dict[str, object]:
    doc_widgets = set(documented_coverage) - obsolete_widgets
    fully_documented_widgets = set(docs) - obsolete_widgets
    code_widgets = (set(widget_types) | set(runtime)) - obsolete_widgets

    missing_widget_docs = sorted_list(code_widgets - doc_widgets)
    stale_doc_widgets = sorted_list(doc_widgets - code_widgets)
    missing_widget_doc_refs = {}
    stale_widget_doc_refs = {}
    for widget in missing_widget_docs:
        refs = []
        refs.extend(widget_type_refs.get(widget, []))
        if widget in runtime:
            refs.extend(runtime[widget].sources)
        missing_widget_doc_refs[widget] = unique_refs(refs)
    for widget in stale_doc_widgets:
        doc = docs.get(widget)
        stale_widget_doc_refs[widget] = [doc.location] if doc and doc.location else []

    per_widget: List[Dict[str, object]] = []
    triage = sorted_list(fully_documented_widgets & code_widgets)
    for widget in triage:
        doc = docs.get(widget, WidgetDoc(widget=widget))
        run = runtime.get(widget, RuntimeWidget(widget=widget))

        runtime_events = set(run.events)
        runtime_actions = set(run.actions)
        doc_events = set(doc.events)
        doc_actions = set(doc.actions)

        missing_events = sorted_list(runtime_events - doc_events)
        extra_events = sorted_list(doc_events - runtime_events)
        missing_actions = sorted_list(runtime_actions - doc_actions)
        extra_actions = sorted_list(doc_actions - runtime_actions)
        extra_properties = sorted_list(doc.properties if widget in stale_doc_widgets else [])

        action_param_drift: List[Dict[str, object]] = []
        for action_name in sorted_list(runtime_actions | doc_actions):
            runtime_params = set(run.actions.get(action_name, RuntimeSignal(action_name)).params)
            doc_params = set(doc.action_params.get(action_name, set()))
            missing_params = sorted_list(runtime_params - doc_params)
            extra_params = sorted_list(doc_params - runtime_params)
            if missing_params or extra_params:
                runtime_ref = []
                doc_ref = []
                for p in missing_params:
                    runtime_ref.extend(
                        run.actions.get(action_name, RuntimeSignal(action_name)).param_evidence.get(
                            p, []
                        )
                    )
                for p in extra_params:
                    if action_name in doc.action_locations:
                        doc_ref.append(doc.action_locations[action_name])
                action_param_drift.append(
                    {
                        "action": action_name,
                        "missing_in_docs": missing_params,
                        "extra_in_docs": extra_params,
                        "origin": slug_origin(
                            run.actions.get(action_name, RuntimeSignal(action_name)).origins
                        ),
                        "confidence": run.actions.get(
                            action_name, RuntimeSignal(action_name)
                        ).confidence,
                        "severity": severity_for(
                            run.actions.get(action_name, RuntimeSignal(action_name)).origins,
                            "param_mismatch",
                        ),
                        "references_runtime": unique_refs(runtime_ref),
                        "references_docs": unique_refs(doc_ref),
                    }
                )

        if (
            missing_events
            or extra_events
            or missing_actions
            or extra_actions
            or action_param_drift
            or extra_properties
        ):
            per_widget.append(
                {
                    "widget": widget,
                    "links": {
                        "docs": [doc.location] if doc.location else [],
                        "c": [
                            ref
                            for ref in unique_refs(run.sources)
                            if str(ref.get("path", "")).startswith("centrallix/htmlgen/")
                        ],
                        "js": [
                            ref
                            for ref in unique_refs(run.sources)
                            if str(ref.get("path", "")).startswith("centrallix-os/sys/js/")
                        ],
                    },
                    "status": {
                        "missing_widget_docs": False,
                        "stale_doc_widget": False,
                    },
                    "missing_events": [
                        {
                            "name": name,
                            "origin": slug_origin(run.events[name].origins),
                            "confidence": run.events[name].confidence,
                            "severity": severity_for(run.events[name].origins, "missing_event"),
                            "references": unique_refs(run.events[name].evidence),
                        }
                        for name in missing_events
                    ],
                    "extra_events": [
                        {
                            "name": name,
                            "references": [
                                doc.event_locations[name]
                            ]
                            if name in doc.event_locations
                            else [],
                        }
                        for name in extra_events
                    ],
                    "missing_actions": [
                        {
                            "name": name,
                            "origin": slug_origin(run.actions[name].origins),
                            "confidence": run.actions[name].confidence,
                            "severity": severity_for(run.actions[name].origins, "missing_action"),
                            "references": unique_refs(run.actions[name].evidence),
                        }
                        for name in missing_actions
                    ],
                    "extra_actions": [
                        {
                            "name": name,
                            "references": [
                                doc.action_locations[name]
                            ]
                            if name in doc.action_locations
                            else [],
                        }
                        for name in extra_actions
                    ],
                    "extra_properties": [
                        {
                            "name": name,
                            "references": [
                                doc.property_locations[name]
                            ]
                            if name in doc.property_locations
                            else [],
                        }
                        for name in extra_properties
                    ],
                    "action_param_drift": action_param_drift,
                }
            )

    return {
        "metadata": {
            "obsolete_widgets": sorted_list(obsolete_widgets),
            "counts": {
                "documented_widgets": len(doc_widgets),
                "fully_documented_widgets": len(fully_documented_widgets),
                "code_widgets": len(code_widgets),
                "missing_widget_docs": len(missing_widget_docs),
                "stale_doc_widgets": len(stale_doc_widgets),
                "widgets_with_findings": len(per_widget),
            },
        },
        "global_findings": {
            "missing_widget_docs": [
                {"widget": w, "references": missing_widget_doc_refs.get(w, [])}
                for w in missing_widget_docs
            ],
            "stale_doc_widgets": [
                {"widget": w, "references": stale_widget_doc_refs.get(w, [])}
                for w in stale_doc_widgets
            ],
        },
        "per_widget": per_widget,
    }


def write_json(path: Path, report: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")


def write_markdown(path: Path, report: Dict[str, object], repo_root: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    report_dir = path.parent
    lines: List[str] = []
    lines.append("# Widget Documentation Drift Report")
    lines.append("")
    lines.append(
        "This report is generated by `doc-report/generate.py` and is report-only."
    )
    lines.append(
        "Fixing documentation issues is intentionally deferred to a future project."
    )
    lines.append("")
    counts = report["metadata"]["counts"]
    lines.append("## Counts")
    lines.append("")
    lines.append(f"- Documented widgets: {counts['documented_widgets']}")
    lines.append(f"- Code widgets: {counts['code_widgets']}")
    lines.append(f"- Missing widget docs: {counts['missing_widget_docs']}")
    lines.append(f"- Stale doc widgets: {counts['stale_doc_widgets']}")
    lines.append(f"- Widgets with findings: {counts['widgets_with_findings']}")
    lines.append("")

    globals_findings = report["global_findings"]
    lines.append("## Global Findings")
    lines.append("")
    lines.append("- Missing widget docs:")
    for entry in globals_findings["missing_widget_docs"]:
        lines.append("  - `%s`" % entry["widget"])
        refs = entry.get("references", [])
        if refs:
            for ref in refs:
                lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        else:
            lines.append("    - source: not found")
    lines.append("- Stale doc widgets:")
    for entry in globals_findings["stale_doc_widgets"]:
        lines.append("  - `%s`" % entry["widget"])
        refs = entry.get("references", [])
        if refs:
            for ref in refs:
                lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        else:
            lines.append("    - docs: not found")
    lines.append("")

    lines.append("## Per-Widget Findings")
    lines.append("")
    for item in report["per_widget"]:
        lines.append(f"### `{item['widget']}`")
        lines.append("")
        status = item["status"]
        lines.append(f"- Missing widget docs: `{status['missing_widget_docs']}`")
        lines.append(f"- Stale doc widget: `{status['stale_doc_widget']}`")
        links = item.get("links", {})
        doc_refs = links.get("docs", [])
        c_refs = links.get("c", [])
        js_refs = links.get("js", [])
        lines.append("- Locations:")
        if doc_refs:
            for ref in doc_refs:
                lines.append("  - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        else:
            lines.append("  - docs: not found")
        if c_refs:
            for ref in c_refs:
                lines.append("  - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        else:
            lines.append("  - c: not found")
        if js_refs:
            for ref in js_refs:
                lines.append("  - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        else:
            lines.append("  - js: not found")
        if item["missing_events"]:
            lines.append("- Missing events in docs:")
            for event in item["missing_events"]:
                lines.append(
                    f"  - `{event['name']}` "
                    f"(origin: `{event['origin']}`, "
                    f"confidence: `{event['confidence']}`, "
                    f"severity: `{event['severity']}`)"
                )
                if event.get("references"):
                    for ref in event["references"]:
                        lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        if item["extra_events"]:
            lines.append("- Extra events in docs:")
            for event in item["extra_events"]:
                lines.append("  - `%s`" % event["name"])
                for ref in event.get("references", []):
                    lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        if item["missing_actions"]:
            lines.append("- Missing actions in docs:")
            for action in item["missing_actions"]:
                lines.append(
                    f"  - `{action['name']}` "
                    f"(origin: `{action['origin']}`, "
                    f"confidence: `{action['confidence']}`, "
                    f"severity: `{action['severity']}`)"
                )
                if action.get("references"):
                    for ref in action["references"]:
                        lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        if item["extra_actions"]:
            lines.append("- Extra actions in docs:")
            for action in item["extra_actions"]:
                lines.append("  - `%s`" % action["name"])
                for ref in action.get("references", []):
                    lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        if item["extra_properties"]:
            lines.append("- Properties present only in docs:")
            for prop in item["extra_properties"]:
                lines.append("  - `%s`" % prop["name"])
                for ref in prop.get("references", []):
                    lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        if item["action_param_drift"]:
            lines.append("- Action parameter differences:")
            for drift in item["action_param_drift"]:
                lines.append(
                    f"  - `{drift['action']}` (origin: `{drift['origin']}`, "
                    f"confidence: `{drift['confidence']}`, severity: `{drift['severity']}`)"
                )
                if drift["missing_in_docs"]:
                    lines.append(
                        f"    - missing in docs: {', '.join(f'`{x}`' for x in drift['missing_in_docs'])}"
                    )
                    for ref in drift.get("references_runtime", []):
                        lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
                if drift["extra_in_docs"]:
                    lines.append(
                        f"    - extra in docs: {', '.join(f'`{x}`' for x in drift['extra_in_docs'])}"
                    )
                    for ref in drift.get("references_docs", []):
                        lines.append("    - %s" % markdown_link_for_ref(report_dir, repo_root, ref))
        lines.append("")

    with path.open("w", encoding="utf-8") as handle:
        handle.write("\n".join(lines).rstrip() + "\n")


def resolve_paths(repo_root: Path, out_dir: Optional[Path]) -> Tuple[Path, Path, Path, Path]:
    widgets_root = repo_root / "centrallix-doc" / "Widgets"
    doc_xml = widgets_root / "widgets.xml"
    wgtr_dir = repo_root / "centrallix" / "wgtr"
    c_driver_dir = repo_root / "centrallix" / "htmlgen"
    js_driver_dir = repo_root / "centrallix-os" / "sys" / "js"
    output_dir = out_dir or widgets_root / "doc-report"
    return doc_xml, wgtr_dir, c_driver_dir, js_driver_dir, output_dir


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    default_root = Path(__file__).resolve().parents[3]
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=default_root,
        help="Path to cx-git repo root (default: auto-detected)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Report output directory (default: Widgets/doc-report)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = args.repo_root.resolve()
    doc_xml, wgtr_dir, c_driver_dir, js_driver_dir, output_dir = resolve_paths(
        repo_root, args.out_dir
    )
    json_path = output_dir / "drift-report.json"
    md_path = output_dir / "drift-report.md"

    docs, documented_coverage = extract_documented_widgets(doc_xml)
    widget_types, widget_families, widget_type_refs = extract_widget_types_from_wgtr(wgtr_dir)
    documented_coverage = expand_any_child_coverage(
        docs, documented_coverage, widget_families
    )
    c_runtime = extract_runtime_from_c(c_driver_dir)
    js_runtime = extract_runtime_from_js(js_driver_dir)
    runtime = merge_runtime(c_runtime, js_runtime)
    report = compute_drift(
        docs,
        documented_coverage,
        widget_types,
        widget_type_refs,
        runtime,
        set(OBSOLETE_WIDGETS),
    )
    write_json(json_path, report)
    write_markdown(md_path, report, repo_root)

    print(f"Generated: {json_path}")
    print(f"Generated: {md_path}")
    print(
        "Documentation fixes are deferred; this tool only reports drift "
        "between docs and source/runtime."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
