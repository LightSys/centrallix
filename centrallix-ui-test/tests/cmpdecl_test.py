# David Hopkns June 2025
# CMPDECL Part 1 Test
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
import random
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException, ElementClickInterceptedException
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
    print("# UI Test coverage: CMPDECL Part 1 Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/cmpdecl_test.app"

        service = Service(ChromeDriverManager().install())
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--lang=en')
        chrome_options.add_argument('--incognito')
        chrome_options.add_argument('--ignore-certificate-errors')

        driver = webdriver.Chrome(service=service, options=chrome_options)
        driver.set_window_size(1920, 1080)

        # --- TEST 1: Page and Framework Initialization ---
        init_test = TestBlock(1, "Page and Framework Initialization")
        init_test.start()
        try:
            driver.get(test_url)
            WebDriverWait(driver, 10).until(lambda d: d.execute_script("return document.readyState") == "complete")
            init_test.add_check("page loaded successfully", True)
        except Exception:
            init_test.add_check("page loaded successfully", False)

        try:
            WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.TAG_NAME, "body")))
            init_test.add_check("body element is present", True)
        except Exception:
            init_test.add_check("body element is present", False)

        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception:
            init_test.add_check("framework is initialized (pg_isloaded)", False)
        all_blocks_passed.append(init_test.conclude())

        # --- TEST 2: Text Area Interaction ---
        textarea_test = TestBlock(2, "Text Area Interaction")
        textarea_test.start()
        textarea = None
        try:
            textarea_selector = "div[id^='tx'][id$='base'] textarea"
            textarea = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.CSS_SELECTOR, textarea_selector)))
            textarea_test.add_check("textarea element is found", True)
            
            time.sleep(2) # Pause to observe
            textarea.click()
            textarea_test.add_check("textarea is clickable", True)

            time.sleep(1) # Pause to observe click
            textarea.send_keys("Hello! This is my fake password: 1234567890!@#$%^&*()_+-=[]{}|;':\",.<>?/")
            textarea_test.add_check("initial text can be sent", True)

            random_text = ''.join(random.choices('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:\'",.<>?/~`', k=1000))
            textarea.send_keys(random_text)
            time.sleep(5) # Pause to observe large input
            textarea_test.add_check("large random text can be sent", True)

        except Exception:
            # If any step failed, mark remaining checks as failed
            if not any("textarea element is found" in check for check in textarea_test.checks):
                textarea_test.add_check("textarea element is found", False)
            if not any("textarea is clickable" in check for check in textarea_test.checks):
                textarea_test.add_check("textarea is clickable", False)
            if not any("initial text can be sent" in check for check in textarea_test.checks):
                textarea_test.add_check("initial text can be sent", False)
            if not any("large random text can be sent" in check for check in textarea_test.checks):
                textarea_test.add_check("large random text can be sent", False)
        all_blocks_passed.append(textarea_test.conclude())

        # --- TEST 3: Button Interaction ---
        button_test = TestBlock(3, "Button Interaction")
        button_test.start()
        button_div = None
        try:
            button_div_selector = "div[id^='tb'][id$='pane']"
            button_div = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.CSS_SELECTOR, button_div_selector)))
            button_text = button_div.find_element(By.TAG_NAME, 'span').text
            button_test.add_check(f"button text retrieved: '{button_text}'", True)
        except Exception:
            button_test.add_check("button text could not be retrieved", False)

        if button_div:
            number_of_clicks = 5
            clicks_succeeded = 0
            time.sleep(1) # Pause before clicks start
            for i in range(number_of_clicks):
                try:
                    button_to_click = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.CSS_SELECTOR, button_div_selector)))
                    button_to_click.click()
                    clicks_succeeded += 1
                    time.sleep(0.5)
                except ElementClickInterceptedException:
                    print(f"    (Info) Click #{i+1} was intercepted, trying JS fallback.")
                    driver.execute_script("arguments[0].click();", button_to_click)
                    clicks_succeeded += 1 # Count JS click as success
                    time.sleep(0.5)
                except TimeoutException:
                    print(f"    (Info) Button lost before click #{i+1}, stopping.")
                    break # Exit loop
            
            # Add a single check for the result of the loop
            all_clicks_successful = (clicks_succeeded == number_of_clicks)
            button_test.add_check(f"attempted {number_of_clicks} clicks ({clicks_succeeded} succeeded)", all_clicks_successful)
        else:
            button_test.add_check(f"click attempts (SKIPPED)", False)
        all_blocks_passed.append(button_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 3:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"CMPDECL Part 1 Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 5 seconds.")
            time.sleep(5)
            driver.quit()

if __name__ == "__main__":
    run_test()