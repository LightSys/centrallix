# Updated by: [David Hopkins] May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

from selenium.webdriver.common.alert import Alert
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
import toml
import time
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
    """Runs the text button test with structured reporting."""
    print("# UI Test coverage: TextButton Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/textbutton_test.app"

        # --- Driver and Page Initialization ---
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

        # --- TEST 2: Button 1 Functionality ---
        button1_test = TestBlock(2, "Button 1 Functionality")
        button1_test.start()
        try:
            button_xpath = "//div[contains(@class, 'cell') and .//span[text()='Button 1']]"
            button1 = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.XPATH, button_xpath)))
            button1_test.add_check("locating the button", True)
            
            driver.execute_script("arguments[0].style.border = '3px solid red';", button1)
            time.sleep(1)
            button1_test.add_check("highlighting the button", True)

            WebDriverWait(driver, 20).until(lambda d: d.find_element(By.XPATH, button_xpath).value_of_css_property("cursor") == "default")
            button1_test.add_check("verifying cursor state is ready", True)

            button1.click()
            time.sleep(3)
            button1_test.add_check("clicking the button", True)
            
            WebDriverWait(driver, 10).until(EC.alert_is_present())
            alert = Alert(driver)
            alert_text = alert.text
            alert.accept()
            button1_test.add_check(f"handling alert (text: '{alert_text}')", True)

        except Exception as e:
            button1_test.add_check("button interaction failed", False)
            print(f"    (Error) {e}")
        all_blocks_passed.append(button1_test.conclude())

        # --- TEST 3: Button 2 Functionality ---
        button2_test = TestBlock(3, "Button 2 Functionality")
        button2_test.start()
        try:
            button_xpath = "//div[contains(@class, 'cell') and .//span[text()='Button 2']]"
            button2 = WebDriverWait(driver, 20).until(EC.element_to_be_clickable((By.XPATH, button_xpath)))
            button2_test.add_check("locating the button", True)

            driver.execute_script("arguments[0].style.border = '3px solid blue';", button2)
            time.sleep(1)
            button2_test.add_check("highlighting the button", True)

            WebDriverWait(driver, 20).until(lambda d: d.find_element(By.XPATH, button_xpath).value_of_css_property("cursor") == "default")
            button2_test.add_check("verifying cursor state is ready", True)

            button2.click()
            time.sleep(3)
            button2_test.add_check("clicking the button", True)
            
            WebDriverWait(driver, 10).until(EC.alert_is_present())
            alert = Alert(driver)
            alert_text = alert.text
            alert.accept()
            button2_test.add_check(f"handling alert (text: '{alert_text}')", True)

        except Exception as e:
            button2_test.add_check("button interaction failed", False)
            print(f"    (Error) {e}")
        all_blocks_passed.append(button2_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 3:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"TextButton Test {final_status}")
        print("---")
        sys.exit(0 if final_status == "PASS" else 1)

        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()