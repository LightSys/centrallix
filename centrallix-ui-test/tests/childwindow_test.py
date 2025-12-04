# David Hopkins - June 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.common.exceptions import TimeoutException
from datetime import datetime
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
    """Runs the entire suite of tests and provides a formatted summary."""
    print("# UI Test coverage: Child Window Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/childwindow_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')
        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Initialization ---
        init_test = TestBlock(1, "Page and Framework Initialization")
        init_test.start()

        # Check 1: Page Load
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page ready state is complete", True)
        except Exception:
            init_test.add_check("page ready state is complete", False)

        # Check 2: Body Element
        try:
            WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.TAG_NAME, "body")))
            init_test.add_check("body element is present", True)
        except Exception:
            init_test.add_check("body element is present", False)

        # Check 3: Framework JS
        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework (pg_isloaded) is initialized", True)
        except Exception:
            init_test.add_check("framework (pg_isloaded) is initialized", False)

        # Check 4: Window JS
        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof wn_topwin !== 'undefined' && wn_topwin !== null"))
            init_test.add_check("window (wn_topwin) is initialized", True)
        except Exception:
            init_test.add_check("window (wn_topwin) is initialized", False)

        all_blocks_passed.append(init_test.conclude())
        time.sleep(1)

        # --- TEST 2: Window Interaction ---
        interaction_test = TestBlock(2, "Window Interaction Test")
        interaction_test.start()
        
        # Check 1: Find Titlebar
        titlebar = None
        try:
            titlebar = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.CSS_SELECTOR, "div.wntitlebar")))
            interaction_test.add_check("titlebar element is found", True)
        except Exception:
            interaction_test.add_check("titlebar element is found", False)

        if titlebar:
            # Check 2: JavaScript Drag
            try:
                driver.execute_script("""
                    var titlebar = arguments[0];
                    var mousedown = new MouseEvent('mousedown', { bubbles: true, clientX: 100, clientY: 100 });
                    var mousemove = new MouseEvent('mousemove', { bubbles: true, clientX: 600, clientY: 100 });
                    var mouseup = new MouseEvent('mouseup', { bubbles: true, clientX: 600, clientY: 100 });
                    titlebar.dispatchEvent(mousedown);
                    setTimeout(() => titlebar.dispatchEvent(mousemove), 500);
                    setTimeout(() => titlebar.dispatchEvent(mouseup), 1000);
                """, titlebar)
                time.sleep(2)
                interaction_test.add_check("javascript drag event", True)
            except Exception:
                interaction_test.add_check("javascript drag event", False)

            # Checks 3 & 4: Minimize and Restore
            actions = ActionChains(driver)
            try:
                actions.move_to_element(titlebar).double_click().perform()
                interaction_test.add_check("double-click to minimize", True)
            except Exception:
                interaction_test.add_check("double-click to minimize", False)
            
            time.sleep(1)
            try:
                actions.move_to_element(titlebar).double_click().perform()
                interaction_test.add_check("double-click to restore", True)
            except Exception:
                interaction_test.add_check("double-click to restore", False)
        else:
            # If titlebar isn't found, these tests cannot run
            interaction_test.add_check("javascript drag event (SKIPPED)", False)
            interaction_test.add_check("double-click to minimize (SKIPPED)", False)
            interaction_test.add_check("double-click to restore (SKIPPED)", False)

        all_blocks_passed.append(interaction_test.conclude())
        time.sleep(3)
        
        # --- TEST 3: Window Closing ---
        closing_test = TestBlock(3, "Window Closing Test")
        closing_test.start()
        try:
            close_button = WebDriverWait(driver, 10).until(EC.element_to_be_clickable((By.CSS_SELECTOR, "td[align='right']")))
            time.sleep(1)
            close_button.click()
            closing_test.add_check("close button click event", True)
        except Exception:
            closing_test.add_check("close button click event", False)
        
        all_blocks_passed.append(closing_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        # Mark all remaining tests as failed if there's a major breakdown
        while len(all_blocks_passed) < 3:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"ChildWindow Widget Test {final_status}")
        print("---\n")
        sys.exit(0 if final_status == "PASS" else 1)
        
        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()

if __name__ == "__main__":
    run_test()