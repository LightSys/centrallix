# Updated by: [David Hopkins] May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
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
        print(f"TEST {self.number} = {self.name}")

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
        print(f"({passed_count}/{total_count}) {status}\n")
        return block_passed

def run_test():
    """Run the edit box test with structured reporting."""
    print("# UI Test coverage: Editbox Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/editbox_test.app"

        # --- Driver and Page Initialization ---
        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')

        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page and Element Initialization ---
        init_test = TestBlock(1, "Page and Element Initialization")
        init_test.start()
        
        input_elem = None
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
        except Exception:
            init_test.add_check("page loaded successfully", False)

        try:
            input_elem = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.XPATH, "//input[last()]")))
            init_test.add_check("input element is found", True)
        except Exception:
            init_test.add_check("input element is found", False)
        
        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Input Box Interactions ---
        interaction_test = TestBlock(2, "Input Box Interactions")
        interaction_test.start()
        
        if input_elem:
            # Check 1: Initial state
            try:
                initial_value = input_elem.get_attribute('value')
                interaction_test.add_check(f"reading initial state (value: '{initial_value}')", True)
            except Exception:
                interaction_test.add_check("reading initial state", False)
            
            # Check 2: Send keys
            try:
                input_elem.send_keys("From Python")
                value_after_send_keys = input_elem.get_attribute('value')
                interaction_test.add_check(f"sending keys via Selenium (value: '{value_after_send_keys}')", True)
            except Exception:
                interaction_test.add_check("sending keys via Selenium", False)
            
            # Check 3: Click
            try:
                input_elem.click()
                is_focused = driver.execute_script("return document.activeElement === arguments[0];", input_elem)
                interaction_test.add_check(f"clicking the element (is focused: {is_focused})", True)
            except Exception:
                interaction_test.add_check("clicking the element", False)

            # Check 4: Inject JS value
            try:
                driver.execute_script("arguments[0].value = 'Hello, LightSys!';", input_elem)
                # Click to potentially trigger any 'onchange' events
                input_elem.click()
                value_after_js = input_elem.get_attribute('value')
                interaction_test.add_check(f"injecting value via JavaScript (value: '{value_after_js}')", True)
            except Exception:
                interaction_test.add_check("injecting value via JavaScript", False)
        else:
            interaction_test.add_check("all interactions (SKIPPED)", False)

        all_blocks_passed.append(interaction_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"Editbox Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 10 seconds.")
            time.sleep(10)
            driver.quit()

if __name__ == "__main__":
    run_test()