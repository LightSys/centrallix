# David Hopkins May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
import sys

class TestBlock:
    """A class to manage a block of test checks and format the output."""
    def __init__(self, number, name):
        self.number = number
        self.name = name
        self.checks = []

    def start(self):
        """Prints the header for this test block."""
        print(f"START TEST {self.number}")

    def add_check(self, description, passed: bool):
        """Adds a check to the block and prints its immediate status."""
        self.checks.append(passed)
        status = "PASS" if passed else "FAIL"
        print(f"    Test {description} ... {status}")

    def conclude(self) -> bool:
        """Prints the summary for the block and returns its overall status."""
        passed_count = sum(1 for p in self.checks if p)
        total_count = len(self.checks)
        block_passed = passed_count == total_count and total_count > 0
        status = "PASS" if block_passed else "FAIL"
        print(f"TEST {self.number} = {self.name} ({passed_count}/{total_count}) {status}\n")
        return block_passed

def run_test():
    """Runs the scrollpane test with structured reporting."""
    print("# UI Test coverage: ScrollPane and TreeView Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/scrollpane_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)
        
        # --- TEST 1: Page Initialization ---
        init_test = TestBlock(1, "Page Initialization")
        init_test.start()
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized", True)
        except Exception as e:
            init_test.add_check("page or framework failed to initialize", False)
            print(f"    (Error) {e}")
        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Tree View Expansion ---
        tree_test = TestBlock(2, "Tree View Expansion")
        tree_test.start()
        
        actions = ActionChains(driver)
        def expand_node(node_name):
            try:
                xpath = f"//div[starts-with(@class, 'tv') and contains(., '{node_name}')]//img[contains(@src, 'sys/images/ico02b.gif')]"
                node = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, xpath)))
                driver.execute_script("arguments[0].scrollIntoView({block: 'center', inline: 'center'});", node)
                time.sleep(1)
                actions.move_to_element(node).click().perform()
                time.sleep(1)
                return True
            except Exception:
                return False

        nodes_to_expand = ['/', 'apps', 'kardia', 'sys', 'tests']
        for node in nodes_to_expand:
            tree_test.add_check(f"expanding '{node}' node", expand_node(node))
        
        all_blocks_passed.append(tree_test.conclude())

        # --- TEST 3: Scrollpane Functionality ---
        scroll_test = TestBlock(3, "ScrollPane Functionality")
        scroll_test.start()

        # RESTORING your original, correct helper function to read computed styles
        def get_scroll_positions(pane):
            top_pos_str = driver.execute_script("return window.getComputedStyle(arguments[0]).top;", pane)
            scroll_top_val = driver.execute_script("return arguments[0].scrollTop;", pane)
            try:
                top_pos_float = float(top_pos_str.replace('px', ''))
            except (ValueError, AttributeError):
                top_pos_float = 0.0
            return top_pos_float, scroll_top_val

        try:
            scrollpane_area = driver.find_element(By.XPATH, "//div[contains(@style, 'clip: rect') and contains(@style, 'z-index: 14')]")
            
            # Test Down Arrow
            initial_top, initial_scroll = get_scroll_positions(scrollpane_area)
            down_arrow = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//img[@name='d']")))
            actions.move_to_element(down_arrow).click().perform()
            time.sleep(2)
            new_top, new_scroll = get_scroll_positions(scrollpane_area)
            did_scroll_down = (new_top < initial_top) or (new_scroll > initial_scroll)
            scroll_test.add_check("down arrow scrolls pane down", did_scroll_down)
            
            # Test Up Arrow
            initial_top, initial_scroll = get_scroll_positions(scrollpane_area)
            up_arrow = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//img[@name='u']")))
            actions.move_to_element(up_arrow).click().perform()
            time.sleep(2)
            new_top, new_scroll = get_scroll_positions(scrollpane_area)
            did_scroll_up = (new_top > initial_top) or (new_scroll < initial_scroll)
            scroll_test.add_check("up arrow scrolls pane up", did_scroll_up)

            # Test Thumb Drag
            initial_top, initial_scroll = get_scroll_positions(scrollpane_area)
            thumb = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.XPATH, "//img[@name='t']")))
            actions.click_and_hold(thumb).move_by_offset(0, 100).release().perform()
            time.sleep(2)
            new_top, new_scroll = get_scroll_positions(scrollpane_area)
            did_drag_down = (new_top < initial_top) or (new_scroll > initial_scroll)
            scroll_test.add_check("dragging thumb scrolls pane down", did_drag_down)

        except Exception as e:
            scroll_test.add_check("all scroll actions", False)
            print(f"    (Error) Scrollbar interaction failed: {e}")

        all_blocks_passed.append(scroll_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 3:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"ScrollPane Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()