#!/usr/bin/env python3
"""
Generate a deterministic widget documentation report. For more info, see
centrallix-doc/Widgets/doc-report/doc-reports-guide.md
"""

from __future__ import annotations

import argparse
import json
import os
import re
from bisect import bisect_right
from dataclasses import dataclass, field
from enum import StrEnum
from pathlib import Path
from typing import Callable, Iterable, Optional, Pattern, TypedDict
from xml.etree import ElementTree


# =============
# Magic Value Setup
id_val = 1
def get_id():
	global id_val
	cur_id = id_val
	id_val += 1
	return cur_id

IGNORE_MISSING_WIDGET_DOCS = get_id()
IGNORE_STALE_WIDGET_DOCS = get_id()
IGNORE_MISSING_EVENT_DOCS = get_id()
IGNORE_STALE_EVENT_DOCS = get_id()
IGNORE_MISSING_ACTION_DOCS = get_id()
IGNORE_STALE_ACTION_DOCS = get_id()
IGNORE_MISSING_ACTION_PARAM_DOCS = get_id()
IGNORE_STALE_ACTION_PARAM_DOCS = get_id()


# =============
# Configs

# Widgets that use other names in the code.  This dictionary maps normalized
# widget alias(es) (e.g. "page_js15") to canonical widget names (e.g. "page").
WIDGETS_ALIASES = {
	"componentdecl": "component-decl",
	"sys_osml": "sys-osml",
	"page_js15": "page",
	"checkbox_moz": "checkbox",
	"radiobutton": "radiobuttonpanel", # May cause issues with the radiobutton widget (child of radiobuttonpanel).
	"window": "childwindow", # May cause issues with window widgets implemented in uawindow files.
}

# The normalized names of widgets that should be completely ignored if they
# appear in either source code or documentation.
IGNORED_WIDGETS = set({
	# Skipped
	# "",
	
	# Obsolete
	"alerter",
	"calendar",
	"formbar",
	"frameset",
	"multiscroll",
	"multiscrollpart",
	"remotectl",
	"remotemgr",
	"spinner",
	"uawindow",
})

# Uncomment any of the strings below to ignore that type of issue.
IGNORED_ERRORS: set[int] = set({
	# IGNORE_MISSING_WIDGET_DOCS,       # Ignore all undocumented widgets. (Not recommended.)
	# IGNORE_STALE_WIDGET_DOCS,         # Ignore all docs for unimplemented widgets. (Not recommended.)
	# IGNORE_MISSING_EVENT_DOCS,        # Ignore events with no docs.
	# IGNORE_STALE_EVENT_DOCS,          # Ignore events with no implementation.
	# IGNORE_MISSING_ACTION_DOCS,       # Ignore actions with no docs.
	# IGNORE_STALE_ACTION_DOCS,         # Ignore actions with no implementation.
	# IGNORE_MISSING_ACTION_PARAM_DOCS, # Ignore action parameter with no docs.
	# IGNORE_STALE_ACTION_PARAM_DOCS,   # Ignore action parameter with no implementations.
})

# Whether to write unminified, human-readable JSON.  Minified JSON is typically
# preferred because it is more readable to modern LLMs (it uses fewer tokens).
HUMAN_JSON = False

# Whether to write the JSON report.  If false, the JSON structure is only used
# internally and it is not written to disk.
WRITE_REPORT_JSON = True

# Whether to write the markdown report.  (Disabling both WRITE_REPORT_JSON and
# WRITE_REPORT_MD may be useful when microbenching performance.)
WRITE_REPORT_MD = True

# Widgets XML
WIDGET_XML_PATH = "centrallix-doc/Widgets/widgets.xml"


# =============
# Regexes.

# General Regexes
identifier_re = r"^[A-Za-z0-9][A-Za-z0-9_-]*$"
quoted_identifier_re = r"[\"'`]([A-Za-z_]\w*)[\"'`]"


# Regexes for parsing line numbers in docs.
doc_widget_re = re.compile(r"<widget\b([^>]*)>", re.IGNORECASE)
doc_attr_re = re.compile(r"([A-Za-z_][A-Za-z0-9_-]*)\s*=\s*(['\"])(.*?)\2")
doc_prop_re = re.compile(r"<property\b[^>]*\bname\s*=\s*(['\"])([^'\"]+)\1", re.IGNORECASE)
doc_event_re = re.compile(r"<event\b[^>]*\bname\s*=\s*(['\"])([^'\"]+)\1", re.IGNORECASE)
doc_action_re = re.compile(r"<action\b[^>]*\bname\s*=\s*(['\"])([^'\"]+)\1", re.IGNORECASE)
doc_child_re = re.compile(r"<child\b[^>]*\btype\s*=\s*(['\"])([^'\"]+)\1", re.IGNORECASE)


# Regexes for parsing implementations.

# Regex to find C wgtrAddType() calls and capture the type name (1).
c_register_re = re.compile(r'wgtrAddType\(\s*[^,]+,\s*"([^"]+)"\s*\)')

# Regex to find C strcpy() calls and capture the widget driver name (1).
c_name_re = re.compile(r'strcpy\(\s*(?:[A-Za-z_]\w*)\s*->\s*WidgetName\s*,\s*"([^"]+)"\s*\)')

# Regex to find C htrAddEvent() calls and capture the event name (1).
c_event_re = re.compile(r'htrAddEvent\(\s*(?:[A-Za-z_]\w*)\s*,\s*"([^"]+)"\s*\)')

# Regex to find C htrAddAction() calls and capture the action name (1).
c_action_re = re.compile(r'htrAddAction\(\s*(?:[A-Za-z_]\w*)\s*,\s*"([^"]+)"\s*\)')

# Regex to find C htrAddParam() calls and capture the action name (1) and param name (2).
c_param_re = re.compile(
	r'htrAddParam\(\s*(?:[A-Za-z_]\w*)\s*,\s*"([A-Za-z_]\w*)"\s*,\s*"([A-Za-z_]\w*)"\s*,\s*[^)]+\)'
)

# Regex to find JS ifcProbeAdd() calls and capture the returned variable name (1)
# and list (ifEvent or ifAction) (2).
js_add_iface_re = re.compile(
	r"([A-Za-z_]\w*)\s*=\s*[^;]*ifcProbeAdd\(\s*(ifEvent|ifAction)\s*\)"
)

# Regex to find JS Add() calls and capture the variable added (1) to and the
# name of the event/action added (2).
js_add_call_re = re.compile(
	r"([A-Za-z_]\w*)\.Add\(\s*[\"']([^\"']+)[\"']\s*(?:,\s*([A-Za-z_]\w*))?"
)

# Regex to find a JS function (fn_name) that implements an action and capture
# the first variable argument (1), even if it is an object deconstruction, and
# then ignore all other parameters.
js_action_impl_re: Callable[[str], Pattern[str]] = lambda fn_name: re.compile(
	rf"function\s+{re.escape(fn_name)}\s*\(\s*(\{{[^{{}}]*\}}|[A-Za-z_]\w*)(?=\s*[,)])[^)]*\)\s*\{{",
	re.MULTILINE
)

# Regex to find JS that reads properties from var_name and capture the property names.
js_property_re: Callable[[str], str] = lambda var_name: rf"\b{re.escape(var_name)}\.([A-Za-z_]\w*)"


# =============
# Types

class Confidence(StrEnum):
	CONFIRMED = "confirmed"
	STRONG = "strong"
	HEURISTIC = "heuristic"

# Reference a code location with a description of what is there.
class Ref(TypedDict):
	path: str
	line: int | None
	desc: str

# Stores how a widget is documented.
@dataclass
class WidgetDoc:
	name: str
	properties: set[str] = field(default_factory=set[str])
	events: set[str] = field(default_factory=set[str])
	actions: set[str] = field(default_factory=set[str])
	action_params: dict[str, set[str]] = field(default_factory=dict[str, set[str]])
	any_child: bool = False
	
	# Ref fields.
	ref: Ref | None = None
	property_refs: dict[str, Ref] = field(default_factory=dict[str, Ref])
	event_refs: dict[str, Ref] = field(default_factory=dict[str, Ref])
	action_refs: dict[str, Ref] = field(default_factory=dict[str, Ref])
	child_refs: dict[str, Ref] = field(default_factory=dict[str, Ref])

# Stores how a widget is implemented.
@dataclass
class WidgetImpl:
	widget_name: str
	events: dict[str, EventImpl] = field(default_factory=dict) # type: ignore (EventImpl is defined later)
	actions: dict[str, ActionImpl] = field(default_factory=dict)  # type: ignore (ActionImpl is defined later)
	definition_refs: list[Ref] = field(default_factory=list[Ref]) # Code locations where this widget is defined.
	
	# Register a unique event.
	def event(self, event_name: str) -> EventImpl:
		event = self.events.get(event_name) or EventImpl(name=event_name)
		self.events[event_name] = event
		return event
	
	# Register a unique action.
	def action(self, action_name: str)  -> ActionImpl:
		action = self.actions.get(action_name) or ActionImpl(name=action_name)
		self.actions[action_name] = action
		return action

# Stores one implemented signal, aka. an event or action.
@dataclass
class SignalImpl:
	name: str
	confidence: Confidence = Confidence.HEURISTIC
	definition_refs: list[Ref] = field(default_factory=list[Ref])
	params: set[str] = field(default_factory=set[str])
	params_refs: dict[str, list[Ref]] = field(default_factory=dict[str, list[Ref]])
	
	# Merge two implementations into one.
	def merge(self, other: SignalImpl) -> None:
		self.update_confidence(other.confidence)
		self.definition_refs.extend(other.definition_refs)
		self.params.update(other.params)
		for param_name, refs in other.params_refs.items():
			self.params_refs.setdefault(param_name, []).extend(refs)
	
	# Call when a new origin is found.
	def found(self, confidence : Confidence, ref: Ref) -> None:
		self.update_confidence(confidence)
		self.definition_refs.append(ref)
	
	# Update confidence if we're more confident now.
	def update_confidence(self, confidence: Confidence) -> None:
		self.confidence = merge_confidence(self.confidence, confidence)
	
	def add_param(self, param_name: str, ref: Ref):
		self.params.add(param_name)
		return self.params_refs.setdefault(param_name, []).append(ref)

@dataclass
class EventImpl(SignalImpl):
	pass

@dataclass
class ActionImpl(SignalImpl):
	pass


# Map character offsets to 1-based line number.
class LineMap:
	def __init__(self, text: str) -> None:
		self.newline_offsets = [match.start() for match in re.finditer("\n", text)]

	def line_number(self, offset: int) -> int:
		return bisect_right(self.newline_offsets, offset) + 1


# Report: Stores an event or action that differs between the docs and implementation.
class SignalIssueEntry(TypedDict):
	name: str
	confidence: Confidence
	refs: list[Ref]

# Report: Stores multiple diffs for events or actions, such as incorrect parameters.
class SignalIssuesEntry(TypedDict):
	signal_name: str # Associated event/action with these diffs.
	signal_refs: list[Ref]
	confidence: Confidence
	missing_param_refs: dict[str, Ref] # Named diffs.
	extra_param_refs: dict[str, Ref] # Named diffs.

# Report: Stores all findings for each widget in the report.
class PerWidgetFinding(TypedDict):
	widget: str
	refs: list[Ref]
	missing_events: list[SignalIssueEntry]
	extra_events: list[SignalIssueEntry]
	missing_actions: list[SignalIssueEntry]
	extra_actions: list[SignalIssueEntry]
	incorrect_action_params: list[SignalIssuesEntry]

# Report: Stores general summary statistics (see the top of report.md).
class ReportStats(TypedDict):
	documented_widgets: int
	implemented_widgets: int
	missing_widget_docs: int
	stale_widget_docs: int
	widgets_with_errors: int
	widget_errors: int
	ignored_errors: int

# Report: Stores a report showing all issues detected between docs and code.
class Report(TypedDict):
	stats: ReportStats
	missing_widget_docs: list[SignalIssueEntry]
	stale_widget_docs: list[SignalIssueEntry]
	per_widget: list[PerWidgetFinding]


# Trim names (preserves case).
def normalize_name(name: str | None) -> str:
	return (name or "").strip()


# Normalize `widget/foo` names to canonical keys.
def normalize_widget_name(name: str | None) -> str:
	text = normalize_name(name).lower()
	if text.startswith("widget/"):
		text = text.split("/", 1)[1]
	canonical_name = WIDGETS_ALIASES.get(text, text)
	return canonical_name if canonical_name not in IGNORED_WIDGETS else ""

# Keep the strongest confidence when combining multiple sources.
def merge_confidence(a: Confidence, b: Confidence) -> Confidence:
	order = {
		Confidence.CONFIRMED: 3,
		Confidence.STRONG: 2,
		Confidence.HEURISTIC: 1
	}
	return a if (order.get(a, 0) >= order.get(b, 0)) else b


# Render compact origin tags used in reports.
def get_origins(refs: Iterable[Ref]) -> str:
	return "+".join(sorted({
		r["path"].rsplit(".", 1)[-1]
		for r in refs
		if "." in r["path"]
	}))

# Sort arrays deterministically (ignores case).
def sorted_list(values: Iterable[str]) -> list[str]:
	return sorted(set(values), key=lambda v: v.lower())


# Convert character offsets to 1-based line numbers.
def get_line_number(text: str, offset: int) -> int:
	return text.count("\n", 0, offset) + 1


# Build a portable source-reference object.
def make_ref(path: str, line: Optional[int], desc: str) -> Ref:
	return {"path": path, "line": line, "desc": desc}


# Legacy text formatter (retained for potential plain-text output/debugging).
def fmt_ref(ref: Ref) -> str:
	if ref.get("line"):
		return f"`{ref['path']}:{ref['line']}` ({ref['desc']})"
	return f"`{ref['path']}` ({ref['desc']})"


# De-duplicate references while preserving first-seen order.
def unique_refs(refs: Iterable[Ref]) -> list[Ref]:
	return list({(r["path"], r["line"]): r for r in refs}.values())


# Format a reference as a markdown link relative to report output.
def ref_to_markdown_link(report_dir: Path, repo_root: Path, ref: Ref) -> str:
	target_path = Path(str(ref.get("path", "")))
	abs_target = (repo_root / target_path).resolve()
	href = os.path.relpath(str(abs_target), str(report_dir.resolve())).replace("\\", "/")
	label = target_path.name
	line = ref.get("line")
	if line != None:
		href = f"{href}#L{line}"
		label = f"{label}:{line}"
	return f"[{label}]({href}) ({ref.get('desc', 'source')})"


# Validate/normalize child type declarations into concrete widget keys.
def normalize_child_name(child_type: str) -> Optional[str]:
	text = normalize_widget_name(child_type)
	if text in {"", "any"}: return None
	if not re.match(identifier_re, text): return None
	return text


# Parse `widgets.xml`, returning a dict mapping widget names to their docs and
# a set of widget types (including children).
def parse_docs(path: Path) -> tuple[dict[str, WidgetDoc], set[str]]:
	# Allocate space for parsed documentation.
	docs: dict[str, WidgetDoc] = {}
	widget_types: set[str] = set()
	
	# Parse each widget entry to extract properties, events, actions, and children.
	xml_tree = ElementTree.parse(path)
	root = xml_tree.getroot()
	for widget in root.findall(".//widget"):
		widget_name = normalize_widget_name(widget.get("name") or widget.get("type"))
		if widget_name == "":
			continue
		
		# Create the widget doc structure.
		doc = WidgetDoc(name=widget_name)
		widget_types.add(widget_name)
		docs[widget_name] = doc
		
		# Parse properties.
		for prop in widget.findall("./properties/property"):
			prop_name = normalize_name(prop.get("name"))
			if prop_name:
				doc.properties.add(prop_name)
		
		# Parse events.
		for event in widget.findall("./events/event"):
			event_name = normalize_name(event.get("name"))
			if event_name:
				doc.events.add(event_name)
		
		# Parse actions.
		for action in widget.findall("./actions/action"):
			action_name = normalize_name(action.get("name"))
			if not action_name:
				continue
			doc.actions.add(action_name)
			
			# widgets.xml does not currently encode structured params, so use
			# quoted text as a heuristic for detecting parameter names.
			raw_text = "".join(action.itertext())
			quoted = re.findall(quoted_identifier_re, raw_text)
			if len(quoted) > 0:
				doc.action_params[action_name] = set(quoted)
		
		# Parse children.
		for child in widget.findall("./children/child"):
			raw_child_type = normalize_name(child.get("type")).lower()
			if raw_child_type == "any":
				doc.any_child = True
				continue
			child_type = normalize_child_name(raw_child_type)
			if child_type:
				widget_types.add(child_type)
	
	# Walk through xml text to get widget references.
	content = path.read_text(encoding="utf-8", errors="ignore")
	line_map = LineMap(content)
	for match in doc_widget_re.finditer(content):
		# Get the doc for the current widget
		attrs = {m.group(1): m.group(3) for m in doc_attr_re.finditer(match.group(1))}
		widget_name = normalize_widget_name(attrs.get("name") or attrs.get("type"))
		if widget_name == "":
			continue
		widget_doc = docs.get(widget_name)
		if not widget_doc:
			continue
		
		# Manually isolate this xml widget block.
		start = match.start()
		end = content.find("</widget>", start)
		if end < 0:
			end = len(content)
		block = content[start:end]
		base_line = line_map.line_number(start)
		
		# Manually parse the xml to get line numbers for refs in the widget block.
		widget_doc.ref = make_ref(WIDGET_XML_PATH, base_line, "widget definition")
		for property_match in doc_prop_re.finditer(block):
			line = line_map.line_number(start + property_match.start())
			name = normalize_name(property_match.group(2))
			widget_doc.property_refs[name] = make_ref(
				WIDGET_XML_PATH, line, "documented property"
			)
		for event_match in doc_event_re.finditer(block):
			line = line_map.line_number(start + event_match.start())
			name = normalize_name(event_match.group(2))
			widget_doc.event_refs[name] = make_ref(
				WIDGET_XML_PATH, line, "documented event"
			)
		for action_match in doc_action_re.finditer(block):
			line = line_map.line_number(start + action_match.start())
			name = normalize_name(action_match.group(2))
			widget_doc.action_refs[name] = make_ref(
				WIDGET_XML_PATH, line, "documented action"
			)
		for cm in doc_child_re.finditer(block):
			line = line_map.line_number(start + cm.start())
			child_name = normalize_child_name(cm.group(2))
			if child_name:
				widget_doc.child_refs[child_name] = make_ref(
					WIDGET_XML_PATH, line, "documented child type"
				)
	
	return docs, widget_types


# Scan `wgtdrv_*.c` for widget type registrations and type-family groupings.
def parse_widgets_in_wgtr(
	path: Path,
) -> tuple[set[str], dict[str, set[str]], dict[str, list[Ref]]]:
	widget_types: set[str] = set()
	widget_families: dict[str, set[str]] = {}
	refs: dict[str, list[Ref]] = {}
	
	# Search every driver file for registrations.
	for c_file in sorted(path.glob("wgtdrv_*.c")):
		content = c_file.read_text(encoding="utf-8", errors="ignore")
		rel = "centrallix/wgtr/%s" % c_file.name
		widget_names: set[str] = set()
		
		# Search for matching widget registrations.
		line_map = LineMap(content)
		for match in c_register_re.finditer(content):
			widget_name = normalize_widget_name(match.group(1))
			if widget_name == "":
				continue
			
			# Add data.
			widget_names.add(widget_name)
			refs.setdefault(widget_name, []).append(
				make_ref(rel, line_map.line_number(match.start()), "wgtrAddType")
			)
		
		for name in widget_names:
			widget_types.add(name)
			widget_families[name] = set(widget_names)
	
	return widget_types, widget_families, refs


# Expand docs coverage when a documented widget allows `child type="any"`.
def expand_any_child_coverage(
	docs: dict[str, WidgetDoc],
	doc_types: set[str],
	widget_families: dict[str, set[str]],
) -> set[str]:
	expanded_doc_types = set(doc_types)
	for widget_name, node in docs.items():
		if node.any_child:
			expanded_doc_types.update(widget_families.get(widget_name, set()))
	return expanded_doc_types


# Parse C htmlgen drivers for events, actions, and params.
def parse_c(path: Path) -> dict[str, WidgetImpl]:
	widget_impls: dict[str, WidgetImpl] = {}

	# Parse each C driver and attach signal-level evidence.
	for c_file in sorted(path.glob("htdrv_*.c")):
		# Check if the file name indicates that we should skip it.
		file_name = c_file.name
		eager_widget_name = normalize_widget_name(file_name[6:-2])
		if eager_widget_name == "":
			continue
		rel = "centrallix/htmlgen/%s" % file_name
		
		# Read file content.
		content = c_file.read_text(encoding="utf-8", errors="ignore")
		line_map = LineMap(content)
		
		# Get the widget name.
		parent_widget_impl = None
		for widget_match in c_name_re.finditer(content):
			widget_name = normalize_widget_name(widget_match.group(1))
			if widget_name == "":
				continue
			
			# Store the widget implementation.
			widget_impl = widget_impls.setdefault(widget_name, WidgetImpl(widget_name=widget_name))
			widget_impl.definition_refs.append(
				make_ref(rel, line_map.line_number(widget_match.start()), "strcpy sets widget name")
			)
			
			# There's no way to know which widget is the parent, so
			# assume that the first widget is the parent.
			if not parent_widget_impl:
				parent_widget_impl = widget_impl
		
		# Proceed with the parent widget.
		if not parent_widget_impl:
			continue
		widget_impl = parent_widget_impl
		widget_name = widget_impl.widget_name
		
		# Handle warning.
		if widget_name != eager_widget_name:
			print(f"Warning: File {file_name} used to declare widget parent {widget_name}.")
			print(f"  Should `\"{eager_widget_name}\": \"{widget_name}\",` be added to WIDGETS_ALIASES?")
		
		# Parse events.
		for event_m in c_event_re.finditer(content):
			event_name = normalize_name(event_m.group(1))
			event = widget_impl.event(event_name)
			event.found(Confidence.STRONG,
				make_ref(rel, line_map.line_number(event_m.start()), "htrAddEvent")
			)
		
		# Parse actions.
		for action_m in c_action_re.finditer(content):
			action_name = normalize_name(action_m.group(1))
			action = widget_impl.action(action_name)
			action.found(Confidence.STRONG,
				make_ref(rel, line_map.line_number(action_m.start()), "htrAddAction")
			)
		
		# Parse event/action params.
		for param_m in c_param_re.finditer(content):
			signal_name = normalize_name(param_m.group(1))
			param_name = normalize_name(param_m.group(2))
			signal : SignalImpl | None = widget_impl.events.get(signal_name) or widget_impl.actions.get(signal_name)
			if signal == None or not param_name:
				continue
			signal.update_confidence(Confidence.STRONG)
			signal.add_param(param_name,
				make_ref(rel, line_map.line_number(param_m.start()), "htrAddParam")
			)
	return widget_impls

# Infer JS action parameters by scanning the handler body for param accesses.
def parse_js_action_params(js: str, js_line_map: LineMap, fn_name: str) -> list[tuple[str, int]]:
	params: list[tuple[str, int]] = []
	
	# Search for the start of the JS function.
	js_fn_decl = js_action_impl_re(fn_name).search(js)
	if not js_fn_decl:
		return params
	param1 = js_fn_decl.group(1).strip()
	body_start = js_fn_decl.end()
	
	# Basic check for parameter deconstruction.
	if (param1.startswith("{") and param1.endswith("}")):
		decl_line = js_line_map.line_number(js_fn_decl.start())
		param_strs = param1[1:-1].split(",")
		params = [(param_str.strip(), decl_line) for param_str in param_strs]
		return params
	
	# Count braces to isolate the function body without a full JS parser.
	# Note: Comments are parsed as code to let programmers use them to patch
	# edge cases manually.  For example, see cases the editbox widget's
	# SetValue action, which accesses the Description parameter outside the
	# body of the function that implements that action.
	idx = body_start
	depth = 1
	while idx < len(js) and depth > 0:
		ch = js[idx]
		if ch == "{":
			depth += 1
		elif ch == "}":
			depth -= 1
		idx += 1
	body = js[body_start : max(body_start, idx - 1)]
	
	# Search the function body to locate all properties that are read. 
	for pm in re.finditer(js_property_re(param1), body):
		param_name = pm.group(1)
		param_line = js_line_map.line_number(body_start + pm.start())
		params.append((param_name, param_line))
	return params


# Parse JS drivers for event and action registrations (and heuristic param usage).
def parse_js(path: Path) -> dict[str, WidgetImpl]:
	widget_impls: dict[str, WidgetImpl] = {}
	
	# Parse each driver file and map interface variables to Add() calls.
	for js_file in sorted(path.glob("htdrv_*.js")):
		# Check if the file name indicates that we should skip it.
		file_name = js_file.name
		eager_widget_name = normalize_widget_name(file_name[6:-3])
		if eager_widget_name == "":
			continue
		
		# Read file content.
		js = js_file.read_text(encoding="utf-8", errors="ignore")
		line_map = LineMap(js)
		
		# Get the widget name (from the file name).
		widget_name = normalize_widget_name(js_file.stem.replace("htdrv_", "", 1))
		if widget_name != eager_widget_name:
			print(f"Warning: File {file_name} used to declare widget {widget_name}.")
			print(f"  Should `\"{eager_widget_name}\": \"{widget_name}\",` be added to WIDGETS_ALIASES?") 
		if widget_name == "":
			continue
		
		# Store the widget.
		rel = "centrallix-os/sys/js/%s" % file_name
		widget_impl = widget_impls.setdefault(widget_name, WidgetImpl(widget_name=widget_name))
		widget_impl.definition_refs.append(make_ref(rel, 1, "JS driver file"))
		
		# Search for local variables that store event or action interfaces.
		iface_vars: dict[str, str] = {}
		for var_name, iface in js_add_iface_re.findall(js):
			iface_vars[var_name] = iface
		
		# Search for Add() calls on interfaces.
		for add_m in js_add_call_re.finditer(js):
			var_name = add_m.group(1)
			signal_name = normalize_name(add_m.group(2))
			fn_name = add_m.group(3) or ""
			have_fn_name = (fn_name != "")
			iface = iface_vars.get(var_name)
			if not signal_name:
				continue
			
			# Parse each relevant type of interface.
			confidence = Confidence.CONFIRMED if have_fn_name else Confidence.STRONG
			if iface == "ifEvent":
				event = widget_impl.event(signal_name)
				event.found(confidence,
					make_ref(rel, line_map.line_number(add_m.start()), "ifEvent.Add")
				)
				if not have_fn_name:
					continue
				# TODO: Handle event params here.
			elif iface == "ifAction":
				action = widget_impl.action(signal_name)
				action.found(confidence,
					make_ref(rel, line_map.line_number(add_m.start()), "ifAction.Add")
				)
				if not have_fn_name:
					continue
				
				# Handle action params.
				for param_name, param_line in parse_js_action_params(js, line_map, fn_name):
					# Ignore private params.
					if (param_name.startswith('_')):
						continue
					
					# Add param.
					action.add_param(param_name,
						make_ref(rel, param_line, "action param use in JS")
					)
			else: continue
	return widget_impls

# Merge two widget lists, preserving informaton from each.
def merge_widget_lists(
	widgets1: dict[str, WidgetImpl], widgets2: dict[str, WidgetImpl]
) -> dict[str, WidgetImpl]:
	merged: dict[str, WidgetImpl] = {}
	
	# Iterate over all widgets.
	for widget_name in set(widgets1) | set(widgets2):
		new_widget = WidgetImpl(widget_name=widget_name)
		widget1 = widgets1.get(widget_name)
		widget2 = widgets2.get(widget_name)
		
		# Iterate over all sources in each widget.
		for source in (widget1, widget2):
			if not source:
				continue
			new_widget.definition_refs.extend(source.definition_refs)
			for name, signal in source.events.items():
				existing = new_widget.events.get(name) or EventImpl(name=name)
				existing.merge(signal)
				new_widget.events[name] = existing
			for name, signal in source.actions.items():
				existing = new_widget.actions.get(name) or ActionImpl(name=name)
				existing.merge(signal)
				new_widget.actions[name] = existing
		merged[widget_name] = new_widget
	return merged


# Compute global and per-widget drift, including evidence-rich details.
def compute_report(
	docs: dict[str, WidgetDoc],
	doc_types: set[str],
	widget_types: set[str],
	widget_type_refs: dict[str, list[Ref]],
	widgets: dict[str, WidgetImpl],
) -> Report:
	stats : ReportStats = {
		"documented_widgets": 0,
		"implemented_widgets": 0,
		"missing_widget_docs": 0,
		"stale_widget_docs": 0,
		"widgets_with_errors": 0,
		"widget_errors": 0,
		"ignored_errors": 0,
	}
	
	# Build normalized comparison.
	doc_widgets_and_children = set(doc_types)
	doc_widgets = set(docs)
	implemented_widgets = (set(widget_types) | set(widgets))
	
	# Get missing/stale widgets.
	missing_widget_docs = sorted_list(implemented_widgets - doc_widgets_and_children)
	stale_widget_docs = sorted_list(doc_widgets_and_children - implemented_widgets)
	
	# Get refs for missing/stale widgets.
	missing_widget_doc_refs: dict[str, list[Ref]] = {}
	stale_widget_doc_refs: dict[str, list[Ref]] = {}
	for widget in missing_widget_docs:
		refs: list[Ref] = []
		refs.extend(widget_type_refs.get(widget, []))
		if widget in widgets:
			refs.extend(widgets[widget].definition_refs)
		missing_widget_doc_refs[widget] = unique_refs(refs)
	for widget_name in stale_widget_docs:
		doc = docs.get(widget_name)
		if doc != None and doc.ref != None:
			stale_widget_doc_refs[widget_name] = [doc.ref]
			continue
		
		# Fallback: search for a child of this name.
		for widget_doc in docs.values():
			for child_name, child_ref in widget_doc.child_refs.items():
				if child_name == widget_name:
					stale_widget_doc_refs[widget_name] = [child_ref]
	
	# Build per-widget detailed diffs only for documented widgets present in code.
	per_widget: list[PerWidgetFinding] = []
	all_widgets = sorted_list(doc_widgets & implemented_widgets)
	for widget in all_widgets:
		doc = docs.get(widget, WidgetDoc(name=widget))
		impl = widgets.get(widget, WidgetImpl(widget_name=widget))
		
		# Ref list
		widget_refs: list[Ref] = []
		if doc.ref:
			widget_refs.append(doc.ref)
		widget_refs.extend(unique_refs(impl.definition_refs))
		
		# Event and action sets.
		event_impl_names = set(impl.events)
		action_impl_names = set(impl.actions)
		event_doc_names = set(doc.events)
		action_doc_names = set(doc.actions)
		
		# Compile top-level per-widget findings.
		findings: PerWidgetFinding = {
			"widget": widget,
			"refs": widget_refs,
			"missing_events": [{
				"name": name,
				"confidence": impl.events[name].confidence,
				"refs": unique_refs(impl.events[name].definition_refs),
			} for name in sorted_list(event_impl_names - event_doc_names)],
			"extra_events": [{
				"name": name,
				"confidence": Confidence.CONFIRMED,
				"refs": [doc.event_refs[name]] if name in doc.event_refs else [],
			} for name in sorted_list(event_doc_names - event_impl_names)],
			"missing_actions": [{
				"name": name,
				"confidence": impl.actions[name].confidence,
				"refs": unique_refs(impl.actions[name].definition_refs),
			} for name in sorted_list(action_impl_names - action_doc_names)],
			"extra_actions": [{
				"name": name,
				"confidence": Confidence.CONFIRMED,
				"refs": [doc.action_refs[name]] if name in doc.action_refs else [],
			} for name in sorted_list(action_doc_names - action_impl_names)],
			"incorrect_action_params": [],
		}
		
		# Make a list of the names of actions that already have known issues.
		issue_action_names = {a["name"] for a in (*findings["missing_actions"], *findings["extra_actions"])}
		
		# Compute action parameter mismatch details with runtime/doc references.
		for action_name in sorted_list(action_impl_names | action_doc_names):
			action_impl = impl.actions.get(action_name, ActionImpl(""))
			if action_impl.name == "":
				continue
			
			# Skip param issues for actions that already have known issues.
			if action_impl.name in issue_action_names:
				continue
			
			# Determine missing/extra action_impl parameters.
			impl_param_names = set(action_impl.params)
			doc_param_names = set(doc.action_params.get(action_name, []))
			missing_param_names = sorted_list(impl_param_names - doc_param_names)
			extra_param_names = sorted_list(doc_param_names - impl_param_names)
			
			# Skip if no errors.
			if not missing_param_names and not extra_param_names:
				continue
			
			# Collect relevant sources.
			action_doc_ref = doc.action_refs.get(action_name) or doc.ref
			signal_refs = action_impl.definition_refs + ([action_doc_ref] if action_doc_ref else [])
			
			# Check for missing params.
			missing_param_refs: dict[str, Ref] = {}
			for missing_param_name in missing_param_names:
				refs = action_impl.params_refs.get(missing_param_name, [])
				if refs:
					missing_param_refs[missing_param_name] = refs[0]
			
			# Check for extra params.
			extra_param_refs: dict[str, Ref] = {}
			if action_doc_ref:
				for extra_param_name in extra_param_names:
					extra_param_refs[extra_param_name] = action_doc_ref
			
			# Handle ignored errors.
			if IGNORE_MISSING_ACTION_PARAM_DOCS in IGNORED_ERRORS:
				stats["ignored_errors"] += len(missing_param_refs)
				missing_param_refs.clear()
			if IGNORE_STALE_ACTION_PARAM_DOCS in IGNORED_ERRORS:
				stats["ignored_errors"] += len(extra_param_refs)
				extra_param_refs.clear()
			if not missing_param_refs and not extra_param_refs:
				continue
			
			# Add errors.
			stats["widget_errors"] += len(missing_param_refs)
			stats["widget_errors"] += len(extra_param_refs)
			findings["incorrect_action_params"].append({
				"signal_name": action_name,
				"signal_refs": signal_refs,
				"confidence": Confidence.HEURISTIC,
				"missing_param_refs": missing_param_refs,
				"extra_param_refs": extra_param_refs,
			})
		
		# Handle ignored errors.
		if IGNORE_MISSING_EVENT_DOCS in IGNORED_ERRORS:
			stats["ignored_errors"] += len(findings["missing_events"])
			findings["missing_events"].clear()
		if IGNORE_STALE_EVENT_DOCS in IGNORED_ERRORS:
			stats["ignored_errors"] += len(findings["extra_events"])
			findings["extra_events"].clear()
		if IGNORE_MISSING_ACTION_DOCS in IGNORED_ERRORS:
			stats["ignored_errors"] += len(findings["missing_actions"])
			findings["missing_actions"].clear()
		if IGNORE_STALE_ACTION_DOCS in IGNORED_ERRORS:
			stats["ignored_errors"] += len(findings["extra_actions"])
			findings["extra_actions"].clear()
		
		# Add entry if errors are found.
		errors = (len(findings["missing_events"])
				  + len(findings["extra_events"])
				  + len(findings["missing_actions"])
				  + len(findings["extra_actions"]))
		if (errors > 0 or findings["incorrect_action_params"]):
			stats["widget_errors"] += errors
			per_widget.append(findings)
	
	# Handle ignored errors.
	if IGNORE_MISSING_WIDGET_DOCS in IGNORED_ERRORS:
		stats["ignored_errors"] += len(missing_widget_docs)
		missing_widget_docs.clear()
	if IGNORE_STALE_WIDGET_DOCS in IGNORED_ERRORS:
		stats["ignored_errors"] += len(stale_widget_docs)
		stale_widget_docs.clear()
	
	# Set other stats.
	stats["documented_widgets"]  = len(doc_widgets_and_children)
	stats["implemented_widgets"] = len(implemented_widgets)
	stats["missing_widget_docs"] = len(missing_widget_docs)
	stats["stale_widget_docs"]   = len(stale_widget_docs)
	stats["widgets_with_errors"] = len(per_widget)
	
	# Return report.
	return {
		"stats": stats,
		"missing_widget_docs": [{
			"name": name,
			"confidence": Confidence.CONFIRMED,
			"refs": unique_refs(missing_widget_doc_refs.get(name, [])),
		} for name in missing_widget_docs],
		"stale_widget_docs": [{
			"name": name,
			"confidence": Confidence.CONFIRMED,
			"refs": unique_refs(stale_widget_doc_refs.get(name, [])),
		} for name in stale_widget_docs],
		"per_widget": per_widget,
	}


# Write the machine-readable JSON artifact with stable formatting.
def write_json(path: Path, report: Report) -> None:
	with path.open("w", encoding="utf-8") as handle:
		json.dump(report, handle, indent=2 if HUMAN_JSON else None, sort_keys=True)
		handle.write("\n")


# Render the human-readable markdown report with clickable evidence links.
def write_markdown(path: Path, report: Report, repo_root: Path) -> None:
	report_dir = path.parent
	lines: list[str] = []
	
	# Write header.
	lines.append(
		"# Widget Documentation Report\n"
		"Report generated by [generate.py](generate.py).\n"
	)
	
	# Write stats.
	s = report["stats"]
	doc_widgets_strict = s["documented_widgets"] - s["stale_widget_docs"]
	progress = doc_widgets_strict / (float(s["implemented_widgets"]) or 1)
	error_percent = s["widgets_with_errors"] / (float(doc_widgets_strict) or 1)
	error_rate = s["widget_errors"] / (float(doc_widgets_strict) or 1)
	lines.append(
		"## Widget Stats\n"
		f"- **Documented**: {doc_widgets_strict}/{s['implemented_widgets']} widgets ({progress:.0%})\n"
		f"- **Missing widget docs**: {s['missing_widget_docs']}\n"
		f"- **Stale widget docs**: {s['stale_widget_docs']}\n"
		f"- **Ignored widgets**: {len(IGNORED_WIDGETS)}\n"
		f"- **Widget docs with errors**: {s['widgets_with_errors']} ({error_percent:.0%})\n"
		f"- **Widget doc errors**: {s['widget_errors']} (~{error_rate:.2}/widget)\n"
		f"- **Ignored errors**: {s['ignored_errors']}\n"
	)

	# Write global findings (aka. missing & stale widget docs).
	for title, entries in [
		("Missing Widget Docs", report["missing_widget_docs"]),
		("Stale Widgets Docs",  report["stale_widget_docs"]),
	]:
		if len(entries) == 0:
			continue
		if title:
			lines.append(f"## {title}")
		for entry in entries:
			refs = entry.get("refs", [])
			lines.append(f"- `{entry['name']}` (origins: `{get_origins(refs)}`)")
			lines.extend(f"  - {ref_to_markdown_link(report_dir, repo_root, r)}" for r in refs)
			if not refs:
				lines.append("  - source: unknown")
		lines.append("")
	
	# Write per-widget differences by type.
	if len(report["per_widget"]) > 0: lines.append("## Widget Errors")
	for item in report["per_widget"]:
		refs = item.get("refs", [])
		lines.append(f"### `{item['widget']}`")
		
		# Write sources.
		lines.append(f"- **Sources** (origin: `{get_origins(refs)}`)")
		lines.extend(f"  - {ref_to_markdown_link(report_dir, repo_root, r)}" for r in refs)
		
		# Write event issues.
		if item["missing_events"]:
			lines.append("- **Undocumented events**")
			for event in item["missing_events"]:
				lines.append(f"  - `{event['name']}` ("
					f"origin: `{get_origins(event['refs'])}`, "
					f"confidence: `{event['confidence']}`"
				")")
				for ref in event.get("refs", []):
					lines.append("    - %s" % ref_to_markdown_link(report_dir, repo_root, ref))
		if item["extra_events"]:
			lines.append("- **Stale event docs**")
			for event in item["extra_events"]:
				lines.append("  - `%s`" % event["name"])
				for ref in event.get("refs", []):
					lines.append("    - %s" % ref_to_markdown_link(report_dir, repo_root, ref))
		
		# Write action issues.
		if item["missing_actions"]:
			lines.append("- **Undocumented actions**")
			for action in item["missing_actions"]:
				lines.append(f"  - `{action['name']}` ("
					f"origin: `{get_origins(action['refs'])}`, "
					f"confidence: `{action['confidence']}`"
				")")
				for ref in action.get("refs", []):
					lines.append("    - %s" % ref_to_markdown_link(report_dir, repo_root, ref))
		if item["extra_actions"]:
			lines.append("- **Stale action docs**")
			for action in item["extra_actions"]:
				lines.append(f"  - `{action['name']}`")
				for ref in action.get("refs", []):
					lines.append(f"    - {ref_to_markdown_link(report_dir, repo_root, ref)}")
		
		# Write action param issues.
		if item["incorrect_action_params"]:
			lines.append("- **Incorrect action parameters**")
			for incorrect_action_params in item["incorrect_action_params"]:
				lines.append(f"  - `{incorrect_action_params['signal_name']}`")
				
				# Write signal sources.
				signal_refs = incorrect_action_params["signal_refs"]
				lines.append(f"    - **Sources** (origin: `{get_origins(signal_refs)}`)")
				for signal_ref in signal_refs:
					lines.append(f"      - {ref_to_markdown_link(report_dir, repo_root, signal_ref)}")
				
				# Write signal param issues.
				for title, refs in [
					("Undocumented params", incorrect_action_params["missing_param_refs"]),
					("Stale param docs",  incorrect_action_params["extra_param_refs"]),
				]:
					if not refs:
						continue
					lines.append(f"    - **{title}**")
					for name, ref in refs.items():
						lines.append(f"      - `{name}`: {ref_to_markdown_link(report_dir, repo_root, ref)}")
		lines.append("")
	
	# Write report data.
	with path.open("w", encoding="utf-8") as handle:
		handle.write("\n".join(lines).rstrip() + "\n")


# Resolve standard input/output paths from repo-root context.
def resolve_paths(
	repo_root: Path, out_dir: Optional[Path]
) -> tuple[Path, Path, Path, Path, Path]:
	widgets_root = repo_root / "centrallix-doc" / "Widgets"
	doc_xml = widgets_root / "widgets.xml"
	wgtr_dir = repo_root / "centrallix" / "wgtr"
	c_driver_dir = repo_root / "centrallix" / "htmlgen"
	js_driver_dir = repo_root / "centrallix-os" / "sys" / "js"
	output_dir = out_dir or widgets_root / "doc-report"
	return doc_xml, wgtr_dir, c_driver_dir, js_driver_dir, output_dir


# Parse CLI flags with repo-root auto-detection.
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


# Orchestrate parse -> normalize -> diff -> write pipeline.
def main() -> int:
	# Resolve refs and output targets.
	args = parse_args()
	repo_root = args.repo_root.resolve()
	doc_xml, wgtr_dir, c_driver_dir, js_driver_dir, output_dir = resolve_paths(
		repo_root, args.out_dir
	)
	json_path = output_dir / "report.json"
	md_path = output_dir / "report.md"
	output_dir.mkdir(parents=True, exist_ok=True)
	
	# Build documented + implemented widget lists.
	docs, doc_types = parse_docs(doc_xml)
	widget_types, widget_families, widget_type_refs = parse_widgets_in_wgtr(wgtr_dir)
	doc_types = expand_any_child_coverage(docs, doc_types, widget_families)
	c_widgets = parse_c(c_driver_dir)
	js_widgets = parse_js(js_driver_dir)
	widgets = merge_widget_lists(c_widgets, js_widgets)
	
	# Compute and write doc reports.
	report = compute_report(
		docs,
		doc_types,
		widget_types,
		widget_type_refs,
		widgets,
	)
	if (WRITE_REPORT_JSON):
		write_json(json_path, report)
		print(f"Generated: {json_path}")
	if (WRITE_REPORT_MD):
		write_markdown(md_path, report, repo_root)
		print(f"Generated: {md_path}")
	
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
