# Table widget tests with a result set much larger than the osrc replica.
#
# Each numbered test targets one suspected table/osrc defect (see
# centrallix-sysdoc/UITestCoverage.md, "Table Large Result Set Test").
# Tests 2-16 are expected to fail until those defects are fixed.
#
# Optional config.toml keys, beyond "url":
#   chrome_binary  - Chrome/Chromium binary to drive.
#   chromedriver   - chromedriver path (skips ChromeDriverManager).
#   headless       - true to run without a visible browser window.
#   destructive    - true to run tests that change data (7, 12, 16, 18).  Each
#                    one undoes its change through the UI.  If one stops
#                    partway, stop the server, run
#                    centrallix-os/tests/ui/table_large/gen_table_large_csv.py,
#                    and restart it (the server caches the CSV).
#   stress_seeds   - list of seeds for the random stress test (17).
#   stress_steps   - steps per stress seed (default 150).
#   stress_exclude - stress actions to leave out, e.g. ["refresh"].
#   only           - list of test numbers to run (default: all).

import json
import random
import sys
import time

import toml
from selenium import webdriver
from selenium.common.exceptions import TimeoutException
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.actions.wheel_input import ScrollOrigin
from selenium.webdriver.support.ui import WebDriverWait
from test_reporter import TestReporter

reporter = TestReporter("TableLarge")

APP = "/tests/ui/table_large/table_large_test.app"
DETAIL_APP = "/tests/ui/table_large/table_large_detail_test.app"
EMPTY_APP = "/tests/ui/table_large/table_large_empty_test.app"
TOTAL_ROWS = 1000

# Helpers injected into the page.  Nothing here changes table or osrc
# behavior except the confirm() override, which auto-accepts and records
# dialogs such as the osrc "already retrieved N records" prompt.
HELPERS_JS = r"""
window.__tl = {
    log: [],
    confirms: [],
    alerts: [],

    tbl: function() { return wgtrFind('table'); },

    relY: function(el) { return parseFloat(el.style.top) || 0; },

    // Fresh visibility scan using the same rule as tbld_is_row_visible.
    scanVis: function(t) {
	var fv = null, lv = null, any = [];
	var cr = t.scrollctr.getBoundingClientRect();
	for (var i = t.rows.first; t.rows.first !== null && i <= t.rows.last; i++) {
	    var r = t.rows[i];
	    if (!r) continue;
	    var ry = __tl.relY(r), rh = $(r).height(), sy = t.scroll_y;
	    var full = !(ry + sy + rh < 0 || ry + sy > t.vis_height) && !(ry + sy < 0 || ry + sy + rh > t.vis_height);
	    if (full) { if (fv === null) fv = i; lv = i; }
	    var rr = r.getBoundingClientRect();
	    if ($(r).css('visibility') != 'hidden' && rr.bottom > cr.top + 1 && rr.top < cr.bottom - 1 && rr.height > 0)
		any.push(i);
	}
	return {firstvis: fv, lastvis: lv, intersecting: any};
    },

    colIndex: function(t, field) {
	for (var j = 0; j < t.cols.length; j++)
	    if (t.cols[j] && t.cols[j].fieldname == field) return j;
	return -1;
    },

    replicaValue: function(o, i, field) {
	var rec = o.replica[i];
	if (!rec) return undefined;
	for (var k in rec)
	    if (rec[k] && rec[k].oid == field) return rec[k].value;
	return null;
    },

    settled: function(t) {
	var o = t.osrc;
	return !t.osrc_busy && !o.pending && !(o.masters_pending && o.masters_pending.length) &&
	    !$(t.scrolldiv).queue().length && !(window.pg_loadqueue && pg_loadqueue.length) &&
	    t.target_y == null;
    },

    state: function(t) {
	var o = t.osrc;
	return {first: t.rows.first, last: t.rows.last, firstvis: t.rows.firstvis, lastvis: t.rows.lastvis,
	    lastosrc: t.rows.lastosrc, scroll_y: t.scroll_y, scrolldiv_top: __tl.relY(t.scrolldiv),
	    target_y: t.target_y, bring_into_view: t.bring_into_view, old_scroll_y: t.old_scroll_y,
	    busy: t.osrc_busy, queue: t.osrc_request_queue.length, cr: t.cr,
	    scroll_minrec: t.scroll_minrec, scroll_maxrec: t.scroll_maxrec,
	    scroll_minheight: t.scroll_minheight, scroll_maxheight: t.scroll_maxheight,
	    thumb_height: t.thumb_height, thumb_avail: t.thumb_avail, thumb_pos: t.thumb_pos,
	    oFirst: o.FirstRecord, oLast: o.LastRecord, oCur: o.CurrentRecord, oFinal: o.FinalRecord,
	    oPending: o.pending};
    },

    // Returns a list of invariant violations (empty when consistent).
    invariants: function(t, opts) {
	opts = opts || {};
	var v = [], o = t.osrc;
	var scan = __tl.scanVis(t);
	var have_data = o.LastRecord >= 1 && o.LastRecord >= o.FirstRecord;

	// I1: something is drawn in the viewport.
	if (have_data && scan.intersecting.length == 0 && !__tl.newrowInView(t))
	    v.push('I1 blank: no row intersects the viewport (rows ' + t.rows.first + '..' + t.rows.last + ', scroll_y ' + t.scroll_y + ')');

	// I2: rows form one contiguous, non-overlapping run.
	for (var i = t.rows.first; t.rows.first !== null && i <= t.rows.last; i++) {
	    var r = t.rows[i];
	    if (!r) { v.push('I2 gap: rows[' + i + '] missing inside ' + t.rows.first + '..' + t.rows.last); continue; }
	    if (!r.positioned) v.push('I2 row ' + i + ' not positioned');
	    var n = t.rows[i+1];
	    if (n && i < t.rows.last) {
		var expect = __tl.relY(r) + $(r).height() + t.cellvspacing*2;
		if (t.newrow && t.newrow.placed && i+1 == t.newrow_slot)
		    expect += $(t.newrow).height() + t.cellvspacing*2;
		if (Math.abs(__tl.relY(n) - expect) > 1)
		    v.push('I2 row ' + (i+1) + ' at y=' + __tl.relY(n) + ', expected ' + expect + ' (after row ' + i + ')');
	    }
	}

	// I3: visibility bookkeeping and scroll offset agree with the DOM.
	if (t.rows.firstvis !== scan.firstvis || t.rows.lastvis !== scan.lastvis)
	    v.push('I3 firstvis/lastvis ' + t.rows.firstvis + '/' + t.rows.lastvis + ' but scan gives ' + scan.firstvis + '/' + scan.lastvis);
	if (Math.abs(__tl.relY(t.scrolldiv) - t.scroll_y) > 1)
	    v.push('I3 scroll_y ' + t.scroll_y + ' but scrolldiv top ' + __tl.relY(t.scrolldiv));

	// I4: every drawn row shows its own replica record.
	var nc = __tl.colIndex(t, 'Name');
	for (var i = t.rows.first; t.rows.first !== null && i <= t.rows.last; i++) {
	    var r = t.rows[i];
	    if (!r || r.disp_mode == 'newselect') continue;
	    var rv = __tl.replicaValue(o, i, 'Name');
	    if (rv === undefined)
		v.push('I4 row ' + i + ' drawn without replica data (shows "' + r.cols[nc].data + '")');
	    else if (String(rv) != String(r.cols[nc].data))
		v.push('I4 row ' + i + ' shows "' + r.cols[nc].data + '" but replica has "' + rv + '"');
	}
	if (opts.check_order) {
	    // The test data has Id == result set position, so row i must show Id i.
	    var ic = __tl.colIndex(t, 'Id'), bad = [];
	    for (var i = t.rows.first; t.rows.first !== null && i <= t.rows.last; i++) {
		var a = t.rows[i];
		if (a && a.disp_mode != 'newselect' && a.cols[ic].data !== '' && parseInt(a.cols[ic].data) != i)
		    bad.push(i + ':' + a.cols[ic].data);
	    }
	    if (bad.length)
		v.push('I4 order: ' + bad.length + ' rows show the wrong record (row:Id ' + bad.slice(0, 5).join(', ') + ')');
	}

	// I5: scrollbar thumb present and roughly where the visible records are.
	var nvis = (scan.firstvis !== null) ? (scan.lastvis - scan.firstvis + 1) : 0;
	var known_more = (o.FinalRecord !== null) ? (o.FinalRecord > nvis) : (o.LastRecord > nvis);
	if (have_data && nvis > 0 && known_more && !(t.thumb_height < t.thumb_avail - 1))
	    v.push('I5 thumb fills the track (' + t.thumb_height + '/' + t.thumb_avail + ') though more rows exist than fit');
	if (o.FinalRecord !== null && o.FinalRecord > nvis && scan.firstvis !== null && t.thumb_avail > t.thumb_height) {
	    var expect_frac = (scan.firstvis - 1) / (o.FinalRecord - nvis);
	    if (expect_frac > 1) expect_frac = 1;
	    var frac = (__tl.relY(t.scrollbar.b) - 18) / (t.thumb_avail - t.thumb_height);
	    if (Math.abs(frac - expect_frac) > (opts.thumb_tolerance || 0.1))
		v.push('I5 thumb at ' + frac.toFixed(3) + ' of track, expected ~' + expect_frac.toFixed(3) + ' (firstvis ' + scan.firstvis + ' of ' + o.FinalRecord + ')');
	}

	// I6: no leftover pending scroll work.
	if (t.target_y != null) v.push('I6 target_y left set (' + t.target_y + ')');
	if (t.bring_into_view) v.push('I6 bring_into_view left set (' + t.bring_into_view + ')');
	if (t.osrc_request_queue.length) v.push('I6 ' + t.osrc_request_queue.length + ' queued osrc requests');
	return v;
    },

    viewportRows: function(t) {
	var res = [], nc = __tl.colIndex(t, 'Name');
	var scan = __tl.scanVis(t);
	for (var k = 0; k < scan.intersecting.length; k++) {
	    var i = scan.intersecting[k], r = t.rows[i];
	    res.push({rownum: i, page_y: r.getBoundingClientRect().top, rel_y: __tl.relY(r), name: r.cols[nc].data, mode: r.disp_mode});
	}
	if (__tl.newrowInView(t)) {
	    var nr = t.newrow;
	    res.push({rownum: null, slot: t.newrow_slot, page_y: nr.getBoundingClientRect().top, rel_y: __tl.relY(nr), name: '', mode: 'newselect', height: $(nr).height()});
	    res.sort(function(a, b) { return a.page_y - b.page_y; });
	}
	return res;
    },

    newrowInView: function(t) {
	var nr = t.newrow;
	if (!nr || !nr.placed || $(nr).css('visibility') == 'hidden') return false;
	var cr = t.scrollctr.getBoundingClientRect(), rr = nr.getBoundingClientRect();
	return rr.top >= cr.top - 1 && rr.bottom <= cr.bottom + 1;
    },

    replicaHoles: function(o) {
	var holes = [];
	for (var i = o.FirstRecord; i <= o.LastRecord; i++)
	    if (!o.replica[i]) holes.push(i);
	return holes;
    },

    record: function(t, fn, args) {
	var e = {fn: fn, args: args};
	try { var s = __tl.state(t); for (var k in s) e[k] = s[k]; } catch (x) { }
	__tl.log.push(e);
	if (__tl.log.length > 300) __tl.log.shift();
    },

    wrap: function(obj, name, t) {
	var orig = obj[name];
	if (typeof orig != 'function' || orig.__tl_wrapped) return;
	var w = function() {
	    var a = [];
	    for (var i = 0; i < arguments.length; i++) {
		var x = arguments[i];
		if (x && typeof x == 'object') x = (x.length !== undefined) ? ('[' + x.length + ']') : (x.rownum !== undefined ? ('row' + x.rownum) : '{obj}');
		a.push(x);
	    }
	    __tl.record(t, name, a);
	    return orig.apply(this, arguments);
	};
	w.__tl_wrapped = true;
	obj[name] = w;
    },

    traceOn: function(t) {
	var tf = ['RedrawAll', 'Scroll', 'BringIntoView', 'PositionRows', 'OsrcRequest', 'ObjectAvailable',
	    'ReplicaMoved', 'ObjectCreated', 'ObjectDeleted', 'ObjectModified', 'DataAvailable', 'OperationComplete'];
	for (var i = 0; i < tf.length; i++) __tl.wrap(t, tf[i], t);
	var of = ['ScrollTo', 'MoveToRecord'];
	for (var i = 0; i < of.length; i++) __tl.wrap(t.osrc, of[i], t);
	__tl.errors = [];
	window.addEventListener('error', function(e) { __tl.errors.push(String(e.message) + ' @' + (e.filename || '').split('/').pop() + ':' + e.lineno); });
	window.confirm = function(m) { __tl.confirms.push(m); return true; };
	window.alert = function(m) { __tl.alerts.push(m); };
    }
};
"""


class Session:
    """One page load of a test app, with helpers to drive and inspect it."""

    def __init__(self, driver, base_url, app, empty=False):
        self.d = driver
        self.d.get(base_url + app)
        loaded = "wgtrFind('table').osrc.FinalRecord !== null" if empty else "wgtrFind('table').rows.last > 0"
        WebDriverWait(self.d, 30).until(lambda d: d.execute_script(
            "return document.readyState == 'complete' && !!window.pg_isloaded && !!wgtrFind('table') && " + loaded))
        self.d.execute_script(HELPERS_JS)
        self.d.execute_script("__tl.traceOn(__tl.tbl());")
        self.table = self.d.execute_script("return __tl.tbl();")
        self.settle()

    def js(self, expr, *args):
        return self.d.execute_script("var t = __tl.tbl(); return " + expr + ";", *args)

    def state(self):
        return self.js("__tl.state(t)")

    def settle(self, timeout=15.0):
        """Wait until the table and osrc are idle; False means it got stuck."""
        time.sleep(0.15)
        end = time.time() + timeout
        ok_count = 0
        while time.time() < end:
            if self.js("__tl.settled(t)"):
                ok_count += 1
                if ok_count >= 2:
                    return True
            else:
                ok_count = 0
            time.sleep(0.1)
        return False

    def invariants(self, **opts):
        return self.js("__tl.invariants(t, arguments[0])", opts)

    def viewport(self):
        return self.js("__tl.viewportRows(t)")

    def trace(self, n=40):
        return self.js("__tl.log.slice(-arguments[0])", n)

    def wheel(self, dy):
        ActionChains(self.d).move_to_element(self.table).perform()
        ActionChains(self.d).scroll_from_origin(ScrollOrigin.from_element(self.table), 0, int(dy)).perform()

    def scroll_to_record(self, n, step=800, max_steps=80):
        """Wheel down until the first visible record is at least n."""
        for _ in range(max_steps):
            fv = self.js("t.rows.firstvis")
            if fv is not None and fv >= n:
                return True
            self.wheel(step)
            if not self.settle():
                return False
        return False

    def click_button(self, name):
        ActionChains(self.d).move_to_element(self.d.execute_script("return wgtrFind(arguments[0]);", name)).click().perform()

    def click_row(self, rownum):
        row = self.js("t.rows[arguments[0]]", rownum)
        ActionChains(self.d).move_to_element(row).click().perform()

    def invoke(self, widget, action, params=None):
        self.d.execute_script("wgtrFind(arguments[0]).ifcProbe(ifAction).Invoke(arguments[1], arguments[2] || {});", widget, action, params)

    def middle_visible_row(self):
        vp = [r for r in self.viewport() if r["mode"] != "newselect"]
        return vp[len(vp) // 2]["rownum"] if vp else None


class Suite:
    def __init__(self, config):
        self.config = config
        self.base_url = config["url"]
        self.driver = None

    def make_driver(self):
        opts = webdriver.ChromeOptions()
        opts.add_argument("--lang=en")
        opts.add_argument("--ignore-certificate-errors")
        opts.add_argument("--window-size=1200,900")
        if self.config.get("headless"):
            opts.add_argument("--headless=new")
            opts.add_argument("--no-sandbox")
        if self.config.get("chrome_binary"):
            opts.binary_location = self.config["chrome_binary"]
        if self.config.get("chromedriver"):
            service = Service(self.config["chromedriver"])
        else:
            from webdriver_manager.chrome import ChromeDriverManager
            service = Service(ChromeDriverManager().install())
        return webdriver.Chrome(service=service, options=opts)

    def session(self, app=APP, empty=False):
        if self.driver is None:
            self.driver = self.make_driver()
        self.last_session = Session(self.driver, self.base_url, app, empty)
        return self.last_session

    def destructive_ok(self, test_no):
        if self.config.get("destructive"):
            return True
        print("  [test %d] SKIP: set destructive = true in config.toml to run this test" % test_no)
        reporter.record_check(test_no, "skipped (destructive tests disabled)", True)
        return False

    def type_into(self, s, eb, text, clear=False):
        el = s.d.execute_script("return wgtrFind(arguments[0]);", eb)
        chain = ActionChains(s.d).click(el)
        if clear:
            chain = chain.key_down(Keys.CONTROL).send_keys("a").key_up(Keys.CONTROL).send_keys(Keys.BACKSPACE)
        chain.send_keys(text).perform()
        time.sleep(0.2)

    def data_is_clean(self, s):
        """True if the first loaded rows are the generated, unmodified data."""
        return not s.invariants(check_order=True) and s.js("__tl.replicaValue(t.osrc, 5, 'Amount')") == "185"

    # ---- reporting helpers ----

    def check(self, s, test_no, label, ok, detail=None):
        reporter.record_check(test_no, label, bool(ok))
        if not ok:
            print("  [test %d] FAIL: %s" % (test_no, label))
            if detail:
                print("    detail: " + (detail if isinstance(detail, str) else json.dumps(detail, default=str)))
            if s is not None:
                try:
                    print("    state: " + json.dumps(s.state(), default=str))
                    print("    trace (most recent last):")
                    for e in s.trace(25):
                        print("      " + json.dumps(e, default=str))
                except Exception as e:
                    print("    (could not read state: %s)" % e)

    def check_invariants(self, s, test_no, label, **opts):
        v = s.invariants(**opts)
        self.check(s, test_no, label, not v, "; ".join(v))
        return not v

    def check_js_errors(self, s, test_no):
        errs = s.js("__tl.errors.splice(0)")
        self.check(s if errs else None, test_no, "no JavaScript errors", not errs, "; ".join(errs[:5]))

    def check_settled(self, s, test_no, label, timeout=15.0):
        ok = s.settle(timeout)
        self.check(s, test_no, label + " (table/osrc settles)", ok, "still busy after %.0fs" % timeout)
        return ok

    # ---- tests ----

    def test1(self):
        reporter.add_test(1, "Baseline scroll within the first replica window")
        s = self.session()
        self.check_invariants(s, 1, "invariants after load", check_order=True)
        for i in range(3):
            s.wheel(400)
            self.check_settled(s, 1, "wheel down %d" % (i + 1))
        self.check_invariants(s, 1, "invariants after wheel down", check_order=True)
        fv = s.js("t.rows.firstvis")
        self.check(s, 1, "wheel moved the table down", fv is not None and fv > 30, "firstvis=%s" % fv)
        for i in range(4):
            s.wheel(-400)
            self.check_settled(s, 1, "wheel up %d" % (i + 1))
        self.check_invariants(s, 1, "invariants after wheel up", check_order=True)
        self.check(s, 1, "back at the top", s.js("t.rows.firstvis") == 1)

    def test2(self):
        reporter.add_test(2, "Far scroll, then osrc First (jump back to record 1)")
        s = self.session()
        self.check(s, 2, "wheel to record ~400", s.scroll_to_record(400))
        self.check_invariants(s, 2, "invariants at record ~400", check_order=True)
        s.click_button("first_btn")
        self.check_settled(s, 2, "after First")
        self.check_invariants(s, 2, "invariants after First", check_order=True)
        fv = s.js("t.rows.firstvis")
        self.check(s, 2, "record 1 area visible after First", fv is not None and fv <= 5, "firstvis=%s" % fv)

    def test3(self):
        reporter.add_test(3, "From the top, osrc Last (jump to record 1000)")
        s = self.session()
        s.click_button("last_btn")
        self.check_settled(s, 3, "after Last", timeout=30)
        self.check_invariants(s, 3, "invariants after Last", check_order=True)
        lv = s.js("__tl.scanVis(t).lastvis")
        self.check(s, 3, "record %d visible after Last" % TOTAL_ROWS, lv == TOTAL_ROWS, "scan lastvis=%s" % lv)

    def test4(self):
        reporter.add_test(4, "Far scroll, Last, then wheel up")
        s = self.session()
        self.check(s, 4, "wheel to record ~400", s.scroll_to_record(400))
        s.click_button("last_btn")
        self.check_settled(s, 4, "after Last", timeout=30)
        self.check_invariants(s, 4, "invariants after Last", check_order=True)
        before = s.js("__tl.scanVis(t).firstvis")
        for i in range(3):
            s.wheel(-100)
            s.settle()
        self.check_settled(s, 4, "after wheel up")
        self.check_invariants(s, 4, "invariants after wheel up", check_order=True)
        after = s.js("__tl.scanVis(t).firstvis")
        self.check(s, 4, "wheel up moves up a little (not to the start)",
                   before is not None and after is not None and after < before and after > 900,
                   "firstvis before=%s after=%s" % (before, after))

    def test5(self):
        reporter.add_test(5, "Jump from the top to a middle record (MoveToRecord 500)")
        s = self.session()
        s.js("(setTimeout(function(){ t.osrc.MoveToRecord(500, t); }, 0), 0)")
        self.check_settled(s, 5, "after MoveToRecord(500)", timeout=30)
        self.check_invariants(s, 5, "invariants after jump to 500", check_order=True)
        scan = s.js("__tl.scanVis(t)")
        self.check(s, 5, "record 500 visible", scan["firstvis"] is not None and scan["firstvis"] <= 500 <= scan["lastvis"],
                   "scan firstvis/lastvis=%s/%s" % (scan["firstvis"], scan["lastvis"]))
        s.click_button("last_btn")
        s.settle(30)
        s.js("(setTimeout(function(){ t.osrc.MoveToRecord(500, t); }, 0), 0)")
        self.check_settled(s, 5, "after Last then MoveToRecord(500)", timeout=30)
        self.check_invariants(s, 5, "invariants with known size (thumb position)", check_order=True)

    def test6(self):
        reporter.add_test(6, "Drag the scrollbar thumb to 80%")
        s = self.session()
        s.click_button("last_btn")
        s.settle(30)
        s.click_button("first_btn")
        s.settle(30)
        self.check_invariants(s, 6, "invariants after Last+First (size known)")
        avail, height, top = s.js("[t.thumb_avail, t.thumb_height, __tl.relY(t.scrollbar.b)]")
        target = 18 + 0.8 * (avail - height)
        box = s.js("t.scrollbar.b")
        ActionChains(s.d).click_and_hold(box).move_by_offset(0, int(target - top)).release().perform()
        self.check_settled(s, 6, "after thumb drag", timeout=60)
        self.check_invariants(s, 6, "invariants after thumb drag", check_order=True)
        fv = s.js("__tl.scanVis(t).firstvis")
        self.check(s, 6, "drag to 80% shows records near 800", fv is not None and abs(fv - 800) <= 100, "firstvis=%s" % fv)

    def test7(self):
        reporter.add_test(7, "Delete the current record while it is out of the DOM")
        if not self.destructive_ok(7):
            return
        s = self.session()
        deleted = False
        try:
            s.click_button("last_btn")
            self.check_settled(s, 7, "after Last", timeout=30)
            for i in range(10):
                s.wheel(-800)
                s.settle()
            in_dom = s.js("!!t.rows[%d]" % TOTAL_ROWS)
            self.check(s, 7, "current record left the DOM", not in_dom)
            s.click_button("delete_btn")
            deleted = True
            self.check_settled(s, 7, "after Delete", timeout=30)
            self.check_invariants(s, 7, "invariants after Delete")
            y0 = s.js("t.scroll_y")
            s.wheel(-200)
            s.settle()
            y1 = s.js("t.scroll_y")
            self.check(s, 7, "table still scrolls after Delete", y1 != y0, "scroll_y %s -> %s" % (y0, y1))
        finally:
            if deleted:
                self.recreate_record(TOTAL_ROWS)

    def recreate_record(self, n):
        """Re-create record n with its generated values after a test deletes it."""
        groups = ["Alpha", "Bravo", "Charlie", "Delta", "Echo"]
        s = self.session()
        s.click_button("new_btn")
        s.settle()
        values = (("id_eb", str(n)), ("name_eb", "Row %04d" % n), ("grp_eb", groups[n % 5]),
                  ("amount_eb", str((n * 37) % 1000)), ("notes_eb", "Note %d" % n))
        for eb, text in values:
            self.type_into(s, eb, text)
        s.click_button("save_btn")
        s.settle(30)

    def test8(self):
        reporter.add_test(8, "Replica hole left by the Last (tail) fetch")
        s = self.session()
        s.click_button("last_btn")
        self.check_settled(s, 8, "after Last", timeout=30)
        holes = s.js("__tl.replicaHoles(t.osrc)")
        print("  [test 8] replica holes after Last: %d (%s)" % (len(holes), (str(holes[0]) + ".." + str(holes[-1])) if holes else "none"))
        for i in range(2):
            s.wheel(-1600)
            s.settle()
        self.check_invariants(s, 8, "invariants after wheeling up into the hole", check_order=True)
        s.click_button("first_btn")
        self.check_settled(s, 8, "after First", timeout=30)
        self.check_invariants(s, 8, "invariants after First", check_order=True)
        name = s.js("__tl.replicaValue(t.osrc, 1, 'Name')")
        self.check(s, 8, "replica record 1 is Row 0001", name == "Row 0001", "replica[1] Name=%s" % name)
        s2 = self.session()
        s2.click_button("last_btn")
        s2.settle(30)
        s2.js("(setTimeout(function(){ t.osrc.MoveToRecord(500, t); }, 0), 0)")
        self.check_settled(s2, 8, "MoveToRecord into the hole", timeout=30)
        self.check_js_errors(s2, 8)
        self.check_invariants(s2, 8, "invariants after MoveToRecord into the hole", check_order=True)

    def test9(self):
        reporter.add_test(9, "Scrolling during a slow Refresh")
        s = self.session()
        s.click_row(5)
        s.settle()
        s.d.execute_cdp_cmd("Network.enable", {})
        s.d.execute_cdp_cmd("Network.emulateNetworkConditions", {"offline": False, "latency": 800, "downloadThroughput": -1, "uploadThroughput": -1})
        try:
            s.click_button("refresh_btn")
            for i in range(5):
                s.wheel(400)
                time.sleep(0.1)
            self.check_settled(s, 9, "after Refresh + wheel", timeout=60)
        finally:
            s.d.execute_cdp_cmd("Network.emulateNetworkConditions", {"offline": False, "latency": 0, "downloadThroughput": -1, "uploadThroughput": -1})
        s.settle(30)
        self.check_invariants(s, 9, "invariants after Refresh + wheel", check_order=True)
        sel, cur, form_name, rep_name = s.js("[t.selected, t.osrc.CurrentRecord, wgtrFind('name_eb').getvalue ? wgtrFind('name_eb').getvalue() : null, __tl.replicaValue(t.osrc, t.osrc.CurrentRecord, 'Name')]")
        self.check(s, 9, "form shows the osrc current record", form_name == rep_name, "form=%s replica=%s" % (form_name, rep_name))
        self.check(s, 9, "table highlight matches the current record", sel in (None, cur), "selected=%s current=%s" % (sel, cur))

    def test10(self):
        reporter.add_test(10, "Recovery from a failed fetch")
        s = self.session()
        s.d.execute_cdp_cmd("Network.enable", {})
        s.d.execute_cdp_cmd("Network.setBlockedURLs", {"urls": ["*ls__req=queryfetch*", "*ls__req=multiquery*"]})
        try:
            for i in range(4):
                s.wheel(800)
                s.settle(3)
        finally:
            s.d.execute_cdp_cmd("Network.setBlockedURLs", {"urls": []})
        fv0 = s.js("__tl.scanVis(t).firstvis")
        final = s.js("t.osrc.FinalRecord")
        self.check(s, 10, "failed fetch not taken as the end of the results", final is None or final == TOTAL_ROWS, "FinalRecord=%s" % final)
        for i in range(4):
            s.wheel(800)
            s.settle()
        self.check_settled(s, 10, "after network restored")
        fv1 = s.js("__tl.scanVis(t).firstvis")
        self.check(s, 10, "scrolling resumes after the failure", fv0 is not None and fv1 is not None and fv1 > fv0 + 20, "firstvis %s -> %s" % (fv0, fv1))
        self.check_invariants(s, 10, "invariants after recovery", check_order=True)

    def test11(self):
        reporter.add_test(11, "Row click after a Refresh deep in the results")
        s = self.session()
        self.check(s, 11, "wheel to record ~300", s.scroll_to_record(300))
        s.click_row(s.middle_visible_row())
        s.settle()
        s.click_button("refresh_btn")
        self.check_settled(s, 11, "after Refresh", timeout=30)
        self.check_invariants(s, 11, "invariants after Refresh", check_order=True)
        print("  [test 11] old_scroll_y after Refresh settled: %s" % s.js("t.old_scroll_y"))
        vp = [r for r in s.viewport() if r["mode"] != "newselect"]
        self.check(s, 11, "rows visible after Refresh", len(vp) > 4)
        if len(vp) <= 4:
            return
        b = vp[2]["rownum"]
        y0 = vp[2]["page_y"]
        s.click_row(b)
        s.settle()
        y1 = s.js("t.rows[arguments[0]] ? t.rows[arguments[0]].getBoundingClientRect().top : null", b)
        rh = s.js("t.min_rowheight")
        self.check(s, 11, "clicked row stays in place", y1 is not None and abs(y1 - y0) <= rh, "row %s page_y %s -> %s" % (b, y0, y1))

    def test12(self):
        reporter.add_test(12, "No snap-back to the current record when the form saves it")
        if not self.destructive_ok(12):
            return
        s = self.session()
        try:
            s.click_row(5)
            s.settle()
            s.wheel(600)
            s.settle()
            before = [(r["rownum"], round(r["page_y"])) for r in s.viewport()]
            self.check(s, 12, "current record scrolled out of view", 5 not in [r[0] for r in before])
            self.type_into(s, "amount_eb", "186", clear=True)
            s.click_button("save_btn")
            self.check_settled(s, 12, "after Save")
            after = [(r["rownum"], round(r["page_y"])) for r in s.viewport()]
            self.check(s, 12, "view unchanged after Save", before == after, "before %s.. after %s.." % (before[:3], after[:3]))
        finally:
            # Restore record 5's original Amount.
            s.settle()
            if s.js("t.osrc.CurrentRecord") == 5:
                self.type_into(s, "amount_eb", "185", clear=True)
                s.click_button("save_btn")
                s.settle(30)

    def test13(self):
        reporter.add_test(13, "Selecting a row while a row with detail is above the view")
        s = self.session(DETAIL_APP)
        s.click_row(4)
        s.settle()
        self.check(s, 13, "row detail opened", s.js("t.rows[4].detail.length") > 0)
        for i in range(3):
            s.wheel(300)
            s.settle()
        vp = [r for r in s.viewport() if r["mode"] != "newselect"]
        self.check(s, 13, "selected row scrolled above the view", 4 not in [r["rownum"] for r in vp])
        b = vp[1]["rownum"]
        y0 = vp[1]["page_y"]
        s.click_row(b)
        y_immediate = s.js("t.rows[arguments[0]].getBoundingClientRect().top", b)
        s.settle()
        y1 = s.js("t.rows[arguments[0]].getBoundingClientRect().top", b)
        self.check(s, 13, "clicked row does not jump on click", abs(y_immediate - y0) <= 2, "row %s page_y %s -> %s (immediately)" % (b, y0, y_immediate))
        self.check(s, 13, "clicked row stays in place", abs(y1 - y0) <= 2, "row %s page_y %s -> %s (settled)" % (b, y0, y1))
        self.check_invariants(s, 13, "invariants after click", thumb_tolerance=0.2)

    def test14(self):
        reporter.add_test(14, "Height change of the first loaded row")
        s = self.session(DETAIL_APP)
        self.check(s, 14, "wheel to record ~120", s.scroll_to_record(120))
        first = s.js("t.rows.first")
        self.check(s, 14, "first loaded row is above the view", first is not None and first > 1 and first < s.js("t.rows.firstvis"))
        ref = s.middle_visible_row()
        rel0 = s.js("__tl.relY(t.rows[arguments[0]])", ref)
        s.js("t.osrc.MoveToRecord(arguments[0], t)", first)
        self.check_settled(s, 14, "after selecting the first loaded row")
        rel1 = s.js("t.rows[arguments[0]] ? __tl.relY(t.rows[arguments[0]]) : null", ref)
        self.check(s, 14, "rows below move only by the height change", rel1 is not None and abs(rel1 - rel0) <= 80,
                   "row %s rel_y %s -> %s" % (ref, rel0, rel1))
        self.check_invariants(s, 14, "invariants after height change", thumb_tolerance=0.2)

    def placeholder(self, s):
        """The placeholder row as listed by viewport(), or None if not fully in view."""
        ph = [r for r in s.viewport() if r["mode"] == "newselect"]
        return ph[0] if ph else None

    def above_slot(self, rows, slot):
        """(rownum, page_y) of the real rows above the placeholder slot."""
        return [(r["rownum"], round(r["page_y"])) for r in rows if r["rownum"] is not None and r["rownum"] < slot]

    def collect_ids(self, s):
        """Wheel to the end of the results, returning {Id: count} of the records seen."""
        seen = {}
        for i in range(150):
            ids = s.js("(function(){ var ic = __tl.colIndex(t, 'Id'), nc = __tl.colIndex(t, 'Name'), r = []; for (var i = t.rows.first; i <= t.rows.last; i++) if (t.rows[i]) r.push([i, t.rows[i].cols[ic].data, t.rows[i].cols[nc].data]); return r; })()")
            for rownum, rid, name in ids:
                seen[rownum] = rid if rid not in ("", None) else ("name:" + name)
            if s.js("t.osrc.FinalRecord !== null && t.rows.last >= t.osrc.FinalRecord"):
                break
            s.wheel(1600)
            s.settle()
        counts = {}
        for rid in seen.values():
            counts[str(rid)] = counts.get(str(rid), 0) + 1
        return counts

    def check_ids(self, s, test_no, label, counts, absent=(), extra=()):
        """Check the walked records: every Id from the first one seen to the end, once each."""
        ids = [int(k) for k in counts if not k.startswith("name:")]
        first_id = min(ids) if ids else 1
        dupes = sorted([k for k, c in counts.items() if c > 1])
        missing = [i for i in range(first_id, TOTAL_ROWS + 1) if str(i) not in counts and i not in absent]
        present = [i for i in absent if str(i) in counts]
        self.check(s, test_no, label + ": no duplicated records", not dupes, "duplicated: %s" % dupes[:20])
        self.check(s, test_no, label + ": no skipped records (from Id %d)" % first_id, not missing, "missing Ids: %s" % missing[:20])
        if absent:
            self.check(s, test_no, label + ": deleted record not shown", not present, "still shown: %s" % present)
        for name in extra:
            self.check(s, test_no, label + ": %s shown once" % name, counts.get("name:" + name) == 1, "count=%s" % counts.get("name:" + name))

    def test15(self):
        reporter.add_test(15, "New row placeholder at the end of the loaded rows (newrow_position end)")
        s = self.session()
        self.check(s, 15, "wheel to record ~200", s.scroll_to_record(200))
        last = s.js("t.osrc.LastRecord")
        s.click_button("new_btn")
        self.check_settled(s, 15, "after New")
        self.check(s, 15, "placeholder slot is after the last loaded record", s.js("t.newrow_slot") == last + 1, "slot=%s LastRecord=%s" % (s.js("t.newrow_slot"), last))
        ph = self.placeholder(s)
        self.check(s, 15, "placeholder row is in the viewport", ph is not None)
        self.check_invariants(s, 15, "invariants with the placeholder shown", check_order=True)
        vp_new = s.viewport()
        s.click_button("cancel_btn")
        self.check_settled(s, 15, "after Cancel")
        self.check(s, 15, "placeholder removed", s.js("t.newrow === null && t.newrow_slot === null"))
        if ph:
            vp = s.viewport()
            self.check(s, 15, "Cancel does not scroll", self.above_slot(vp_new, ph["slot"])[:5] == self.above_slot(vp, ph["slot"])[:5],
                       "before %s after %s" % (self.above_slot(vp_new, ph["slot"])[:3], self.above_slot(vp, ph["slot"])[:3]))
        self.check_invariants(s, 15, "invariants after Cancel", check_order=True)

    def new_and_save(self, s, test_no, name, fields=()):
        """New + Save at the configured position; checks the record stays at the placeholder."""
        s.click_button("new_btn")
        s.settle()
        ph = self.placeholder(s)
        self.check(s, test_no, "placeholder row is in the viewport", ph is not None)
        vp_new = s.viewport()
        for eb, text in tuple(fields) + (("name_eb", name), ("amount_eb", "5")):
            self.type_into(s, eb, text)
        s.click_button("save_btn")
        self.check_settled(s, test_no, "after Save", timeout=30)
        vp = s.viewport()
        created = [r for r in vp if r["name"] == name]
        self.check(s, test_no, "created record visible", len(created) == 1, [r["name"] for r in vp])
        if ph and created:
            self.check(s, test_no, "created record is where the placeholder was", abs(created[0]["page_y"] - ph["page_y"]) <= 2 and created[0]["rownum"] == ph["slot"],
                       "placeholder slot %s y %s, record row %s y %s" % (ph["slot"], ph["page_y"], created[0]["rownum"], created[0]["page_y"]))
            self.check(s, test_no, "Save does not scroll", self.above_slot(vp_new, ph["slot"]) == self.above_slot(vp, ph["slot"]),
                       "before %s after %s" % (self.above_slot(vp_new, ph["slot"])[:3], self.above_slot(vp, ph["slot"])[:3]))
        self.check_invariants(s, test_no, "invariants after Save")
        return ph

    def delete_created(self, s, name):
        if s.js("__tl.replicaValue(t.osrc, t.osrc.CurrentRecord, 'Name')") == name:
            s.click_button("delete_btn")
            s.settle(30)

    def test16(self):
        reporter.add_test(16, "Create and delete mid-list without skipped or duplicated records")
        if not self.destructive_ok(16):
            return

        # Create at the end of the loaded rows, then walk to the end.
        s = self.session()
        try:
            self.check(s, 16, "wheel to record ~200", s.scroll_to_record(200))
            self.new_and_save(s, 16, "Created Row")
            self.check_ids(s, 16, "after create", self.collect_ids(s), extra=("Created Row",))
        finally:
            s.settle()
            self.delete_created(s, "Created Row")

        # Delete a record mid-list, then walk to the end.
        s = self.session()
        deleted = None
        try:
            self.check(s, 16, "wheel to record ~300", s.scroll_to_record(300))
            target = [r["rownum"] for r in s.viewport() if r["rownum"] and r["rownum"] % 7 != 0][3]
            s.click_row(target)
            s.settle()
            cur = s.js("t.osrc.CurrentRecord")
            self.check(s, 16, "record to delete became the current record", cur == target, "CurrentRecord=%s target=%s" % (cur, target))
            if cur == target:
                s.click_button("delete_btn")
                deleted = target
                self.check_settled(s, 16, "after Delete", timeout=30)
                self.check_ids(s, 16, "after delete", self.collect_ids(s), absent=(target,))
        finally:
            if deleted:
                self.recreate_record(deleted)

    def test17(self):
        reporter.add_test(17, "Seeded random stress")
        seeds = self.config.get("stress_seeds") or [random.randrange(1, 1000000)]
        steps = int(self.config.get("stress_steps", 150))
        for seed in seeds:
            label = "seed %d: invariants hold for %d steps" % (seed, steps)
            rng = random.Random(seed)
            s = self.session()
            done = []
            failure = None
            for n in range(steps):
                step = self.stress_step(s, rng)
                done.append(step)
                if not s.settle(20):
                    failure = "stuck (not settled) after step %d" % n
                    break
                v = s.invariants()
                if v:
                    failure = "after step %d: %s" % (n, "; ".join(v))
                    break
            confirms = s.js("__tl.confirms")
            if confirms:
                print("  [test 17] seed %d: confirm() dialogs auto-accepted: %s" % (seed, confirms))
            detail = None
            if failure:
                detail = "%s | steps: %s" % (failure, done)
            self.check(s if failure else None, 17, label, failure is None, detail)

    def test18(self):
        reporter.add_test(18, "New row in view without scrolling (newrow_position auto)")
        s = self.session(DETAIL_APP)
        self.check(s, 18, "wheel to record ~100", s.scroll_to_record(100))
        sel = s.middle_visible_row()
        s.click_row(sel)
        s.settle()
        vp0 = s.viewport()
        s.click_button("new_btn")
        self.check_settled(s, 18, "after New")
        slot = s.js("t.newrow_slot")
        self.check(s, 18, "placeholder slot is after the selected row", slot == sel + 1, "slot=%s selected=%s" % (slot, sel))
        ph = self.placeholder(s)
        self.check(s, 18, "placeholder row is in the viewport", ph is not None)
        self.check(s, 18, "New does not scroll", self.above_slot(vp0, slot) == self.above_slot(s.viewport(), slot),
                   "before %s after %s" % (self.above_slot(vp0, slot)[:3], self.above_slot(s.viewport(), slot)[:3]))
        if ph:
            self.check(s, 18, "placeholder shows its row detail", ph["height"] > 40, "height=%s" % ph["height"])
        self.check_invariants(s, 18, "invariants with the placeholder shown", thumb_tolerance=0.2)
        if self.config.get("destructive"):
            s.click_button("cancel_btn")
            s.settle()
            s.click_row(sel)
            s.settle()
            try:
                self.new_and_save(s, 18, "Auto Row")
            finally:
                s.settle()
                self.delete_created(s, "Auto Row")
        else:
            s.click_button("cancel_btn")
            self.check_settled(s, 18, "after Cancel")
            self.check_invariants(s, 18, "invariants after Cancel", thumb_tolerance=0.2)

    def test19(self):
        reporter.add_test(19, "New row placeholder at the start of the loaded rows (newrow_position start)")
        s = self.session()
        s.js("(t.newrow_pos = 'start', 0)")
        self.check(s, 19, "wheel to record ~200", s.scroll_to_record(200))
        first = s.js("t.osrc.FirstRecord")
        s.click_button("new_btn")
        self.check_settled(s, 19, "after New")
        self.check(s, 19, "placeholder slot is the first loaded record", s.js("t.newrow_slot") == first, "slot=%s FirstRecord=%s" % (s.js("t.newrow_slot"), first))
        self.check(s, 19, "placeholder row is in the viewport", self.placeholder(s) is not None)
        self.check_invariants(s, 19, "invariants with the placeholder shown", check_order=True)
        s.click_button("cancel_btn")
        self.check_settled(s, 19, "after Cancel")
        self.check_invariants(s, 19, "invariants after Cancel", check_order=True)
        if not self.config.get("destructive"):
            return

        # Insert at the current record's slot: Changing must report the
        # previous current record's values as the old values.
        cur, first = s.js("[t.osrc.CurrentRecord, t.osrc.FirstRecord]")
        self.check(s, 19, "current record is the first loaded record", cur == first == 1, "CurrentRecord=%s FirstRecord=%s" % (cur, first))
        s.js("""(function(){ var iv = t.osrc.ifcProbe(ifValue); var orig = iv.Changing; __tl.changes = [];
            iv.Changing = function(n, nv, f, ov) { __tl.changes.push([n, nv, ov]); return orig.apply(this, arguments); }; return 0; })()""")
        try:
            self.new_and_save(s, 19, "Start Row")
            names = [c for c in s.js("__tl.changes") if c[0] == "Name" and c[1] == "Start Row"]
            self.check(s, 19, "Changing reports the previous record's Name as the old value",
                       len(names) >= 1 and all(c[2] == "Row 0001" for c in names), "Name changes (attr, new, old): %s" % names)
        finally:
            s.settle()
            self.delete_created(s, "Start Row")

    def test20(self):
        reporter.add_test(20, "Jump past the end of the results")
        s = self.session()
        s.js("(setTimeout(function(){ t.osrc.MoveToRecord(2000, t); }, 0), 0)")
        self.check_settled(s, 20, "after MoveToRecord(2000)", timeout=30)
        final, cur = s.js("[t.osrc.FinalRecord, t.osrc.CurrentRecord]")
        self.check(s, 20, "end of results found at record %d" % TOTAL_ROWS, final == TOTAL_ROWS, "FinalRecord=%s" % final)
        self.check(s, 20, "current record is the last record", cur == TOTAL_ROWS, "CurrentRecord=%s" % cur)
        scan = s.js("__tl.scanVis(t)")
        self.check(s, 20, "last record visible", scan["lastvis"] == TOTAL_ROWS, "scan lastvis=%s" % scan["lastvis"])
        self.check_invariants(s, 20, "invariants after the jump", check_order=True)
        s.js("(setTimeout(function(){ t.osrc.MoveToRecord(500, t); }, 0), 0)")
        self.check_settled(s, 20, "after MoveToRecord(500)", timeout=30)
        scan = s.js("__tl.scanVis(t)")
        self.check(s, 20, "record 500 still reachable", scan["firstvis"] is not None and scan["firstvis"] <= 500 <= scan["lastvis"],
                   "scan firstvis/lastvis=%s/%s" % (scan["firstvis"], scan["lastvis"]))
        self.check_invariants(s, 20, "invariants at record 500", check_order=True)

        s = self.session()
        s.js("(setTimeout(function(){ t.osrc_busy = true; t.osrc.ScrollTo(1500, 1540); }, 0), 0)")
        self.check_settled(s, 20, "after ScrollTo(1500, 1540)", timeout=30)
        final = s.js("t.osrc.FinalRecord")
        self.check(s, 20, "ScrollTo past the end keeps the real end", final is None or final == TOTAL_ROWS, "FinalRecord=%s" % final)
        self.check_invariants(s, 20, "invariants after ScrollTo past the end", check_order=True)

    def test21(self):
        reporter.add_test(21, "Reconciliation keeps records with Object.prototype names")
        s = self.session()
        res = s.js("""(function(){
            var o = t.osrc;
            var real = __tl.replicaValue(o, 1, 'name');
            o.deleted_names[String(__tl.replicaValue(o, 2, 'name'))] = true;
            var known = osrc_replica_names(o);
            var r = [['real', osrc_skip_fetched_name(o, known, real)], ['deleted', osrc_skip_fetched_name(o, known, __tl.replicaValue(o, 2, 'name'))]];
            ['constructor', 'toString', '__proto__', 'hasOwnProperty', 'valueOf'].forEach(function(n){ r.push([n, osrc_skip_fetched_name(o, known, n)]); });
            delete o.deleted_names[String(__tl.replicaValue(o, 2, 'name'))];
            return r;
            })()""")
        res = dict(res)
        self.check(s, 21, "loaded record is skipped", res["real"] is True, res)
        self.check(s, 21, "deleted record is skipped", res["deleted"] is True, res)
        for n in ("constructor", "toString", "__proto__", "hasOwnProperty", "valueOf"):
            self.check(s, 21, "record named %s is kept" % n, res[n] is False, res)

    def test22(self):
        reporter.add_test(22, "First record in an empty result set")
        s = self.session(EMPTY_APP, empty=True)
        ndm = "$(t).children('#ndm').is(':visible')"
        self.check(s, 22, "no data message shown", s.js(ndm))
        s.click_button("new_btn")
        self.check_settled(s, 22, "after New")
        ph = self.placeholder(s)
        self.check(s, 22, "placeholder row is in the viewport", ph is not None)
        if ph:
            self.check(s, 22, "placeholder shows its row detail", ph["height"] > 40, "height=%s" % ph["height"])
        self.check(s, 22, "no data message hidden", not s.js(ndm))
        s.click_button("cancel_btn")
        self.check_settled(s, 22, "after Cancel")
        self.check(s, 22, "placeholder removed", s.js("t.newrow === null && t.newrow_slot === null"))
        self.check(s, 22, "no data message shown again", s.js(ndm))
        if not self.config.get("destructive"):
            return
        try:
            # Id -1 matches the app's query (Id < 0), which the server re-runs to find the new record.
            ph = self.new_and_save(s, 22, "First Row", fields=(("id_eb", "-1"),))
            self.check(s, 22, "created record is row 1", s.js("t.rows.first === 1 && t.rows.last === 1"), "rows %s..%s" % (s.js("t.rows.first"), s.js("t.rows.last")))
        finally:
            s.settle()
            self.delete_created(s, "First Row")

    def thumb_target_record(self, s):
        """Record the thumb's current position points to."""
        return s.js("Math.floor(((__tl.relY(t.scrollbar.b) - 18) / Math.max(1, t.thumb_avail - t.thumb_height)) * Math.max(0, t.thumb_total - t.thumb_per_view)) + 1")

    def check_jump(self, s, test_no, label, expect):
        self.check_settled(s, test_no, label, timeout=60)
        self.check_invariants(s, test_no, "invariants " + label, check_order=True)
        fv = s.js("__tl.scanVis(t).firstvis")
        self.check(s, test_no, "view near record %d %s" % (expect, label), fv is not None and abs(fv - expect) <= 100, "firstvis=%s" % fv)

    def test23(self):
        reporter.add_test(23, "Thumb drag into the replica hole after Last")

        # A real drag: many small mouse moves up to the middle of the track.
        s = self.session()
        s.click_button("last_btn")
        self.check_settled(s, 23, "after Last", timeout=30)
        avail, height, top = s.js("[t.thumb_avail, t.thumb_height, __tl.relY(t.scrollbar.b)]")
        target = 18 + 0.5 * (avail - height)
        steps = 25
        chain = ActionChains(s.d).click_and_hold(s.js("t.scrollbar.b"))
        for i in range(steps):
            chain = chain.move_by_offset(0, int(round((target - top) / steps))).pause(0.03)
        chain.release().perform()
        self.check_jump(s, 23, "after dragging the thumb", self.thumb_target_record(s))

        # Two thumb jumps in a row: the second waits while the first loads.
        s = self.session()
        s.click_button("last_btn")
        s.settle(30)
        s.js("(t.ThumbScroll(0.9), t.ThumbScroll(0.5), 0)")
        expect = s.js("Math.floor(0.5 * Math.max(0, t.thumb_total - t.thumb_per_view)) + 1")
        self.check_jump(s, 23, "after two quick thumb jumps", expect)

        # Scrolling afterwards: no delayed jump from a superseded request.
        for i in range(3):
            s.wheel(100)
            s.settle()
        fv0 = s.js("__tl.scanVis(t).firstvis")
        time.sleep(2)
        s.settle()
        fv1 = s.js("__tl.scanVis(t).firstvis")
        self.check(s, 23, "view stays put after scrolling", fv0 == fv1, "firstvis %s -> %s" % (fv0, fv1))

    def drag_thumb_sampled(self, s, frac, steps=8):
        """Drag the thumb to frac of its track in several moves while sampling its position.

        Returns (start_top, target_top, samples) where samples are the thumb's
        top every 15ms from just before the drag until the table settles.
        """
        avail, height, top = s.js("[t.thumb_avail, t.thumb_height, __tl.relY(t.scrollbar.b)]")
        target = 18 + frac * (avail - height)
        s.js("(__tl.samp = [], __tl.dragStarted = false, __tl.sampTimer = setInterval(function(){ __tl.samp.push(__tl.relY(t.scrollbar.b)); if (window.tbldx_current) __tl.dragStarted = true; }, 15), 0)")
        chain = ActionChains(s.d).click_and_hold(s.js("t.scrollbar.b"))
        for i in range(steps):
            chain = chain.move_by_offset(0, int(round((target - top) / steps))).pause(0.04)
        chain.release().perform()
        s.settle(60)
        samples = s.js("(clearInterval(__tl.sampTimer), __tl.samp)")
        return top, target, samples

    def check_thumb_samples(self, s, test_no, label, start, target, samples):
        lo, hi = min(start, target) - 30, max(start, target) + 30
        strays = [round(y) for y in samples if y < lo or y > hi]
        self.check(s, test_no, label + ": thumb stays between its start and drop positions", not strays,
                   "start %d, drop %d, out of range: %s" % (start, target, strays[:10]))
        reached = [i for i, y in enumerate(samples) if abs(y - target) <= 10]
        back = [round(y) for y in samples[reached[0]:] if abs(y - target) > 30] if reached else []
        self.check(s, test_no, label + ": thumb reaches its drop position", bool(reached), "drop %d, samples %s" % (target, [round(y) for y in samples[-10:]]))
        self.check(s, test_no, label + ": thumb does not snap back", not back, "drop %d, later samples %s" % (target, back[:10]))
        final = s.js("__tl.relY(t.scrollbar.b)")
        self.check(s, test_no, label + ": thumb ends at its drop position", abs(final - target) <= 10, "drop %d, final %d" % (target, final))

    def test24(self):
        reporter.add_test(24, "Thumb stays where it is dragged")

        # Slow data: the thumb must not follow the old view while the jump loads.
        s = self.session()
        s.click_button("last_btn")
        s.settle(30)
        s.click_button("first_btn")
        s.settle(30)
        s.d.execute_cdp_cmd("Network.enable", {})
        s.d.execute_cdp_cmd("Network.emulateNetworkConditions", {"offline": False, "latency": 500, "downloadThroughput": -1, "uploadThroughput": -1})
        try:
            start, target, samples = self.drag_thumb_sampled(s, 0.3)
        finally:
            s.d.execute_cdp_cmd("Network.emulateNetworkConditions", {"offline": False, "latency": 0, "downloadThroughput": -1, "uploadThroughput": -1})
        self.check_thumb_samples(s, 24, "slow load", start, target, samples)
        self.check_invariants(s, 24, "invariants after the slow-load drag", check_order=True)

        # A thumb animation still running when the drag starts (as after First).
        s = self.session()
        s.click_button("last_btn")
        s.settle(30)
        s.click_button("first_btn")
        s.settle(30)
        # Slow enough that the press still lands on the (small) moving thumb.
        s.js("(setRelativeY(t.scrollbar.b, t.thumb_avail - t.thumb_height + 18), $(t.scrollbar.b).animate({top: '18px'}, 6000), 0)")
        time.sleep(0.3)
        start, target, samples = self.drag_thumb_sampled(s, 0.3)
        self.check(s, 24, "running animation: the drag started", s.js("__tl.dragStarted"), "the press did not land on the thumb")
        self.check_thumb_samples(s, 24, "running animation", start, target, samples)
        self.check_invariants(s, 24, "invariants after dragging during an animation", check_order=True)

    def stress_step(self, s, rng):
        kinds = ["wheel", "wheel", "wheel", "bigwheel", "drag", "click", "first", "last", "prev", "next", "refresh", "newcancel"]
        exclude = self.config.get("stress_exclude") or []
        kind = rng.choice([k for k in kinds if k not in exclude])
        if kind == "wheel":
            dy = rng.choice([-1, 1]) * rng.randint(100, 600)
            s.wheel(dy)
            return "wheel %d" % dy
        if kind == "bigwheel":
            dy = rng.choice([-1, 1]) * rng.randint(1000, 3000)
            s.wheel(dy)
            return "wheel %d" % dy
        if kind == "drag":
            avail, height, top = s.js("[t.thumb_avail, t.thumb_height, __tl.relY(t.scrollbar.b)]")
            if avail - height < 4:
                return "drag skipped"
            frac = rng.random()
            target = 18 + frac * (avail - height)
            ActionChains(s.d).click_and_hold(s.js("t.scrollbar.b")).move_by_offset(0, int(target - top)).release().perform()
            return "drag %.2f" % frac
        if kind == "click":
            vp = [r for r in s.viewport() if r["mode"] != "newselect"]
            if not vp:
                return "click skipped"
            r = rng.choice(vp)["rownum"]
            s.click_row(r)
            return "click row %d" % r
        if kind == "newcancel":
            s.click_button("new_btn")
            s.settle()
            s.click_button("cancel_btn")
            return "new+cancel"
        s.click_button(kind + "_btn")
        return kind

    def run(self):
        only = self.config.get("only")
        tests = [getattr(self, "test%d" % i) for i in range(1, 25) if not only or i in only]
        try:
            if not self.data_is_clean(self.session()):
                print("Test data is not in its generated state.  Stop the server, run")
                print("centrallix-os/tests/ui/table_large/gen_table_large_csv.py, and restart it.")
                return False
            for t in tests:
                print("Running %s" % t.__name__)
                num = int(t.__name__[4:])
                self.last_session = None
                try:
                    t()
                    if self.last_session is not None:
                        errs = self.last_session.js("__tl.errors.splice(0)")
                        if errs:
                            self.check(None, num, "no JavaScript errors (end of test)", False, "; ".join(errs[:5]))
                except Exception as e:
                    if num not in reporter.tests:
                        reporter.add_test(num, t.__name__)
                    self.check(None, num, "ran without a harness error", False, "%s: %s" % (type(e).__name__, e))
        finally:
            if self.driver:
                self.driver.quit()
        return reporter.print_report()


def run_test():
    try:
        config = toml.load("config.toml")
    except FileNotFoundError:
        print("Config.toml is missing. Make sure to rename config.template and try again.")
        return
    result = Suite(config).run()
    sys.exit(0) if result else sys.exit(1)


if __name__ == "__main__":
    run_test()
