# David Hopkins May 2025
# NOTE: USE ChromeDriverManager. Pip install it.

import toml
import time
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager

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
    """Runs the textarea test with structured reporting."""
    print("# UI Test coverage: TextArea Test")
    print("Author: David Hopkins")
    driver = None
    all_blocks_passed = []

    try:
        config = toml.load("config.toml")
        test_url = config["url"] + "/tests/ui/textarea_test.app"

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
        except Exception:
            init_test.add_check("page loaded successfully", False)
        
        try:
            WebDriverWait(driver, 20).until(lambda d: d.execute_script("return typeof pg_isloaded !== 'undefined' && pg_isloaded"))
            init_test.add_check("framework is initialized (pg_isloaded)", True)
        except Exception:
            init_test.add_check("framework is initialized (pg_isloaded)", False)
        
        all_blocks_passed.append(init_test.conclude())
        time.sleep(2)

        # --- TEST 2: Text Area Interaction ---
        textarea_test = TestBlock(2, "Text Area Interaction")
        textarea_test.start()
        
        textarea = None
        try:
            textarea = WebDriverWait(driver, 10).until(EC.presence_of_element_located((By.CSS_SELECTOR, "textarea")))
            textarea_test.add_check("locating the primary textarea", True)
        except Exception:
            textarea_test.add_check("locating the primary textarea", False)
        
        if textarea:
            try:
                textarea.click()
                time.sleep(1)
                textarea_test.add_check("clicking the primary textarea", True)
            except Exception:
                textarea_test.add_check("clicking the primary textarea", False)
            
            try:
                long_text = (
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. " * 5 +
                    "1234567890!@#$%^&*()_+-=[]{}|;:',.<>/?~` " * 5 +
                    "The quick brown fox jumps over the lazy dog. " * 5
                )
                textarea.send_keys(long_text)
                time.sleep(2)
                textarea_test.add_check("sending a long text string", True)
            except Exception:
                textarea_test.add_check("sending a long text string", False)

            try:
                textarea.clear()
                time.sleep(2)
                textarea_test.add_check("clearing the textarea", True)
            except Exception:
                textarea_test.add_check("clearing the textarea", False)

            # Check for a second text area
            try:
                textareas = driver.find_elements(By.CSS_SELECTOR, "textarea")
                if len(textareas) > 1:
                    second_textarea = textareas[1]
                    second_textarea.click()
                    time.sleep(2)
                    textarea_test.add_check("clicking the second textarea", True)
                else:
                    textarea_test.add_check("verifying no second textarea is present", True)
            except Exception as e:
                textarea_test.add_check("interacting with second textarea", False)
                print(f"    (Error) {e}")
        else:
            textarea_test.add_check("all textarea interactions skipped", False)

        all_blocks_passed.append(textarea_test.conclude())

    except Exception as e:
        print(f"\n--- A CRITICAL ERROR OCCURRED ---\n{e}")
        while len(all_blocks_passed) < 2:
            all_blocks_passed.append(False)

    finally:
        final_status = "PASS" if all(all_blocks_passed) else "FAIL"
        print(f"TextArea Test {final_status}")
        print("---")
        if driver:
            print("Test complete. Browser will close in 2 seconds.")
            time.sleep(2)
            driver.quit()

if __name__ == "__main__":
    run_test()